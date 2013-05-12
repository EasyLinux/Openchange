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
 */
int RecursiveMkDir(struct EasyLinuxContext *elContext, char *Path, mode_t Mode)
{
char *d;
int i, len;
int DoChown=0;

DEBUG(3,("MAPIEasyLinux : RecursiveMkDir %s\n",Path));
d = talloc_strdup(elContext->mem_ctx, Path);
len = strlen(Path);
for(i=1 ; i<len ; i++)
  {
  if( '/' == d[i] )
    {
    d[i] = '\0';
    if( mkdir(d, Mode) && (EEXIST != errno) )
      {
      DEBUG(0,("ERROR : MAPIEasyLinux - cannot create '%s' folder\n",d));
      talloc_unlink(elContext->mem_ctx, d);
      return MAPISTORE_ERROR;
      }
    if( strcmp(elContext->User.homeDirectory,d) == 0 )
      DoChown = 1;
    if( DoChown )
      {
      DEBUG(3,("MAPIEasyLinux : Chown('%s',%i,%i)\n",d, elContext->User.uidNumber, elContext->User.gidNumber));
      if( chown(d, elContext->User.uidNumber, elContext->User.gidNumber) )
        DEBUG(0,("ERROR : MAPIEasyLinux - cannot chown '%s' folder\n",d));
      }
    d[i++] = '/';
    while( d[i++] == '/' );  // Pass trailing '/'
    }  
  }
talloc_unlink(elContext->mem_ctx, d);
mkdir(Path, Mode);
chown(d, elContext->User.uidNumber, elContext->User.gidNumber);
return MAPISTORE_SUCCESS;
}

