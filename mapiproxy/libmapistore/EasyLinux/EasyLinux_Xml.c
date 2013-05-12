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
#include <libxml/parser.h>
#include <sys/types.h>
#include <dirent.h>

#define isFile 0x8
#define isDir  0x4

struct XmlBackend {
  xmlDocPtr 	XmlDoc;
  int					NotSaved;   // 0 means Xml file updated,  1 means Xml file need to be updated
  };

/*
 * OpenXml root file
 *
 * Xml files are used for FALLBACK entries, this function try to open <homedir>/Maildir/FALLBACK/<Folder>.xml
 * if XmlFile doesn't exist -> create it
 */
int OpenFallBack(struct EasyLinuxContext *elContext)
{
struct XmlBackend  *elXml;
char *Path, *File;
int  hFile;

File = talloc_asprintf(elContext->mem_ctx,"%s.xml",elContext->RootFolder.displayName);
elContext->bkType = EASYLINUX_FALLBACK;
elContext->RootFolder.stType = EASYLINUX_FOLDER;
elContext->RootFolder.RelPath = talloc_strdup(elContext->mem_ctx, (char *)"/");
elContext->RootFolder.FolderType = 0; // FOLDER_ROOT
elContext->RootFolder.FullPath = talloc_asprintf(elContext->mem_ctx,"%s/Maildir/FALLBACK/%s",elContext->User.homeDirectory, File);

// Store Path and FileName
Path = talloc_asprintf(elContext->mem_ctx,"%s/Maildir/FALLBACK",elContext->User.homeDirectory);
elXml = (struct XmlBackend *)talloc_zero_size(elContext->mem_ctx, sizeof(struct XmlBackend *));
// Store Xml parameters in Backend
elContext->bkContext=(void *)elXml;

// Open Xml File
hFile = open(elContext->RootFolder.FullPath,O_RDONLY);
if( hFile != -1 )
  close(hFile);  // File already exist
else
  {  // First access (auto-provisionning) -> we create file
  DEBUG(0,("MAPIEasyLinux : Create %s\n",Path));
  RecursiveMkDir(elContext,Path,0755);
  CreateXmlFile(elContext);
  }
  
// Store Xml file in memory

talloc_unlink(elContext->mem_ctx, Path);
talloc_unlink(elContext->mem_ctx, File);
return MAPISTORE_SUCCESS;
}

/*
 * Close xml file, if needed save actual memory
 */
int CloseXml(struct EasyLinuxContext *elContext)
{
struct XmlBackend  *elXml;

elXml=(struct XmlBackend  *)elContext->bkContext;


// Close Xml File
if( elXml->NotSaved != 0 )
  {  // Need to be written
  //xmlSaveFormatFileEnc(elContext->RootFolder.FullPath, elXml->XmlDoc, "UTF-8", 1);
  DEBUG(0,("MAPIEasyLinux : Xml file saved !\n"));
  }
// Free memory
//xmlFreeDoc(elXml->XmlDoc);
return MAPISTORE_SUCCESS;
}



int SaveMessageXml(struct EasyLinuxMessage *elMessage, TALLOC_CTX *mem_ctx)
{
DEBUG(0, ("MAPIEasyLinux : Saving XML Message\n"));
/*
xmlDocPtr doc = NULL;       	// document pointer 
xmlNodePtr root_node = NULL;	// node pointers 

// Replace displayName by a more friendly name 

len = strlen(elContext->RootFolder.Uri);
j=0;
for( i=0 ; i<len ; i++ )
  {
  if( elContext->RootFolder.Uri[i] == 'x' )
    {
    if( !first )
      first=1;
    else
      {
      j=i-1;
      i=len;
      }
    }
  i++;
  }
wFID = talloc_strdup(elContext->mem_ctx, &elContext->RootFolder.Uri[j]);

// Majuscule
len = strlen(wFID);
for(i=2 ; i<len ; i++)
  wFID[i] = toupper(wFID[i]);
wFID[i-1] = '\0';

elContext->RootFolder.FID = strtoull(wFID, NULL, 16);

// Creates a new document, a node and set it as a root node
doc = xmlNewDoc(BAD_CAST "1.0");
root_node = xmlNewNode(NULL, BAD_CAST "root");
xmlDocSetRootElement(doc, root_node);

xmlNewChild(root_node, NULL, BAD_CAST "Folder", BAD_CAST elContext->RootFolder.displayName);
xmlNewChild(root_node, NULL, BAD_CAST "Path", BAD_CAST "/");
sFID   = talloc_asprintf(elContext->mem_ctx,"%lu", elContext->RootFolder.FID);
xmlNewChild(root_node, NULL, BAD_CAST "FMID", BAD_CAST sFID);
hFID   = talloc_asprintf(elContext->mem_ctx,"%lX", elContext->RootFolder.FID);
xmlNewChild(root_node, NULL, BAD_CAST "Hex_FMID", BAD_CAST hFID);

 
// Dumping document to file
xmlSaveFormatFileEnc(XmlFile, doc, "UTF-8", 1);
xmlFreeDoc(doc);
xmlCleanupParser();
xmlMemoryDump();

talloc_unlink(elContext->mem_ctx, sFID);
*/
return 0;
}

/*
 * Create an empty Xml file
 */ 
int CreateXmlFile(struct EasyLinuxContext *elContext)
{
xmlDocPtr  doc = NULL;       	// document pointer 
xmlNodePtr root_node = NULL, folder_node = NULL;	// node pointers 
char *sFID, *hFID;

// Creates a new document, a node and set it as a root node
doc = xmlNewDoc(BAD_CAST "1.0");
root_node = xmlNewNode(NULL, BAD_CAST "Root");
xmlDocSetRootElement(doc, root_node);

folder_node = xmlNewChild(root_node, NULL, BAD_CAST "Folder", NULL);
xmlNewChild(folder_node, NULL, BAD_CAST "FullPath", BAD_CAST elContext->RootFolder.FullPath);
xmlSetProp(folder_node, BAD_CAST "Path", BAD_CAST elContext->RootFolder.RelPath);
sFID   = talloc_asprintf(elContext->mem_ctx,"%lu", elContext->RootFolder.FID);
xmlNewChild(folder_node, NULL, BAD_CAST "FMID", BAD_CAST sFID);
hFID   = talloc_asprintf(elContext->mem_ctx,"%lX", elContext->RootFolder.FID);
xmlNewChild(folder_node, NULL, BAD_CAST "Hex_FMID", BAD_CAST hFID);

// Dumping document to file
xmlSaveFormatFileEnc(elContext->RootFolder.FullPath, doc, "UTF-8", 1);
xmlFreeDoc(doc);
xmlCleanupParser();
xmlMemoryDump();

talloc_unlink(elContext->mem_ctx, sFID);
return(0);
}

