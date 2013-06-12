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

int SetUserInformation(struct EasyLinuxContext *elContext, TALLOC_CTX *mem_ctx, char *User, struct ldb_context *ldb)
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

elContext->User.uid = talloc_strdup(mem_ctx, User);
for (i = 0; i < resultMsg->count; ++i) 
  {
  for( j=0 ; j<resultMsg->msgs[i]->num_elements ; j++)
    {
    MessageElement = &resultMsg->msgs[i]->elements[j];
    if( strcmp(MessageElement->name,"displayName") == 0 )
      elContext->User.displayName = talloc_strdup(mem_ctx, (char *)MessageElement->values[0].data);
    if( strcmp(MessageElement->name,"uidNumber") == 0 )
      elContext->User.uidNumber = atoi( (char *)MessageElement->values[0].data) ;
    if( strcmp(MessageElement->name,"gidNumber") == 0 )
      elContext->User.gidNumber = atoi((char *)MessageElement->values[0].data);
    if( strcmp(MessageElement->name,"unixHomeDirectory") == 0 )
      elContext->User.homeDirectory = talloc_strdup(mem_ctx, (char *)MessageElement->values[0].data);
    }
  }
if( elContext->User.homeDirectory == NULL )
  {  
  DEBUG(0, ("ERROR: MAPIEasyLinux - user informations unavailable ! No homeDirectory\n"));
  return MAPISTORE_ERROR;
  }
    
DEBUG(3, ("MAPIEasyLinux :     User (uid:%i gid:%i displayName:%s homeDirectory:%s) \n", elContext->User.uidNumber, elContext->User.gidNumber, 
                                    elContext->User.displayName, elContext->User.homeDirectory));
talloc_unlink(mem_ctx, Search);
return MAPISTORE_SUCCESS;
}

/*
 * Initialise root folder
 *
 * A context is linked to somewhat is a Folder, depending on information store,
 * we will use Xml, Ldap, MySql, or wathever needed.
 * Each functionnal system is separated in EasyLinux_<type>.c
 *
 */
int InitialiseRootFolder(struct EasyLinuxContext *elContext, TALLOC_CTX *mem_ctx, char *User, struct ldb_context *ldb,const char *uri)
{
const char *expression = "(MAPIStoreURI=EasyLinux://%s)";
char *Search;
const char * const Attribs[] = {"MAPIStoreURI","PidTagDisplayName","cn",NULL};
struct ldb_result *resultMsg;
struct ldb_message_element *MessageElement;
int i,j;

DEBUG(3,("MAPIEasyLinux : InitialiseRootFolder\n")); 
// We need to find link between URI and FID (64bits)
Search = talloc_asprintf(mem_ctx,expression,uri);
if( LDB_SUCCESS != ldb_search(ldb, mem_ctx, &resultMsg, NULL, LDB_SCOPE_DEFAULT, Attribs, "%s", Search) )
  {
  DEBUG(0, ("ERROR: MAPIEasyLinux - cannot link FID and MAPISToreName - ldb_search\n"));
  return MAPISTORE_ERROR;
  }
if( resultMsg->count == 0 )
  {
  DEBUG(0, ("ERROR: MAPIEasyLinux - cannot link FID and MAPISToreName - No records found!\n"));
  return MAPISTORE_ERROR;
  }
// Extract from record information
for (i = 0; i < resultMsg->count; ++i) 
  {
  for( j=0 ; j<resultMsg->msgs[i]->num_elements ; j++)
    {
    MessageElement = &resultMsg->msgs[i]->elements[j];
    if( strcmp(MessageElement->name,"MAPIStoreURI") == 0 )  // MAPIStoreURI: EasyLinux://INBOX/
      elContext->RootFolder.Uri = talloc_strdup(mem_ctx, (char *)MessageElement->values[0].data);
    if( strcmp(MessageElement->name,"PidTagDisplayName") == 0 )  // PidTagDisplayName: INBOX
      elContext->RootFolder.displayName = talloc_strdup(mem_ctx, (char *)MessageElement->values[0].data);
    if( strcmp(MessageElement->name,"cn") == 0 )  // cn: 1946117988977475585
      elContext->RootFolder.FID = strtoull( (char *)MessageElement->values[0].data, NULL, 10);
    }
  }

// Do whatever we need 
switch( GetBkType(&elContext->RootFolder.Uri[12]) )  // Uri is EasyLinux://INBOX/...
  {
  case EASYLINUX_FALLBACK:
    OpenFallBack(elContext);  // Folder represent rootfolder for libmapistore -> correspond to a xml file under FALLBACK
    break;

  case EASYLINUX_MAILDIR:
    OpenRootMailDir(elContext);
    break;
    
  case EASYLINUX_CALENDAR:  
    OpenCalendar(elContext);
    break;
    
  case EASYLINUX_CONTACTS:
    OpenContacts(elContext);
    break;
    
  case EASYLINUX_TASKS:
    OpenTasks(elContext);
    break;
    
  case EASYLINUX_NOTES:
    OpenNotes(elContext);
    break;
    
  case EASYLINUX_JOURNAL:  
    OpenJournal(elContext);
    break;

  default:
    DEBUG(0, ("MAPIEasyLinux : ERROR '%s' \n", &elContext->RootFolder.Uri[i]));  
    break;
  }
  
elContext->RootFolder.stType = EASYLINUX_FOLDER;  
elContext->RootFolder.Parent = NULL;
elContext->RootFolder.elContext = elContext;
  
//Dump((void *)&elContext->RootFolder);
talloc_unlink(mem_ctx, Search);
return MAPISTORE_SUCCESS;
}

/* 
 * GetUniqueFMID
 *
 * This function is a copy of original openchangedb_get_new_folderID from libproxy/openchangedb.C
 *
 * I extract this function because :
 *    - Function can be implemented directly in the backend
 *    - 
 */
enum MAPISTATUS GetUniqueFMID(struct EasyLinuxContext *elContext, uint64_t *FMID)
{
int			ret;
struct ldb_result	*res;
struct ldb_message	*msg;
const char * const	attrs[] = { "*", NULL };

/* Get the current GlobalCount */
ret = ldb_search(elContext->LdbTable, elContext->mem_ctx, &res, ldb_get_root_basedn(elContext->LdbTable),
			 LDB_SCOPE_SUBTREE, attrs, "(objectClass=server)");
if( ret != LDB_SUCCESS || !res->count )
  return MAPI_E_NOT_FOUND;			 

*FMID = ldb_msg_find_attr_as_uint64(res->msgs[0], "GlobalCount", 0);

/* Update GlobalCount value */
msg = ldb_msg_new(elContext->mem_ctx);
msg->dn = ldb_dn_copy(msg, ldb_msg_find_attr_as_dn(elContext->LdbTable, elContext->mem_ctx, res->msgs[0], "distinguishedName"));
ldb_msg_add_fmt(msg, "GlobalCount", "%llu", (long long unsigned int) ((*FMID) + 1));
msg->elements[0].flags = LDB_FLAG_MOD_REPLACE;
ret = ldb_modify(elContext->LdbTable, msg);
if( ret != LDB_SUCCESS || !res->count )
  return MAPI_E_NOT_FOUND;			 

*FMID = (exchange_globcnt(*FMID) << 16) | 0x0001;

return MAPI_E_SUCCESS;
}


