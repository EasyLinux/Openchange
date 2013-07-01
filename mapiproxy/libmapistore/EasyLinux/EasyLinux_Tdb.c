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
 * Routines parts for Maildir access
 *
 */

#include "mapiproxy/libmapistore/mapistore.h"
#include "mapiproxy/libmapistore/mapistore_errors.h"
#include "mapiproxy/libmapistore/mapistore_private.h"
#include "mapiproxy/libmapistore/EasyLinux/MAPIStoreEasyLinux.h"
#include <talloc.h>
#include <core/ntstatus.h>
#include <popt.h>
#include <param.h>
#include <util/debug.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/types.h>
#include <dirent.h>

#define isFile 0x8
#define isDir  0x4


/*
 * Open .tdb root file
 *
 * tdb files are used for FALLBACK entries, this function try to open <homedir>/Maildir/FALLBACK/<FID>/<FID>.tdb
 * (if .tdb doesn't exist -> create it)
 */
int OpenFallBack(struct EasyLinuxContext *elContext)
{
char *Path, *FullPath, *Url, *xmlFile; 
int  hFile, i;
DIR *dPath;
struct tdb_context *tFallBack;
TDB_DATA	key, dbuf;


DEBUG(0 ,("MAPIEasyLinux :   Open Fallback\n"));
// Find path and tdb file name
Path     = talloc_asprintf(elContext->mem_ctx,"%s/Maildir/FALLBACK/%s",elContext->User.homeDirectory, elContext->RootFolder.displayName);
xmlFile  = talloc_asprintf(elContext->mem_ctx,"%s/Contexts.xml",elContext->IndexingPath);  
FullPath = talloc_asprintf(elContext->mem_ctx,"%s/%s.tdb",Path, elContext->RootFolder.displayName);
Url      = talloc_asprintf(elContext->mem_ctx,"EasyLinux://FALLBACK/0x%.16lX/",elContext->RootFolder.FID);

// Try to open Path - /home/NET6A/Administrator/Maildir/FALLBACK/Finder.tdb
hFile = open(FullPath, O_RDONLY);
if( hFile == -1 )
  { 
  DEBUG(0, ("MAPIEasyLinux :   --> Create %s \n",Path));
  if( RecursiveMkDir(elContext->User.homeDirectory, Path, elContext->User.uidNumber, elContext->User.gidNumber, 0777) != MAPISTORE_SUCCESS )
    {
    DEBUG(0, ("ERROR - MAPIEasyLinux : can't create %s\n",Path));
    return MAPISTORE_ERR_CONTEXT_FAILED;
    }
  DEBUG(0, ("MAPIEasyLinux :   --> Create %s \n",FullPath));
  tFallBack = tdb_open( FullPath, 0, 0, O_RDWR | O_CREAT, 0660);
  if( tFallBack == NULL )
    {
    DEBUG(0, ("ERROR - MAPIEasyLinux : can't create %s\n",FullPath));
    return MAPISTORE_ERR_CONTEXT_FAILED;
    }
	// Add the record given its fid and mapistore_uri 
	key.dptr = (unsigned char *) talloc_asprintf(elContext->mem_ctx, "0x%lX", elContext->RootFolder.FID);
	key.dsize = strlen((const char *) key.dptr);

	dbuf.dptr = (unsigned char *) talloc_strdup(elContext->mem_ctx, Path);
	dbuf.dsize = strlen((const char *) dbuf.dptr);
	
  DEBUG(0, ("MAPIEasyLinux :   --> AddRecord(0x%lX, %s) \n",elContext->RootFolder.FID, Path));
	tdb_store(tFallBack, key, dbuf, TDB_INSERT);
	
	talloc_free(key.dptr);
	talloc_free(dbuf.dptr);
  
  DEBUG(0, ("MAPIEasyLinux :   --> Close %s \n",Path));
  tdb_close(tFallBack);
  
  // We need to add in Contexts.xml
  DEBUG(0, ("MAPIEasyLinux :   --> Contexts.xml AddContext(%s) \n",elContext->RootFolder.displayName));  
  AddXmlContext( elContext->mem_ctx, xmlFile, elContext->RootFolder.displayName, "true", Url, "tag");
  
  }
else
  { // File exist
  DEBUG(0, ("MAPIEasyLinux :   --> %s exist \n",Path));
  close(hFile);
  }
   

elContext->bkType = EASYLINUX_FALLBACK;
elContext->RootFolder.stType = EASYLINUX_FOLDER;
elContext->RootFolder.RelPath = talloc_asprintf(elContext->mem_ctx,"%s/%s.tdb",elContext->RootFolder.displayName, elContext->RootFolder.displayName);
elContext->RootFolder.FolderType = 0; // FOLDER_ROOT
elContext->RootFolder.FullPath = FullPath;

talloc_unlink(elContext->mem_ctx, xmlFile);
talloc_unlink(elContext->mem_ctx, FullPath);
talloc_unlink(elContext->mem_ctx, Path);
talloc_unlink(elContext->mem_ctx, Url);
return MAPISTORE_SUCCESS;
}





