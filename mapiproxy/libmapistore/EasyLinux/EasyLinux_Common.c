/**
 * MAPIStoreEasyLinux - this file is part of EasyLinux project
 *
 * Copyright (C) 2013 Aisysco Ltd
 *
 * Author: Serge NOEL <serge.noel@net6a.com>
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 *
 * This file is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 * \file MAPIStoreEasyLinux.c
 * \brief Heart of EasyLinux mapistore backend library
 * \author Serge NOEL
 * \version 0.1
 * \date  2013-04-10
 *
 * Routines parts for ldap access
 *
 */

#include "mapiproxy/libmapistore/mapistore.h"
#include "mapiproxy/libmapistore/mapistore_errors.h"
#include "mapiproxy/libmapistore/EasyLinux/MAPIStoreEasyLinux.h"
#include <talloc.h>
#include <core/ntstatus.h>
#include <popt.h>
#include <param.h>
#include <util/debug.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <dirent.h>


#define YES		1
#define NO		0
#define BOOL	unsigned int


enum EasyLinux_Backend_Type GetBkType(char *sBackend)
{
int Return;

if( strncmp ("FALLBACK/", sBackend, 9)==0 )
  Return = EASYLINUX_FALLBACK;
else if( strncmp("INBOX/", sBackend, 6)==0 )
  Return = EASYLINUX_MAILDIR;
else if( strncmp("CALENDAR/", sBackend, 9)==0 )
  Return = EASYLINUX_CALENDAR;
else if( strncmp("CONTACTS/", sBackend, 9)==0 )
  Return = EASYLINUX_CONTACTS;
else if( strncmp("TASKS/", sBackend, 6)==0 )
  Return = EASYLINUX_TASKS;
else if( strncmp("NOTES/", sBackend, 6)==0 )
  Return = EASYLINUX_NOTES;
else if( strncmp("JOURNAL/", sBackend, 8)==0 )
  Return = EASYLINUX_JOURNAL;
else
  Return = EASYLINUX_UNKNOW;
return Return;
}

void Dump(void *Object)
{
struct EasyLinuxGeneric *elGeneric;
struct EasyLinuxFolder  *elFolder;

elGeneric = (struct EasyLinuxGeneric *)Object;

switch(elGeneric->stType)
  {
	case EASYLINUX_FOLDER:
	  elFolder = (struct EasyLinuxFolder *)Object;
	  DEBUG(0,("MAPIEasyLinux : Dumping FOLDER OBJECT\n"));
	  DEBUG(0,("MAPIEasyLinux :    Uri: %s\n",elFolder->Uri));
	  DEBUG(0,("MAPIEasyLinux :    FID: %lX\n",elFolder->FID));
	  DEBUG(0,("MAPIEasyLinux :    displayName: %s\n",elFolder->displayName));
	  DEBUG(0,("MAPIEasyLinux :    RelPath: %s\n",elFolder->RelPath));
	  DEBUG(0,("MAPIEasyLinux :    FullPath: %s\n",elFolder->FullPath));
    break;
    
  case EASYLINUX_USER:
	case EASYLINUX_MSG:
	case EASYLINUX_TABLE:
	case EASYLINUX_BACKEND:
	  break;

	default:
	  DEBUG(0,("ERROR: MAPIEasyLinux - Dump - Unknow object type (%i)\n",elGeneric->stType));
	  break;
  }
  
}

/*
 *
 * /home/NET6A/Administrator/Maildir/FALLBACK/0x1f04000000000001/
 */
int RecursiveMkDir(struct EasyLinuxUser *elUser, char *Path, mode_t Mode)
{
int LenHomeDir, LenPath, i, j;
DIR *curPath;

// First test if homeDirectory exist

LenHomeDir = strlen(elUser->homeDirectory);
LenPath = strlen(Path);
Path[LenHomeDir] = 0;
Path[LenHomeDir] = '/';
Path[LenPath-1] = 0;

// Start from last path to homeDir
for( i=(LenPath-3) ; i>(LenHomeDir-1) ; i--)
  {
  if( Path[i] == '/' )
    {
    Path[i] = 0;
    // try to open path
    curPath = opendir(Path);
    if( curPath != NULL )
      {  // This part of fullpath can be open, it is time to create 
      closedir(curPath);
      j = i+1;
      i=LenHomeDir;
      }
    }
  }
// Now we start from homeDir to FullPath
for( i=0 ; i!= (LenPath-1) ; i++ )
  {
  if( Path[i] == 0 )
    {
    Path[i] = '/';
    // Create folder
    DEBUG(3,("MAPIEasyLinux :   --> mkdir %s !\n", Path));
    if( mkdir(Path, Mode) && (EEXIST != errno) )
      {
      DEBUG(0,("ERROR : MAPIEasyLinux - cannot create %s\n",Path));
      DEBUG(0,("ERROR : MAPIEasyLinux - (%s)\n",strerror(errno)));
      return MAPISTORE_ERROR;
      }
    chown(Path, elUser->uidNumber, elUser->gidNumber);
    }
  }
  
return MAPISTORE_SUCCESS;
}

/*
 *
struct SPropValue {
	enum MAPITAGS ulPropTag;  // en HEX
	uint32_t dwAlignPad;
	union SPropValue_CTR value; // [switch_is(ulPropTag&0xFFFF)]  - PROP_VAL_UNION
} // [noprint,nopush,public,nopull] 

#define PT_UNSPECIFIED		0x0000	// ((ULONG)  0) (Reserved for interface use) type doesn't matter to caller 
#define PT_NULL						0x0001	// ((ULONG)  1) NULL property value 
#define	PT_I2							0x0002	// ((ULONG)  2) Signed 16-bit value 
#define PT_LONG						0x0003	// ((ULONG)  3) Signed 32-bit value 
#define	PT_R4							0X0004	// ((ULONG)  4) 4-byte floating point 
#define PT_DOUBLE					0x0005	// ((ULONG)  5) Floating point double 
#define PT_CURRENCY				0x0006	// ((ULONG)  6) Signed 64-bit int (decimal w/	4 digits right of decimal pt) 
#define	PT_APPTIME				0x0007	// ((ULONG)  7) Application time 
#define PT_ERROR					0x000A	// ((ULONG) 10) 32-bit error value 
#define PT_BOOLEAN				0x000B	// ((ULONG) 11) 16-bit boolean (non-zero true) 
#define PT_OBJECT					0x000D	// ((ULONG) 13) Embedded object in a property 
#define	PT_I8							0x0014	// ((ULONG) 20) 8-byte signed integer 
#define PT_STRING8				0x001E  // ((ULONG) 30) terminated Unicode string 
#define PT_UNICODE			  0x001F	// ((ULONG) 31) Null terminated Unicode string 
#define PT_SYSTIME				0x0040	// ((ULONG) 64) FILETIME 64-bit int w/ number of 100ns periods since Jan 1,1601 
#define	PT_CLSID					0x0048	// ((ULONG) 72) OLE GUID 
#define PT_BINARY					0x0102	// ((ULONG) 258) Uninterpreted (counted byte array) 

struct EasyLinuxProp {
	uint32_t												PropTag;
	int															PropType;
	void														*PropValue;
	};	
 
 */
void StorePropertie(struct EasyLinuxTable *Table, struct SPropValue Prop)
{
struct 	EasyLinuxProp **NewProps, **OldProps, *CurProps;
int i, 	SizeToAllocate;
uint8_t *Val8; 

// Table contains a dynamic table, we need to add one element
OldProps = Table->Props;
SizeToAllocate = sizeof(struct EasyLinuxProp *) * (Table->rowCount +1);
// Reserve room for pointers
NewProps = (struct EasyLinuxProp **)talloc_zero_size(Table->elContext->mem_ctx, SizeToAllocate );
for( i=0 ; i< Table->rowCount ; i++)
  NewProps[i] = OldProps[i];
Table->Props = NewProps;
Table->rowCount++;
if( OldProps != NULL )
  talloc_unlink(Table->elContext->mem_ctx,OldProps);

NewProps[Table->rowCount-1] = (struct EasyLinuxProp *)talloc_zero_size(Table->elContext->mem_ctx, sizeof(struct EasyLinuxProp));
CurProps = NewProps[Table->rowCount-1];
CurProps->PropTag = Prop.ulPropTag;

switch( Prop.ulPropTag & 0x0000FFFF )
  {
  case PT_UNICODE:
    CurProps->PropType = PT_UNICODE;
    CurProps->PropValue = (char *)talloc_strdup(Table->elContext->mem_ctx, Prop.value.lpszW);
    DEBUG(0,("MAPIEasyLinux :      0x%08X: %s\n", Prop.ulPropTag, (char *)Prop.value.lpszA));
    break;
    
  case PT_I8:
    CurProps->PropType = PT_I8;
    CurProps->PropValue = (char *)talloc_asprintf(Table->elContext->mem_ctx, "0x%X", Prop.value.l);
    //Val8 = (uint8_t *)talloc_zero_size(Table->elContext->mem_ctx, sizeof(uint8_t));
    //*Val8 = Prop.value.l; 
    //CurProps->PropValue = (void *)Val8;
    DEBUG(0,("MAPIEasyLinux :      0x%08X: %i\n", Prop.ulPropTag, (int8_t)Prop.value.l));
    break;
  
  default:
    DEBUG(0,("MAPIEasyLinux :      0x%08X: Need to be taken\n", Prop.ulPropTag));
    break;
  }



/*


switch( Prop.ulPropTag & 0xFFFF0000 )
  {
  case 0x30010000:  // PidTagDisplayName  0x3001001F 
    DEBUG(0,("MAPIEasyLinux :      PidTagDisplayName: %s\n",(char *)Prop.value.lpszA));
    break;
    
  case 0x67490000:  // PidTagParentFolderId  0x67490014  
    DEBUG(0,("MAPIEasyLinux :      PidTagParentFolderId: %lX\n",(uint64_t *)Prop.value.l));
    break;
    
  case 0x67A40000:  // PidTagChangeNumber  0x67A40014  
    DEBUG(0,("MAPIEasyLinux :      PidTagChangeNumber: %08X\n",(uint32_t )Prop.value.l));
    break;
    
  case 0x001A0000:  // PidTagMessageClass 0x001A001F
    DEBUG(0,("MAPIEasyLinux :      PidTagMessageClass: %s\n",(char *)Prop.value.lpszA));
    break;
    
  case 0x003D0000:  // PidTagSubjectPrefix 0x003D001F
    DEBUG(0,("MAPIEasyLinux :      PidTagSubjectPrefix: %s\n",(char *)Prop.value.lpszA));
    break;
    
  case 0x0E1D0000:  // PidTagNormalizedSubject 0x0E1D001F
    DEBUG(0,("MAPIEasyLinux :      PidTagNormalizedSubject: %s\n",(char *)Prop.value.lpszA));
    break;
        
  default:
    DEBUG(0,("MAPIEasyLinux :      HexTag: 0x%08X\n",Prop.ulPropTag));
    break;
  } */
}
