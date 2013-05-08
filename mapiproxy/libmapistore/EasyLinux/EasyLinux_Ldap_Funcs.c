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

int SetUserInformation(struct EasyLinuxBackendContext *elBackendContext, TALLOC_CTX *mem_ctx, char *User, struct ldb_context *ldb)
{
DEBUG(3, ("MAPIEasyLinux : Searching for user(%s) data's\n",User));
const char *expression = "(uid=%s)";
char *Search;
const char * const Attribs[] = {"unixHomeDirectory","gidNumber","uidNumber","displayName",NULL};
struct ldb_result *resultMsg;
struct ldb_message_element *MessageElement;
int i,j;

Search = talloc_asprintf(mem_ctx,expression,User);

if( LDB_SUCCESS != ldb_search(ldb, mem_ctx, &resultMsg, NULL, LDB_SCOPE_DEFAULT, Attribs, "%s", Search) )
  {
  DEBUG(0, ("ERROR: MAPIEasyLinux - user informations unavailable !\n"));
  return MAPISTORE_ERROR;
  }

DEBUG(5, ("MAPIEasyLinux : %i records returned\n", resultMsg->count));

for (i = 0; i < resultMsg->count; ++i) 
  {
  for( j=0 ; j<resultMsg->msgs[i]->num_elements ; j++)
    {
    MessageElement = &resultMsg->msgs[i]->elements[j];
    if( strcmp(MessageElement->name,"displayName") == 0 )
      elBackendContext->User.displayName = talloc_strdup(mem_ctx, (char *)MessageElement->values[0].data);
    if( strcmp(MessageElement->name,"uidNumber") == 0 )
      elBackendContext->User.uidNumber = atoi( (char *)MessageElement->values[0].data) ;
    if( strcmp(MessageElement->name,"gidNumber") == 0 )
      elBackendContext->User.gidNumber = atoi((char *)MessageElement->values[0].data);
    if( strcmp(MessageElement->name,"unixHomeDirectory") == 0 )
      elBackendContext->User.homeDirectory = talloc_strdup(mem_ctx, (char *)MessageElement->values[0].data);
    }
  }
DEBUG(3, ("MAPIEasyLinux : User (uid:%i gid:%i displayName:%s homeDirectory:%s \n", elBackendContext->User.uidNumber, elBackendContext->User.gidNumber, 
                                    elBackendContext->User.displayName, elBackendContext->User.homeDirectory));
talloc_unlink(mem_ctx, Search);
return MAPISTORE_SUCCESS;
}

/*
 * We need to link uri (64bit FID) to the root folder
 *
 * # record 34
 * dn: CN=1946117988977475585,CN=1513209474796486657,CN=Administrator,CN=First Administrative Group,CN=First Organization,CN=OPENCHANGE,DC=net6a,DC=lan
 * cn: 1946117988977475585
 * FolderType: 1
 * MAPIStoreURI: EasyLinux://INBOX/
 * objectClass: systemfolder
 * PidTagAccess: 63
 * PidTagAttributeHidden: 0
 * PidTagAttributeReadOnly: 0
 * PidTagAttributeSystem: 0
 * PidTagContentCount: 0
 * PidTagContentUnreadCount: 0
 * PidTagCreationTime: 130115550510000000
 * PidTagFolderId: 1946117988977475585
 * PidTagFolderType: 1
 * PidTagParentFolderId: 1513209474796486657
 * PidTagRights: 2043
 * SystemIdx: 13
 * PidTagDisplayName: INBOX
 * PidTagContainerClass: IPF.Note
 * PidTagLastModificationTime: 130115550510000000
 * PidTagChangeNumber: 5837791016979005441
 * PidTagMessageClass: All
 * PidTagMessageClass: IPM
 * PidTagMessageClass: Report.IPM
 * distinguishedName: CN=1946117988977475585,CN=1513209474796486657,CN=Administrator,CN=First Administrative Group,CN=First Organization,CN=OPENCHANGE,DC=net6a,DC=lan
 *
 */
int InitialiseRootFolder(struct EasyLinuxBackendContext *elBackendContext, TALLOC_CTX *mem_ctx, char *User, struct ldb_context *ldb,const char *uri)
{
const char *expression = "(MAPIStoreURI=EasyLinux://%s)";
char *Search, *Folder;
const char * const Attribs[] = {"MAPIStoreURI","PidTagDisplayName","cn",NULL};
struct ldb_result *resultMsg;
struct ldb_message_element *MessageElement;
int i,j;

Search = talloc_asprintf(mem_ctx,expression,uri);

if( LDB_SUCCESS != ldb_search(ldb, mem_ctx, &resultMsg, NULL, LDB_SCOPE_DEFAULT, Attribs, "%s", Search) )
  {
  DEBUG(0, ("ERROR: MAPIEasyLinux - cannot link FID and MAPISToreName\n"));
  return MAPISTORE_ERROR;
  }

DEBUG(3, ("MAPIEasyLinux : InitialiseRootFolder %i records returned for (%s)\n", resultMsg->count, Search));

for (i = 0; i < resultMsg->count; ++i) 
  {
  for( j=0 ; j<resultMsg->msgs[i]->num_elements ; j++)
    {
    MessageElement = &resultMsg->msgs[i]->elements[j];
    if( strcmp(MessageElement->name,"MAPIStoreURI") == 0 )
      elBackendContext->RootFolder.Uri = talloc_strdup(mem_ctx, (char *)MessageElement->values[0].data);
    if( strcmp(MessageElement->name,"PidTagDisplayName") == 0 )
      elBackendContext->RootFolder.displayName = talloc_strdup(mem_ctx, (char *)MessageElement->values[0].data);
    if( strcmp(MessageElement->name,"cn") == 0 )
      elBackendContext->RootFolder.FID = atoll( (char *)MessageElement->values[0].data );
    }
  }
DEBUG(3, ("MAPIEasyLinux : Folder (displayName:%s Uri:%s int64:%lX)\n", elBackendContext->RootFolder.displayName, 
                     elBackendContext->RootFolder.Uri, elBackendContext->RootFolder.FID));
// MAPIEasyLinux : Folder (displayName:Deferred Action Uri:EasyLinux://FALLBACK/0xef03000000000001/ int64:7FFFFFFFFFFFFFFF)

// Remove EasyLinux:// part
i=0;
j=1;
while(j)
  {
  if( (elBackendContext->RootFolder.Uri[i] == '/') && (elBackendContext->RootFolder.Uri[i+1] == '/') )
    {
    j=0;
    i=i+1;
    }
  i++;
  }

if( strncmp ("FALLBACK/", &elBackendContext->RootFolder.Uri[i], 9)==0 )
  {  // mk xml file  '/home/NET6A/Administrator/Maildir/FALLBACK/Deferred Action.xml'
  Folder = talloc_asprintf(mem_ctx,"%s.xml",elBackendContext->RootFolder.displayName);
  CreateXml(elBackendContext, Folder);  // Folder represent rootfolder for libmapistore -> correspond to a xml file under FALLBACK
  }
else if( strncmp("INBOX/", &elBackendContext->RootFolder.Uri[i], 6)==0 )
  {
  DEBUG(0, ("MAPIEasyLinux : mkdir INBOX (%s/Maildir/%s)\n",elBackendContext->User.homeDirectory,&elBackendContext->RootFolder.Uri[i]));  
  }
else
  {
  DEBUG(0, ("MAPIEasyLinux : (%s/Maildir/%s)\n",elBackendContext->User.homeDirectory,&elBackendContext->RootFolder.Uri[i]));  
  }

talloc_unlink(mem_ctx, Search);
return MAPISTORE_SUCCESS;
}
