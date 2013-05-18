/**
 * MAPIStoreEsayLinux - this file is part of EasyLinux project
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
 * Initialisation routines of OpenChange EasyLinux storage backend.
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

/**
 *  \details Initialize mapistore backend : specific to EasyLinux
 *  NB: there is no BackendUnInitialise, so every connection made, or file open must be closed 
 *  before completion of called function ! mapistore_backend_delete_context only frees allocated memory
 *
 *  \return MAPISTORE_SUCCESS on success
 */
static enum mapistore_error BackendInit (void)
{
// On doit connaitre le serveur Ldap, les éléments de connexion, initialiser la connexion
DEBUG(0,("MAPIEasyLinux : \n"));
DEBUG(0,("MAPIEasyLinux : \n"));
DEBUG(0,("MAPIEasyLinux : BackendInit\n"));
return MAPISTORE_SUCCESS;
}

/*
 * You will notice there is no function to delete a context explicitly within the backend. This is because the mapistore 
 * interface deletes a backend on its own by free'ing the memory context used by this backend.
 *
 * However, backend's developers are required to allocate their internal data with the memory context passed during 
 * create_context and associate a talloc destructor function. If additional operations need to be performed such as 
 * close a file descriptor, socket or call the language specific memory release system (garbage collector, pool etc.)
 *
 * When mapistore will delete the context (free up memory), the hierarchy will automatically be processed and the 
 * destructor function for the backend will be called. This is in this internal function that you need to implement 
 * actions such as close (fd, socket) etc.
 *
 * To create a talloc destructor, call the following function:
 *
 * talloc_set_destructor((void *)context, (int (*)(void *))destructor_fct);
 * 
 * In this example context is the structure allocated with the memory context passed in parameter of the 
 * create_context function. An example of the destructor function would looks like:
 */
static int BackendDestructor(void *Object)
{
// perform operations to clean-up everything properly 


return MAPISTORE_SUCCESS;
}


/**
 * \details Create a connection context to the backend
 *
 * This is the function called when we are creating a context (see Contexts subsection). Its prototype uses the following parameters:
 *
 * \param mem_ctx               TALLOC_CTX 		Pointer to the memory context used for this backend. This is the memory context you need to use 
 *                              							to allocate memory for your specific backend's context structure and for which you may want to set 
 *                              							a destructor.
 * \param *conn_info            struct mapistore_connection_info * 		Pointer to a data structure holding information about the incoming user and 
 *                              							wrapping structures such as the mapistore_context structure pointer. It is needed for a backend for 
 *                              							example to register messages on its own.
 * \param *indexingTdb          struct tdb_wrap * 		Pointer to a wrapped TDB database (sort of hash table) which associates for all folder/messages 
 *                              							an Exchange specific folder and message identifier to a mapistore URI.
 * \param uri                   const char 		Pointer to the URI for this context
 * \param **backend_object      void 					Pointer to a backend object to return and which the backend uses to associate backend's specific information on the context.
 *  
 *   
 */

static enum mapistore_error BackendCreateContext(TALLOC_CTX *mem_ctx,
                            struct mapistore_connection_info *conn_info,
                            struct tdb_wrap *indexingTdb,
                            const char *uri, void **context_object)
{
struct EasyLinuxContext *elContext;
//int Len;

// Initialise Backend structure
elContext = (struct EasyLinuxContext *)talloc_zero_size(mem_ctx, sizeof(struct EasyLinuxContext) );
elContext->stType 			= EASYLINUX_BACKEND;
elContext->mem_ctx  		= mem_ctx;
elContext->Indexing 		= indexingTdb;
elContext->LdbTable 		= conn_info->oc_ctx;
elContext->mstore_ctx 	= conn_info->mstore_ctx;
*context_object 				= (void *)elContext;

if( !conn_info )
  {  // We need connection information to get user and Openchange.ldb pointer
  DEBUG(0,("ERROR : MAPIEasyLinux - CreateContext No Conn Information !\n"));
  return MAPISTORE_ERR_CONTEXT_FAILED;
  }
  
// Retreive user informations from Ldap thru ldbsearch
if( SetUserInformation(elContext, mem_ctx, conn_info->username, conn_info->sam_ctx) != MAPISTORE_SUCCESS)
  return MAPISTORE_ERR_CONTEXT_FAILED;

// Retrieve MAPI litteral Name
if( !conn_info->oc_ctx )  // We need connection information to get user and Openchange.ldb pointer
  DEBUG(0,("ERROR: MAPIEasyLinux - CreateContext No indexing file !\n"));
// Initialise Root Folder (in Easyinux_Ldap.c)  
if( InitialiseRootFolder(elContext, mem_ctx, conn_info->username, conn_info->oc_ctx, uri) != MAPISTORE_SUCCESS)
  return MAPISTORE_ERR_CONTEXT_FAILED;

DEBUG(0, ("MAPIEasyLinux : Context created  Uri:%s (%s)\n",uri, elContext->RootFolder.FullPath ));  
DEBUG(0, ("MAPIEasyLinux :   Len: %i\n",(int)strlen(uri)));

// MAPIStore backend don't include a destructor function capability, we want one 
talloc_set_destructor((void *)elContext, (int (*)(void *))BackendDestructor);
return MAPISTORE_SUCCESS;
}

/**
 * \details Create root folder 
 */
static enum mapistore_error BackendCreateRootFolder (const char *username,
                                 enum mapistore_context_role role,
                                 uint64_t fid, 
                                 const char *name,
                                 TALLOC_CTX *mem_ctx, 
                                 char **mapistore_urip)
{
// MAPISTORE_SUCCESS   MAPISTORE_ERR_NOT_IMPLEMENTED

DEBUG(0, ("MAPIEasyLinux : BackendCreateRootFolder Role(%i) FID(%lX) Name(%s)\n",role, fid, name));
return MAPISTORE_ERR_NOT_IMPLEMENTED;
}

/**
 * \details 
 *
 * This function is part of the auto-provisioning process available in OpenChange 1.0. It is called by 
 * OpenChange server to query the MAPIStore URI for the backend available for specified user. Its prototype 
 * uses the following parameters:
 *
 * \param *username       		const char				Username.  
 * \param *indexingTdb      	struct tdb_wrap 	a pointer to the wrapped TDB database (sort of hash table) which 
 *                           										associates for all folder/messages an Exchange specific folder and message identifier 
 *                           										to a mapistore URI.
 * \param *mem_ctx          	TALLOC_CTX 				Pointer to the memory context used for this backend. This is the memory 
 *                           										context you need to use to allocate memory for the mapistore_contexts_list to return
 * \param **contexts_listp  	struct mapistore_contexts_list 	Pointer of Pointer on a double chained list of 
 *                           										mapistore contexts list information to return
*/
static enum mapistore_error BackendListContexts(const char *username, struct tdb_wrap *indexingTdb,
                           TALLOC_CTX *mem_ctx,
                           struct mapistore_contexts_list **contexts_listp)  // SNOEL contexts_listp  (adresse du pointeur)
{
int rc=MAPISTORE_SUCCESS;  // MAPISTORE_ERR_NO_DIRECTORY  MAPISTORE_SUCCESS   MAPISTORE_ERROR
int i=0;
struct mapistore_contexts_list *Context, *ContextOld, *ContextNew;
char *ContextRole[] = {"INBOX","INBOX/Drafts", "INBOX/Sent", "INBOX/Outbox","INBOX/Trash","CALENDAR","CONTACTS","TASKS",
                     "NOTES","JOURNAL","FALLBACK","MAX"};
char *NameRole[] = {"Inbox","Drafts", "Sent", "Outbox","Trash","Calendar","Contacts","Tasks",
                     "Notes","Journal","Fallback","Max"};
enum mapistore_context_role  Roles[]= {MAPISTORE_MAIL_ROLE, 	MAPISTORE_DRAFTS_ROLE, 	MAPISTORE_SENTITEMS_ROLE,
                     MAPISTORE_OUTBOX_ROLE, MAPISTORE_DELETEDITEMS_ROLE, MAPISTORE_CALENDAR_ROLE,
                     MAPISTORE_CONTACTS_ROLE, MAPISTORE_TASKS_ROLE, MAPISTORE_NOTES_ROLE, MAPISTORE_JOURNAL_ROLE,
                     MAPISTORE_FALLBACK_ROLE, MAPISTORE_MAX_ROLES };

ContextOld = NULL;
ContextNew = NULL;
for( i=0 ; i < 12 ; i++ )
  {
	if( i == 0 )
	  {
  	Context = (struct mapistore_contexts_list *)talloc_zero_size(mem_ctx, sizeof(struct mapistore_contexts_list) );
	  *contexts_listp = Context;
	  }

	Context->url 				 		= talloc_asprintf(mem_ctx,"EasyLinux://%s",ContextRole[i]);
	Context->name				 		= talloc_strdup(mem_ctx, NameRole[i]);
	Context->main_folder 		= true;
	Context->role 					= Roles[i];
	Context->tag						= talloc_strdup(mem_ctx, "tag");
	Context->prev	          = ContextOld;
	
	if( i < 11 )
	  {
		ContextNew = (struct mapistore_contexts_list *)talloc_zero_size(mem_ctx, sizeof(struct mapistore_contexts_list) );
		Context->next					= ContextNew;
		ContextOld						= Context;
		Context								= ContextNew;
		}
	else
		Context->next					= NULL;
	}

DEBUG(3, ("MAPIEasyLinux : BackendListContexts %s done! \n",username));
return rc;
}


/**
 * return the mapistore path associated to a given message or folder ID
 *
 * In MAPIStore, backends can't register FMID to mapistore URI themselves. This is the role of mapistore middleware.
 *
 * To understand this choice, an overview of message creation provides a good example. When a MAPI client creates a 
 * message, it doesn't perform this action through a unique operation. It may even not do it through a single network 
 * transaction. This is different from an IMAP message where the email is composed and pushed at once. In the case of 
 * MAPI, a client will successively call (for a draft email): CreateMessage, SetProps, ModifyRecipients, SaveChangesMessage. 
 * If a client releases the message before it reaches the SaveChangesMessage, the message is destroyed and won't be anymore 
 * available on the system. However, during the message creation process, the message virtually exists and has a MID 
 * associated. It is just that nobody except the client creating the message can access it.
 *
 * From an openchange perspective, it means that the MID exists but is not yet stored within the indexing.tdb database 
 * of the user and is not permanently associated to a mapistore URI:
 *
 *  - When the createmessage's backend function is called, it receives a MID.
 *  - It generates a mapistore URI for this MID
 *  - It stores internally this MID to mapistore URI mapping
 *  - It goes this way through the entire message creation
 *
 * When the message is finally save, it is time to expose the message publicly and reference it. This is where the 
 * context.get_path functions matters:
 *
 *  - The mapistore middleware will register the MID (or FID) within the indexing.tdb of the user
 *  - However it doesn't know the mapistore URI which was associated
 *  - It queries the backend using context.get_path to retrieve the URI associated to the FMID
 *  - It registers the message within the indexing.tdb database
 *
 * You have also figured out that such behavior doesn't only apply to messages but also to folders. When a CreateFolder 
 * is called, the FID is passed to CreateFolder, the backend generates a URI and internally associates it to this FID. 
 * When mapistore middleware registers the folder, it queries context.get_path with the FID and retrieve the URI.
 *
 * This is an indirect way to retrieve data, but it is also useful when a call returns an error etc.
 * 
 * 
 * \param *backend_object
 * \param *mem_ctx
 * \param fmid
 * \param **path
 */
static enum mapistore_error ContextGetPath(void *object, TALLOC_CTX *mem_ctx, uint64_t fmid, char **path)
{
char *Path;
struct EasyLinuxContext *elContext;
struct EasyLinuxGeneric *elGeneric;
struct EasyLinuxFolder  *elFolder;
struct EasyLinuxMessage *elMessage;
struct EasyLinuxTable   *elTable;

// We need to know of wich object Properties belong to
elGeneric = (struct EasyLinuxGeneric *)object;
switch( elGeneric->stType )
  {
  case EASYLINUX_BACKEND:
    // A Rootfolder doesn't have information into indexing.tdb
    DEBUG(0, ("MAPIEasyLinux : ContextGetPath: Context: %lX\n",fmid));

    *path = NULL;
    return MAPISTORE_ERR_INVALID_NAMESPACE;
    break;
   
  case EASYLINUX_FOLDER:
    elFolder = (struct EasyLinuxFolder *)object;
    Path = talloc_strdup(mem_ctx,"?");
    DEBUG(0, ("MAPIEasyLinux : ContextGetPath: Folder: %lX - %s\n",fmid, Path));
    break;
  
  case EASYLINUX_MSG:
    elMessage = (struct EasyLinuxMessage *)object;
    Path = talloc_strdup(mem_ctx,"?");
    DEBUG(0, ("MAPIEasyLinux : ContextGetPath: Message: %lX - %s\n",fmid, Path));
    break;

  case EASYLINUX_TABLE:
    elTable = (struct EasyLinuxTable *)object;
    //break;
    
  default:
    DEBUG(0,("ERROR: MAPIEasyLinux - ContextGetPath: Unknown object type (%i)\n",elGeneric->stType));
    return MAPISTORE_ERR_INVALID_NAMESPACE;
    break;
  }

*path = Path;
return MAPISTORE_SUCCESS;
}

/**
 * \details Get root folder of context
 * mapistore use object concepts. For example you may have noticed that we have divided backend functions from context functions. 
 * Forthcoming documentation also describe folder, message, table or property functions. Each of these subsets of functions 
 * represent an objet. So we have:
 *
 * - a backend object
 * - a context object
 * - a folder object
 * - a message object
 * - etc.
 *
 * When we create a context, we are in fact opening a folder, except that this folder is a root folder. For this very special 
 * case, we are only creating a context but the folder is however opened. It means that a context is also a folder. The get_root_folder 
 * functions returns a folder representation of the context object created. It lets backends directly call folder operations on 
 * contexts rather than having to open (again) the folder to call its operations.
 */
static enum mapistore_error ContextGetRootFolder(void *backend_object, TALLOC_CTX *mem_ctx, uint64_t fid, void **folder_object)  
{
struct EasyLinuxContext *elContext;

elContext = (struct EasyLinuxContext *)backend_object;

DEBUG(0,("MAPIEasyLinux : ContextGetRootFolder %lX - %s\n",elContext->RootFolder.FID, elContext->RootFolder.displayName));
*folder_object = &elContext->RootFolder;

return MAPISTORE_SUCCESS;
}

/**
 * \details Open a folder 
 * This function opens a folder in the backend. It should only perform an action when the folder is not a 
 * root/mapistore folder (referenced in openchange.ldb), since these specific root folders are opened during 
 * context creation and morphed/returned as a folder object when context.get_root_folder returns.
 * For any folders within a backend and different from the root folder, the folder should be opened. The 
 * function takes in parameter:
 *
 * \param *parent_folder    		void  				the parent folder object of the folder to open
 * \param *mem_ctx          		TALLOC_CTX  	the memory context to allocate folder_object with
 * \param fir 									uint64_t  		the folder identifier of the folder to open
 * \param **childfolder_object  void 					the opened folder object to return
 *
 * Folders are opened within a given context, which folder object is retrieved through context.get_root_folder call. 
 * While this specifically apply to folder underneath the root folder, the same logic apply to sub folders which parent 
 * is not the root folder object within the context. It requires either to keep a reference to the root folder along 
 * newly created folder objects or be able to walk through the associated MAPIStore URI and figure out what the root folder URI is.
 *
 * \return MAPISTORE_SUCCESS on success, otherwise MAPISTORE_ERROR
 */
static enum mapistore_error FolderOpen(void *parent_folder, TALLOC_CTX *mem_ctx, uint64_t fid, void **childfolder_object)
{
int rc=MAPISTORE_ERR_NOT_IMPLEMENTED;
DEBUG(0, ("MAPIEasyLinux : FolderOpen\n"));
return rc;
}

/**
 * \details Create a folder in the sogo backend
 * This function only creates a folder within a existing context. The resulting created folder will have a 
 * URI using the same backend that the context it is using. Upon success, the folder is opened and returned 
 * within the void ** parameter. The function takes in parameter:

 * \param *parent_folder    void 					the parent folder object of the folder to be created
 * \param *mem_ctx          TALLOC_CTX  	the memory context to use to allocate the new folder upon success
 * \param fid               uint64_t 			the folder identifier for this folder
 * \param *aRow						  struct SRow 	an Exchange data structures which must store PR_DISPLAY_NAME or 
 *																				PR_DISPLAY_NAME_UNICODE (the folder name to be created)
 * \param **childfolder			void					the folder object for the newly created folder
 *
 * The SRow structure may also supply the following Exchange properties:
 *
 * - PR_COMMENT or PR_COMMENT_UNICODE: the folder description
 * - PR_PARENT_FID: the folder identifier of the parent folder object
 * - PR_CHANGE_NUM: the new change number
 * 
 * \return MAPISTORE_SUCCESS on success, otherwise MAPISTORE_ERROR
 
struct SRow {
	uint32_t ulAdrEntryPad;
	uint32_t cValues;							// [range(0,100000)] 
	struct SPropValue *lpProps;		// [unique,size_is(cValues)] 
}																// [noprint,nopush,public,nopull] ;

struct SPropValue {
	enum MAPITAGS ulPropTag;
	uint32_t dwAlignPad;
	union SPropValue_CTR value;		// [switch_is(ulPropTag&0xFFFF)] 
}																// [noprint,nopush,public,nopull] ;



 */
static enum mapistore_error FolderCreate(void *parent_folder, TALLOC_CTX *mem_ctx, uint64_t fid, struct SRow *aRow, void **childfolder)
{
int rc=MAPISTORE_ERR_NOT_IMPLEMENTED, i;
struct EasyLinuxFolder *elFolder, *elNewFolder;

elFolder = (struct EasyLinuxFolder *)parent_folder;
elNewFolder = (struct EasyLinuxFolder *)talloc_zero_size(mem_ctx, sizeof(struct EasyLinuxFolder));
*childfolder = (void *)elNewFolder;

DEBUG(0, ("MAPIEasyLinux : FolderCreate\n"));
// On a besoin de :
switch( elFolder->elContext->bkType )
  {
  case EASYLINUX_FALLBACK:
    //*rowCount = XmlCreateFolder(elFolder, table_type);
    rc=MAPISTORE_ERR_NOT_IMPLEMENTED;
    break;
    
  case EASYLINUX_MAILDIR:
    DEBUG(0, ("MAPIEasyLinux :   FID: 0x%lX\n",fid));
    DEBUG(0, ("MAPIEasyLinux :   EntryPad: %X  Values: %i\n",aRow->ulAdrEntryPad, aRow->cValues));
    for(i=0 ; i<aRow->cValues ; i++)
      {
      switch( aRow->lpProps[i].ulPropTag )
        {
        case 0x36010003:  // PidTagFolderType 0x36010003   -> int FOLDER_GENERIC=0x1, FOLDER_SEARCH=0x2
          elNewFolder->FolderType = (int)aRow->lpProps[i].value.l;
          break;
        
        case 0x3001001E: // PidTagDisplayName_string8 0x3001001E 
          elNewFolder->displayName = talloc_strdup(mem_ctx,(char *)aRow->lpProps[i].value.lpszA);
          DEBUG(0, ("MAPIEasyLinux :   DisplayName  %s\n", elNewFolder->displayName));
          break;
          
        case 0x3004001E: // PidTagComment_string8 0x3004001E  
          elNewFolder->Comment = talloc_strdup(mem_ctx,(char *)aRow->lpProps[i].value.lpszA);
          break;
          
        case 0x67490014: // PidTagParentFolderId 0x67490014
          break;
          
        case 0x67A40014: // PidTagChangeNumber 0x67A40014 
          break;

        default:
          DEBUG(0, ("MAPIEasyLinux :   PropTag[%i]: 0x%X\n", i, (int)aRow->lpProps[i].ulPropTag));
          break;
        }
      }
    elNewFolder->stType=EASYLINUX_FOLDER;
	  elNewFolder->Uri = talloc_asprintf(mem_ctx,"EasyLinux://%s",elNewFolder->displayName);
	  elNewFolder->FID=fid;
	  elNewFolder->RelPath  = ImapToMaildir(mem_ctx, elNewFolder->displayName);
	  elNewFolder->FullPath = talloc_asprintf(mem_ctx, "%s%s/", elFolder->FullPath, elNewFolder->RelPath);
	  elNewFolder->Parent   = elFolder;
	  elNewFolder->elContext = elFolder->elContext;
      
    Dump(elNewFolder);
    Dump(elFolder);
    rc = MailDirCreateFolder(elNewFolder);
    break;

  default:
    DEBUG(0,("ERROR: MAPIEasyLinux - %i invalid type of backend !\n", elFolder->elContext->bkType));
    break;
  }

return rc;
}


/**
   \details Delete a folder in current context

   \param private_data pointer to the current sogo context
   \param parent_fid the FID for the parent of the folder to delete
   \param fid the FID for the folder to delete

   \return MAPISTORE_SUCCESS on success, otherwise MAPISTORE_ERROR
*/
static enum mapistore_error FolderDelete(void *folder_object)
{
int rc=MAPISTORE_ERR_NOT_IMPLEMENTED;
DEBUG(0, ("MAPIEasyLinux : FolderDelete\n"));
return rc;
}

/**
 * This function retrieves either the number of messages, FAI messages or folders within specified folder. The function takes in parameters:
 *
 * \param *folder_object	  void				the folder on which the count has to be retrieved
 * \param table_type  			uint8_t			the type of item to retrieve. Possible values are listed below
 * \param *rowCount  				uint32_t 		the number of elements retrieved
 *
 * Possible values for table_type are:
 *   - 2 MAPISTORE_MESSAGE_TABLE: the number of messages within the folder
 *   - 3 MAPISTORE_FAI_TABLE: the number of FAI messages within the folder
 *   - 1 MAPISTORE_FOLDER_TABLE: the number of folders within the folder
 */
static enum mapistore_error FolderGetChildCount(void *folder_object, uint32_t table_type, uint32_t *rowCount)
{
int rc=MAPISTORE_SUCCESS;
struct EasyLinuxFolder *elFolder;

elFolder = (struct EasyLinuxFolder *)folder_object;
// On a besoin de :
switch( elFolder->elContext->bkType )
  {
  case EASYLINUX_FALLBACK:
    //*rowCount = GetXmlChildCount(elFolder, table_type);
    DEBUG(0, ("MAPIEasyLinux : FolderGetChildCount for '%s' (%i)\n", elFolder->FullPath, *rowCount));
    *rowCount = 1;
    break;
    
  case EASYLINUX_MAILDIR:
    *rowCount = GetMaildirChildCount(elFolder, table_type);
    break;

	case EASYLINUX_CALENDAR:
	  *rowCount = GetCalendars(elFolder);
	  break;
	  
	case EASYLINUX_CONTACTS:
	  *rowCount = GetContacts(elFolder);
	  break;
	
	case EASYLINUX_TASKS:
	  *rowCount = GetTasks(elFolder);
	  break;
	  
	case EASYLINUX_NOTES:
	  *rowCount = GetNotes(elFolder);
	  break;
	  
	case EASYLINUX_JOURNAL:
	  *rowCount = GetJournal(elFolder);
	  break;

  default:
    DEBUG(0,("ERROR: MAPIEasyLinux - FolderGetChildCount %i invalid type of backend !\n", elFolder->elContext->bkType));
    break;
  }


return rc;
}

/**
This function opens a message and returns a message object. This function takes in parameters:

    void *folder_object: the parent folder object of the message to open
    TALLOC_CTX *: the memory context to use to allocate the message to be opened
    uint64_t mid: the message identifier of the message to open
    void **message_object: the message object to return

*/
static enum mapistore_error FolderOpenMessage(void *folder_object,
                         TALLOC_CTX *mem_ctx,
                         uint64_t mid, bool write_access,
                         void **message_object)
{
int rc=MAPISTORE_ERR_NOT_IMPLEMENTED;
DEBUG(0, ("MAPIEasyLinux : FolderOpenMessage\n"));
return rc;
}

/**
 * \details Deliver a temporary message object
 *
 * This function creates a temporary message object. Messages are a bit particular in Exchange, 
 * because they are not created through a single transaction. Creating message is the first operation 
 * of the process, but message doesn't get available/saved or sent until you respectively call save or 
 * submit backend methods. Between create and save/submit, MAPI clients can perform several operations such as:
 *
 * - set or delete properties
 * - add or delete attachments
 * - add or delete recipients
 *
 * Until save/submit is called, this message object remains virtual. However MAPI clients need a way to reference 
 * this object within current context until it's saved. This is why OpenChange generates a MID (message identifier) 
 * and requests the backend to temporarily create an associated MAPIStore URI. But this MID/MAPIStore URI couple is 
 * not stored within the user idexing.tdb database.
 *
 * Remember that mapistore calls backend with MID/FID and expect the backend to lookup associated mapistore URI.
 *
 * It is the backend's responsibility to virtual store this message until save/submit is called. If no such mechanism/API 
 * exist in your backend or if you want to speed up the process, you can use the ocpf (http://apidocs.openchange.org/libocpf/index.html) 
 * library which will store everything for you properly.
 * The function takes in parameter:
 *
 * \param 	*parent_folder		void					the parent folder object in which the message has to be created
 * \param   *mem_ctx					TALLOC_CTX 		the memory context to use to create the message object
 * \param   mid 					   	uint64_t 			the message identifier for this new message
 * \param  	associated				uint8_t 			whether this message is already associated to a file or not (see below)
 * \param   **message_object 	void 					the message object to return
 *
 * Regarding the associated parameter, it is a flag that tells the backend if it already has an associated 
 * file for this message or not. Some background is required to understand this concept:
 *
 * When you create a message, you will set properties for the message and finally save it. However, you may 
 * not have a 1 to 1 mapping between MAPI properties and what the remote system supports. Many properties are 
 * Exchange specific and your remote system may not have associated parameters for them.
 *
 * Here is the crucial part: there is a small amount of information that a remote system is unlikely to map/support 
 * but which ARE REQUIRED for the message or folder to be valid/fetched properly by MAPI clients.
 *
 * While you can skip/drop many Exchange properties, you CAN'T skip/drop the required ones.
 *
 * The associated parameter lets your backend know if it already has a file or structure associated to this message 
 * in which it stores exchange properties it can't map.
 */
static enum mapistore_error FolderCreateMessage(void *parent_folder,
                           TALLOC_CTX *mem_ctx,
                           uint64_t mid,
                           uint8_t associated,
                           void **message_object)
{
struct EasyLinuxFolder  *elFolder;
struct EasyLinuxMessage *elMessage;

elFolder  = (struct EasyLinuxFolder  *)parent_folder;
elMessage = (struct EasyLinuxMessage *)talloc_zero_size(mem_ctx, sizeof(struct EasyLinuxMessage) );

elMessage->stType = EASYLINUX_MSG;
elMessage->MID = mid;
elMessage->associated = associated;
elMessage->Parent = elFolder;
elMessage->elContext = elFolder->elContext;
elMessage->Table = NULL;

*message_object = (void *)elMessage;

DEBUG(0, ("MAPIEasyLinux : FolderCreateMessage - MID: 0x%0lX\n", mid));
return MAPISTORE_SUCCESS;
}

/**
 *
 * The delete_message operation deletes a message given its message identifier within a given folder. 
 * This function takes in parameter:
 *
 * \param			*folder-object 		void 				the folder object in which the message to delete is stored
 * \param  		mid								uint64_t 		the message identifier representing the message to delete
 * \param 		flags							uint8_t 		flags that indicate the kind of deletion
 *
 * Possible deletion flags are:
 *    - MAPISTORE_SOFT_DELETE: to virtually delete the message. 
 *      The message is not physically deleted on the remote system, it is just marked as SOFT_DELETED 
 *      in the indexing database of the user. It can be recovered at any time.
 */
static enum mapistore_error FolderDeleteMessage(void *folder_object, uint64_t mid, uint8_t flags)
{
int rc=MAPISTORE_ERR_NOT_IMPLEMENTED;
DEBUG(0, ("MAPIEasyLinux : FolderDeleteMessage\n"));
return rc;
}

/**

*/
static enum mapistore_error FolderMoveCopyMessages(void *folder_object,
                               void *source_folder_object,
                               TALLOC_CTX *mem_ctx,
                               uint32_t mid_count,
                               uint64_t *src_mids, uint64_t *t_mids,
                               struct Binary_r **target_change_keys,
                               uint8_t want_copy)
{
int rc=MAPISTORE_ERR_NOT_IMPLEMENTED;
DEBUG(0, ("MAPIEasyLinux : FolderMoveCopyMessages\n"));
return rc;
}

/**

*/
static enum mapistore_error FolderMove(void *folder_object, void *target_folder_object,
                        TALLOC_CTX *mem_ctx, const char *new_folder_name)
{
int rc=MAPISTORE_ERR_NOT_IMPLEMENTED;
DEBUG(0, ("MAPIEasyLinux : FolderMoveFolder\n"));
return rc;
}

/**

*/
static enum mapistore_error FolderCopy(void *folder_object, void *target_folder_object, TALLOC_CTX *mem_ctx,
                        bool recursive, const char *new_folder_name)
{
int rc=MAPISTORE_ERR_NOT_IMPLEMENTED;
DEBUG(0, ("MAPIEasyLinux : FolderCopyFolder\n"));
return rc;
}

/**

*/
static enum mapistore_error FolderGetDeletedFmids(void *folder_object, TALLOC_CTX *mem_ctx,
                              enum mapistore_table_type table_type, uint64_t change_num,
                              struct UI8Array_r **fmidsp, uint64_t *cnp)
{
int rc=MAPISTORE_ERR_NOT_IMPLEMENTED;
DEBUG(0, ("MAPIEasyLinux : FolderGetDeletedFmids\n"));
return rc;
}

/**
This function creates a table object to be used along with backend's table operations. The function takes in parameter:
- void *folder_object: the folder on which the table has to be created
- TALLOC_CTX *: the memory context to use to create the table
- uint8_t table_type: the table type to create. See below for possible table_type valuesFolder.displayName
- uint32_t handle_id: Exchange temporary id for the object. Used for the moment for notifications on table
- void **table_object: the table object to return
- uint32_t *row_count: the number of row of the table to be returned. It is used by OpenChange server and returned 
  clients, so they know how many rows are available and how much they can query.

Possible values for table_type are:
- MAPISTORE_MESSAGE_TABLE: creates a table which only lists messages
- MAPISTORE_FAI_TABLE: creates a table which only lists FAI messages
- MAPISTORE_FOLDER_TABLE: creates a table which only lists folders
- MAPISTORE_PERMISSION_TABLE: creates a table which list permissions for the folder

For the handle_id, it is a hackish implementation from SOGo. It is used to process notifications on 
tables when registered by clients. This number is associated with the table objects and the couple 
is used to find a table and trigger a notification payload upon table changes.

The handling of handle_id can be avoided for now as it is likely to change with OCSManager final implementation.

OpenChange is waiting for a table_object (void **) which it doesn't care about, but will pass it to your 
backend when a new table operation occurs. It is also waiting for the number of rows in the table (uint32_t *row_count)
*/
static enum mapistore_error FolderCreateTable(void *folder_object, TALLOC_CTX *mem_ctx,
                       enum mapistore_table_type table_type, uint32_t handle_id,
                       void **table_object, uint32_t *row_count)
{
struct EasyLinuxFolder *elFolder;

elFolder = (struct EasyLinuxFolder *)folder_object;
elFolder->Table = talloc_zero_size(mem_ctx, sizeof(struct EasyLinuxTable));

if( elFolder->Table == NULL )
  {
  DEBUG(0,("ERROR: MAPIEasyLinux - Cannot allocate memory for table\n"));
  return MAPISTORE_ERROR;
  }

*table_object = (void *)elFolder->Table;

elFolder->Table->stType        = EASYLINUX_TABLE;
elFolder->Table->rowCount      = 0;
// elTable->IdTable       = handle_id;  // Sogo specific
elFolder->Table->tType         = table_type;
// Link Table and Folder together
elFolder->Table->elParent = (void *)folder_object;
elFolder->Table->elContext     = elFolder->elContext;
elFolder->Table->Props = NULL;

*row_count             = elFolder->Table->rowCount;

DEBUG(0, ("MAPIEasyLinux : FolderCreateTable - Creer table\n"));
return MAPISTORE_SUCCESS;
}

/**

*/
static enum mapistore_error FolderModifyPermissions(void *folder_object, uint8_t flags,
                               uint16_t pcount,
                               struct PermissionData *permissions)
{
int rc=MAPISTORE_ERR_NOT_IMPLEMENTED;
DEBUG(0, ("MAPIEasyLinux : FolderModifyPermissions\n"));
return rc;
}

/**

*/
static enum mapistore_error FolderPreloadMessageBodies(void *folder_object, enum mapistore_table_type table_type, const struct UI8Array_r *mids)
{
int rc=MAPISTORE_ERR_NOT_IMPLEMENTED;
DEBUG(0, ("MAPIEasyLinux : FolderPreloadMessageBodies\n"));
return rc;
}

/**

*/
static enum mapistore_error MessageGetMessageData(void *message_object,
                              TALLOC_CTX *mem_ctx,
                              struct mapistore_message **msg_dataP)
{
int rc=MAPISTORE_ERR_NOT_IMPLEMENTED;
DEBUG(0, ("MAPIEasyLinux : MessageGetMessageData\n"));
return rc;
}

/**
*/
static enum mapistore_error MessageCreateAttachment (void *message_object, TALLOC_CTX *mem_ctx, void **attachment_object, uint32_t *aidp)
{
int rc=MAPISTORE_ERR_NOT_IMPLEMENTED;
DEBUG(0, ("MAPIEasyLinux : MessageCreateAttachment\n"));
return rc;
}

/**
This operation opens an attachment and takes in parameters:

    void *message_object: the message object the attachment belongs to
    TALLOC_CTX *: the memory context to use to allocate the attachment object upon success
    uint32_t aid: the attachment ID number referenced by message.get_attachment_table call
    void **attachment_object: the attachment object to return

Attachments behaves like messages within a folder: To retrieve messages we first retrieve a content table on the folder object, then query the table for rows and use MID to identify messages.

For attachments MID are replaced with an attachment ID which identifies the index of the attachment within the message object.
This attachment ID is finally referred and returned by OpenChange server as the PidTagAttachNumber MAPI property.

Opening an attachment means to retrieve an attachment stored on a message object and referenced by its attachment ID.
*/
static enum mapistore_error MessageOpenAttachment (void *message_object, TALLOC_CTX *mem_ctx,
                              uint32_t aid, void **attachment_object)
{
int rc=MAPISTORE_ERR_NOT_IMPLEMENTED;
DEBUG(0, ("MAPIEasyLinux : MessageOpenAttachment\n"));
return rc;
}

/**
This operation creates a table object to be used along with table operations. The function takes in parameters:

    void *message_object: the message on which the table has to be created
    TALLOC_CTX *: the memory context to use to create the table
    void **table_object: the table object to return
    uint32_t *row_count: pointer on the total number of attachments the table handles for this message

*/
static enum mapistore_error MessageGetAttachmentTable (void *message_object, TALLOC_CTX *mem_ctx, void **table_object, uint32_t *row_count)
{
int rc=MAPISTORE_ERR_NOT_IMPLEMENTED;
DEBUG(0, ("MAPIEasyLinux : MessageGetAttachmentTable\n"));
return rc;
}

/**
This functions adds delete or modify recipients on a message. The function takes in parameter:

    void *message_object: the message object on which to change recipients
    struct SPropTagArray *columns: the properties to lookup for each recipients specified with struct ModifyRecipientRow *
    struct ModifyRecipientRow *recipients: an array of recipients structure holding recipients information. See below for detailed explanation
    uint16_t count: the number of recipients elements in struct ModifyRecipientRow

Generally your backend will temporary maintain a list of recipients. modify_recipients calls are performed whenever something need to be changed within your internal list.
The column parameter defines a set of MAPI properties which are packed for each recipients referenced by the recipients structure.
For each recipient in struct ModifyRecipientRow *, we receive:

    idx the index within the array
    RecipClass: the action to perform for this recipient which can be:
        MODRECIP_NULL: do nothing
        MODRECIP_INVALID: invalid user
        MODRECIP_ADD: add the user
        MODRECIP_MODIFY: replace what has already been saved for the user with this content
        MODRECIP_REMOVE: remove the user from the list.
    RecipientRow: a complex structure which values are set or not depending on its RecipientFlags parameter. The function also provides a prop_value DATA blob where values from properties defined in struct SPropTagArray *columns are packed one to next over.

*/
static enum mapistore_error MessageModifyRecipients (void *message_object,
                                struct SPropTagArray *columns,
                                uint16_t count,
                                struct mapistore_message_recipient *recipients)
{
int rc=MAPISTORE_ERR_NOT_IMPLEMENTED;
DEBUG(0, ("MAPIEasyLinux : MessageModifyRecipients\n"));
return rc;
}

/**

*/
static enum mapistore_error MessageSetReadFlag (void *message_object, uint8_t flag)
{
int rc=MAPISTORE_ERR_NOT_IMPLEMENTED;
DEBUG(0, ("MAPIEasyLinux : MessageSetReadFlag  Message Id     flag %x\n",flag));
    //folders that are containers and which are of the mstoredb kind.
    //folders that are actually heavily used such as Inbox, Calendar, Outbox etc and which can be of any backend type.
return rc;
}

/**
 *
 * This operation saves a message on remote/local storage system to your backend handles. It takes all the temporary information 
 * associated to the message that has been modified/created and dump replicate the change/creation on the remote system. 
 * This function takes in parameter the message object. This is for example used when you create or edit a calendar, 
 * note, task or draft item.
*/
static enum mapistore_error MessageSave(void *message_object, TALLOC_CTX *mem_ctx)
{
int rc=MAPISTORE_SUCCESS;
struct EasyLinuxMessage *elMessage;

elMessage = message_object;
// On a besoin de :
switch( elMessage->elContext->bkType )
  {
  case EASYLINUX_FALLBACK:
    SaveMessageXml(elMessage, mem_ctx);
    break;
    
  default:
    DEBUG(0,("ERROR: MAPIEasyLinux - MessageSave %i invalid type of backend !\n", elMessage->elContext->bkType));
    break;
  }
    
return rc;
}

/**
This operation push a message for dispatch. It only apply to emails or appointment invitation. The client 
expects the message to be sent to the spooler and dispatched to specified recipients.
The function takes in parameters:

    void *message_object: the message object to submit
    enum SubmitFlags: unused so far

*/
static enum mapistore_error MessageSubmit (void *message_object, enum SubmitFlags flags)
{
int rc=MAPISTORE_ERR_NOT_IMPLEMENTED;
DEBUG(0, ("MAPIEasyLinux : MessageSubmit\n"));
return rc;
}

/**
This operation opens an attachment as a message. Available operations on message applies to this object once returned.
The function takes in parameters:

    void *attachment_object: The attachment object representing this message object
    TALLOC_CTX *: the memory context to use to allocate the message object
    void **message_object: the message object to return
    uint64_t *mid: pointer to the message identifier matching the message to return
    struct mapistore_message **msg: pointer on pointer to a mapistore_message structure which holds message and recipients information about the message.

*/
static enum mapistore_error MessageAttachmentOpenEmbeddedMessage (void *attachment_object,
                                               TALLOC_CTX *mem_ctx,
                                               void **message_object,
                                               uint64_t *midP,
                                               struct mapistore_message **msg)
{
int rc=MAPISTORE_ERR_NOT_IMPLEMENTED;
DEBUG(0, ("MAPIEasyLinux : MessageAttachmentOpenEmbeddedMessage\n"));
return rc;
}

/**

*/
static enum mapistore_error MessageAttachmentCreateEmbeddedMessage (void *attachment_object,
                                                 TALLOC_CTX *mem_ctx,
                                                 void **message_object,
                                                 struct mapistore_message **msg)
{
int rc=MAPISTORE_ERR_NOT_IMPLEMENTED;
DEBUG(0, ("MAPIEasyLinux : MessageAttachmentCreateEmbeddedMessage\n"));
return rc;
}

/**
table.get_available_properties

This function retrieves all the properties available for the table given the type of objects it handles. It doesn't relate to table.set_columns and the set of properties it set, but instead return a generic list of properties that can potentially be set on the given object type the table handles: message, folder, fai, permissions.
The function takes in parameters:

    void *table_object: the table object
    TALLOC_CTX *: the memory context to allocate the struct SPropTagArray to return
    struct SPropTagArray **properties: a pointer on pointer to the list of available properties to return

*/
static enum mapistore_error TableGetAvailableProperties(void *table_object,
                                               TALLOC_CTX *mem_ctx, struct SPropTagArray **propertiesP)
{
int rc=MAPISTORE_ERR_NOT_IMPLEMENTED;
DEBUG(0, ("MAPIEasyLinux : TableGetAvailableProperties\n"));
return rc;
}

/*
table.set_columns
This function sets the columns we want to retrieve when querying for table objects row. The function takes in parameters:

    void *table_object: the table object on which we want to set columns
    uint16_t count: the number of enum MAPITAGS *properties to be set
    enum MAPITAGS *properties: an array of MAPITAGS to be used to set columns
*/
static enum mapistore_error TableSetColumns (void *table_object, uint16_t count, enum MAPITAGS *properties)
{
//struct EasyLinuxTable *elTable;


return MAPISTORE_ERR_NOT_IMPLEMENTED;
}

/*
 *
 * set_restrictions
 * 
 * This function sets one or more filters on the table. When the table is queried again through get_row or get_row_count, 
 * filters will get applied and only filtered results returned.
 * The function takes in parameters:
 *
 *  void *object: the table object restrictions apply on
 *  struct mapi_SRestriction *restrictions: the restrictions to apply
 *  uint8_t *table_status: the status of the table. In theory, we should be able to return from the set_restriction 
 *                         even if the operation didn't yet complete (asynchronous mechanism). In practice, we just do the operation 
 *                         synchronously and return TBLSTAT_COMPLETE if the restrictions applied properly.
 *
 * Regarding mapi_SRestriction, this structure defined a filter in Exchange world and should be detailed on the wiki. 
 * This is however already covered by Exchange specifications in [MS-OXCDATA] section 2.12.
 
struct mapi_SRestriction {
	uint8_t rt;
	union mapi_SRestriction_CTR res;   // [switch_is(rt)] 
}// [public,flag(LIBNDR_FLAG_NOALIGN)] ;

struct mapi_SPropertyRestriction {
	uint8_t relop;
	enum MAPITAGS ulPropTag;
	struct mapi_SPropValue lpProp;
}; [flag(LIBNDR_FLAG_NOALIGN)] 

struct mapi_SPropValue {
	enum MAPITAGS ulPropTag;
	union mapi_SPropValue_CTR value;  // [switch_is(ulPropTag&0xFFFF)] 
}; // [public,flag(LIBNDR_FLAG_NOALIGN)]
 
 *
 */
static enum mapistore_error TableSetRestrictions (void *table_object, struct mapi_SRestriction *restrictions, uint8_t *table_status)
{
char     *sRes, *sMsg, *sFilter;
TALLOC_CTX *mem_ctx;
struct EasyLinuxTable *elTable;

//DEBUG(0,("MAPIEasyLinux : TableSetRestrictions rt:%i relop:%i ulPropTag: 0x%08X ulPropTag: 0x%08X\n", restrictions->rt, ));

elTable = (struct EasyLinuxTable *)table_object;
mem_ctx= elTable->elContext->mem_ctx;

switch(restrictions->rt)
  {
  case RES_AND:
    sRes = talloc_strdup(mem_ctx, "RES_AND");
    break;
  case RES_OR:
    sRes = talloc_strdup(mem_ctx, "RES_OR");
    break;
  case RES_CONTENT:
    sRes = talloc_strdup(mem_ctx, "RES_CONTENT");
    break;
  case RES_PROPERTY:
    sRes = talloc_strdup(mem_ctx, "RES_PROPERTY");
    sMsg = talloc_asprintf(mem_ctx,"relop:%X - ulPropTag: %X",restrictions->res.resProperty.relop,restrictions->res.resProperty.ulPropTag);
    // relop 4 => EQUAL
    
    switch( restrictions->res.resProperty.ulPropTag )
      {
      case 0x0037001F:  // PidTagSubject 0x0037001F 
        DEBUG(0,("MAPIEasyLinux : TableSetRestrictions: Subject=%s\n", (char *)restrictions->res.resProperty.lpProp.value.lpszA ));
        sFilter = talloc_asprintf(mem_ctx,"Subject=%s",(char *)restrictions->res.resProperty.lpProp.value.lpszA);
        //elTable->Filter = sFilter;
        break;
        
      case 0x3001001F:  // PidTagDisplayName  0x3001001F
        DEBUG(0,("MAPIEasyLinux : TableSetRestrictions: DisplayName=%s\n", (char *)restrictions->res.resProperty.lpProp.value.lpszA ));
        sFilter = talloc_asprintf(mem_ctx,"DisplayName=%s",(char *)restrictions->res.resProperty.lpProp.value.lpszA);
        //elTable->Filter = sFilter;
        break;
            
      default:
        DEBUG(0,("MAPIEasyLinux : TableSetRestrictions: Table RES_PROPERTY 0x%X\n",restrictions->res.resProperty.ulPropTag));
        break;
      }
    break;
  case RES_COMPAREPROPS:
    sRes = talloc_strdup(mem_ctx, "RES_COMPAREPROPS");
    break;
  case RES_BITMASK:
    sRes = talloc_strdup(mem_ctx, "RES_BITMASK");
    break;
  case RES_SIZE:
    sRes = talloc_strdup(mem_ctx, "RES_SIZE");
    break;
  case RES_EXIST:
    sRes = talloc_strdup(mem_ctx, "RES_EXIST");
    break;
  default:
    sRes = talloc_strdup(mem_ctx, "Inconnu");
    DEBUG(0, ("MAPIEasyLinux : TableSetRestrictions (rt: %i) Type: %s \n",restrictions->rt, sRes));
    break;
  }
 	
*table_status = TBLSTAT_COMPLETE;


talloc_unlink(mem_ctx, sRes);
talloc_unlink(mem_ctx, sMsg);

DEBUG(0, ("MAPIEasyLinux : TableSetRestrictions (rt: %i)\n",restrictions->rt));
return MAPISTORE_SUCCESS;
}

/*
table.set_sort_order
This function set the sorting order depending on the specified struct SSortOrderSet *sort_order. This function takes in parameters:

    void *table_object: the table object on which sorting applies
    struct SSortOrderSet *sort_oder: the sort to apply to the table
    uint8_t *table_status: the table status to return (TBLSTAT_COMPLETE, see above subsection).

*/
static enum mapistore_error TableSetSortOrder (void *table_object, struct SSortOrderSet *sort_order, uint8_t *table_status)
{
int rc=MAPISTORE_ERR_NOT_IMPLEMENTED;
DEBUG(0, ("MAPIEasyLinux : TableSetSortOrder\n"));
return rc;
}

/*
table.get_row
The function takes in parameters:

    void *table_object: the table object to retrieve the row from
    TALLOC_CTX : the memory context to user to allocate data to return
    enum table_query_type query_type: defines on which view the query occurs:
        MAPISTORE_PREFILTERED_QUERY: has the table already been filtered using 
            set_restrictions or set_sort_order etc. In this case, return the row 
            in this filtered view
        MAPISTORE_LIVEFILTERED_QUERY: only setcolumns was applied, return rows 
            directly from the raw table results.
    uint32_t row_id: the index of the row to fetch within the table
    struct mapistore_property_data **data: the set of data for the row to return

mapistore_property_data is an abstracted way to return data. When OpenChange returns 
data to a client, it sends them within a DATA_BLOB where values (matching requested 
properties) are packed one to next,
sometimes prefixed by an error value to let the client know it didn't found the value 
or accessing the value was forbidden.

struct mapistore_property_data {
        void *data;
        int error;
};

This mapistore_property_data is just a data structure backends use to fill information. 
They put the value matching a property in void *data.
This is used to pass any kind of data back. If the value was found, set error to 
MAPISTORE_SUCCESS. If an error occurs, set data to NULL and set the error to 
MAPISTORE_ERR_NOT_FOUND or MAPI_E_NO_ACCESS depending on the status.
*/
static enum mapistore_error TableGetRow (void *table_object, TALLOC_CTX *mem_ctx,
                    enum mapistore_query_type query_type, uint32_t row_id,
                    struct mapistore_property_data **data)
{
//if( query_type == MAPISTORE_PREFILTERED_QUERY )
//if( query_type == MAPISTORE_LIVEFILTERED_QUERY )

DEBUG(0, ("MAPIEasyLinux : TableGetRow\n"));
return MAPISTORE_ERR_NOT_FOUND;
}

/*
 * get_row_count
 * This function retrieves the number of row for the specific table given the query_type. 
 * The function takes in parameter:
 *
 *  void *table_object: the table object from which to retrieve the number of rows
 *  enum table_type query_type: defines on which view the query occurs
 *  uint32_t *count: the number of rows to return

enum mapistore_query_type {
	MAPISTORE_PREFILTERED_QUERY,
	MAPISTORE_LIVEFILTERED_QUERY,
};
    
*/
static enum mapistore_error TableGetRowCount (void *table_object,
                          enum mapistore_query_type query_type,
                          uint32_t *row_countp)
{
struct EasyLinuxTable *elTable;

elTable = table_object;
if( query_type == MAPISTORE_PREFILTERED_QUERY )
  *row_countp = elTable->rowCount;
if( query_type == MAPISTORE_LIVEFILTERED_QUERY )
  *row_countp = elTable->rowCount;
  
DEBUG(0, ("MAPIEasyLinux : TableGetRowCount %i \n", elTable->rowCount));
return MAPISTORE_SUCCESS;
}

/*
handle_destructor

This is called automatically from OpenChange server when the table is destroyed or the associated folder is closed. It makes use of the talloc
hierarchy which creates a tree of memory contexts: parent, children.

When a parent gets deleted, all the children gets also deleted. the destructor function is a callback that can be associated to a TALLOC context and is called before the pointer is free'd.

In this case, the destructor receives the table object. So you know which table is associated and what to destroy. There is also the handle id passed down which let you know (remember the handle id / table object combination discussed earlier) which specific table to delete - if you followed this model.
The function takes in parameter:

    void *table_object: the table object to destroy
    unt32_t handle_id: the handle id that was associated to the specific table to delete


*/
static enum mapistore_error TableHandleDestructor (void *table_object, uint32_t handle_id)
{
int rc=MAPISTORE_ERR_NOT_IMPLEMENTED;
DEBUG(0, ("MAPIEasyLinux : TableHandleDestructor\n"));
return rc;
}

/*
properties.get_available_properties

Similarly to table.get_available_properties, this function returns the set of possible properties 
that can be set on the specified object. In this case the specified object is generic and it is up 
to the backend to detect which one it is referring to.
The function takes in parameter:

    void *object: generic object which can be folder, message etc.
    TALLOC_CTX *: the memory context to use to allocate the returned set of properties
    struct SPropTagArray **properties: pointer on pointer to the set of properties to return
*/
static enum mapistore_error PropertiesGetAvailableProperties(void *object,
                                                    TALLOC_CTX *mem_ctx,
                                                    struct SPropTagArray **propertiesP)
{
int rc=MAPISTORE_ERR_NOT_IMPLEMENTED;
DEBUG(0, ("MAPIEasyLinux : PropertiesGetAvailableProperties\n"));
return rc;
}

/*
properties.get_properties

This function retrieves the request properties value for the specific generic object. Similarly 
to properties.get_available_properties, it is up to the backend to detect which object is passed along.
The function takes in parameter:

    void *object: generic object to retrieve properties value from
    TALLOC_CTX *: the memory context to use to allocate returned properties data
    uint16_t count: the number of property tags requested
    enum MAPITAGS *: the list of property tags requested
    struct mapistore_property_data *: the set of property data to return (see table.get_row for information on mapistore_property_data)
*/
static enum mapistore_error PropertiesGetProperties (void *object,
                                TALLOC_CTX *mem_ctx,
                                uint16_t count, enum MAPITAGS *properties,
                                struct mapistore_property_data *data)
{
int rc=MAPISTORE_ERR_NOT_IMPLEMENTED;
DEBUG(0, ("MAPIEasyLinux : PropertiesGetProperties\n"));
return rc;
}

/*
properties.set_properties

This function sets properties on the generic specified object.
The function takes in parameter:

    void *object: generic object to set properties on
    struct SRow *aRow: pointer on a SRow structure holding properties to apply
    
    
struct SPropValue {
	enum MAPITAGS ulPropTag;  // en HEX
	uint32_t dwAlignPad;
	union SPropValue_CTR value; // [switch_is(ulPropTag&0xFFFF)]  - PROP_VAL_UNION
} // [noprint,nopush,public,nopull] 

struct SRow {
	uint32_t ulAdrEntryPad;
	uint32_t cValues; // [range(0,100000)] 
	struct SPropValue *lpProps; // [unique,size_is(cValues)] 
}// [noprint,nopush,public,nopull] 

union PROP_VAL_UNION {
	uint16_t i;										// [case(0x0002)] 
	uint32_t l;										// [case(0x0003)] 
	uint8_t b;										// [case(0x000b)] 
	const char *lpszA;						// [unique,charset(DOS),case(0x001e)] 
	const char *lpszW;						// [unique,charset(UTF16),case(0x001f)] 
	struct Binary_r bin;					// [case(0x0102)] 
	struct FlatUID_r *lpguid;			// [unique,case(0x0048)] 
	struct FILETIME ft;						// [case(0x0040)] 
	enum MAPISTATUS err;					// [case(0x000a)] 
	struct ShortArray_r MVi;			// [case(0x1002)] 
	struct LongArray_r MVl;				// [case(0x1003)] 
	struct StringArray_r MVszA;		// [case(0x101e)] 
	struct BinaryArray_r MVbin;		// [case(0x1102)] 
	struct FlatUIDArray_r MVguid;	// [case(0x1048)] 
	struct StringArrayW_r MVszW;	// [case(0x101f)] 
	struct DateTimeArray_r MVft;	// [case(0x1040)] 
	uint32_t null;								// [case(0x0001)] 
};															// [switch_type(uint32)] 




*/
static enum mapistore_error PropertiesSetProperties (void *object, struct SRow *aRow)
{
int rc=MAPISTORE_SUCCESS;
struct EasyLinuxGeneric *elGeneric;
struct EasyLinuxFolder  *elFolder;
struct EasyLinuxMessage *elMessage;
struct EasyLinuxTable   *elTable;
TALLOC_CTX *MemCtx;
char *sstType;
int i;

// We need to know of wich object Properties belong to
elGeneric = (struct EasyLinuxGeneric *)object;
switch( elGeneric->stType )
  {
  case EASYLINUX_FOLDER:
    elFolder = (struct EasyLinuxFolder *)object;
    if( elFolder->Table == NULL )
      {
      elFolder->Table = (struct EasyLinuxTable *)talloc_zero_size(elFolder->elContext->mem_ctx, sizeof(struct EasyLinuxTable));
      elFolder->Table->Props = NULL;
      elFolder->Table->elContext = elFolder->elContext;
      }
    elTable = elFolder->Table;  
    MemCtx = elFolder->elContext->mem_ctx;
    sstType = talloc_strdup(MemCtx,"Folder");
    break;
  case EASYLINUX_MSG:
    elMessage = (struct EasyLinuxMessage *)object;
    if( elMessage->Table == NULL )
      {
      elMessage->Table = (struct EasyLinuxTable *)talloc_zero_size(elMessage->elContext->mem_ctx, sizeof(struct EasyLinuxTable));
      elMessage->Table->Props = NULL;
      elMessage->Table->elContext = elMessage->elContext;
      }
    elTable = elMessage->Table;  
    MemCtx = elMessage->elContext->mem_ctx;
    sstType = talloc_strdup(MemCtx,"Message");
    break;
  case EASYLINUX_TABLE:
    elTable = (struct EasyLinuxTable *)object;
    MemCtx = elTable->elContext->mem_ctx;
    sstType = talloc_strdup(MemCtx,"Table");
    break;
  default:
    DEBUG(0,("ERROR: MAPIEasyLinux - Unknown object type (%i)\n",elGeneric->stType));
    break;
  }

DEBUG(0,("MAPIEasyLinux : SetProperties is about: %s\n", sstType));

// Store Properties
for(i=0 ; i<aRow->cValues ; i++)
  StorePropertie(elTable, aRow->lpProps[i]);

talloc_unlink(MemCtx,sstType);  
return rc;
}


/*
 *
 */
static enum mapistore_error ManagerGenerateUri (TALLOC_CTX *mem_ctx, 
                           const char *user, 
                           const char *folder, 
                           const char *message, 
                           const char *rootURI,
                           char **uri)
{
DEBUG(0, ("MAPIEasyLinux : ManagerGenerateUri\n"));
return MAPISTORE_ERR_NOT_IMPLEMENTED;
}

/**
 *  \details Entry point for mapistore backend. This function initiate entry points need by backend
 *
 *  This is the function called right after the backend is registered (mapistore_backend_register). 
 *  This function will be executed each time mapistore_init function is called from the calling application.
 *
 *  In the context of OpenChange server and Samba forked model, it means the backend would be called each time 
 *  a new instance of the server is created. To keep init only called once in the server's lifetime, it is required 
 *  to use a static variable within mapistore_init_backend which value changes when backend gets initialized the first time.
 *
 *  The init function is used to initialize your backend with data (or environment) it will need all along mapistore's 
 *  lifetime. It is a one-time operation and is generally used to: 
 *  - open a connection to a remote system
 *  - load a virtual machine
 *  It results in having a static structure, local to the backend's file which is used along calls to access an environment or file descriptor.
 *  It prevents from having to open or load a virtual machine or open a connection to a system each time a context is created.
 *
 *  \return MAPISTORE_SUCCESS on success, otherwise MAPISTORE error
 */
int mapistore_init_backend(void)
{
struct mapistore_backend	backend;
int				ret;
static BOOL registered = NO;

if (registered)
  ret = MAPISTORE_SUCCESS;
else
  {
  registered = YES;
  // Backend 
  backend.backend.name 												= "EasyLinux";
  backend.backend.description 								= "MAPIStore EasyLinux backend";
  backend.backend.namespace 									= "EasyLinux://";
  backend.backend.init 												= BackendInit;											// Ok
  backend.backend.create_context 							= BackendCreateContext;							// Ok
  backend.backend.create_root_folder 					= BackendCreateRootFolder;
  backend.backend.list_contexts 							= BackendListContexts;							// OK
  // Context
  backend.context.get_path 										= ContextGetPath;										// Ok
  backend.context.get_root_folder 						= ContextGetRootFolder;							// Ok
  // Folder
  backend.folder.open_folder 									= FolderOpen;
  backend.folder.create_folder 								= FolderCreate;
  backend.folder.delete 											= FolderDelete;
  backend.folder.open_message 								= FolderOpenMessage;
  backend.folder.create_message 							= FolderCreateMessage;
  backend.folder.delete_message 							= FolderDeleteMessage;
  backend.folder.move_copy_messages 					= FolderMoveCopyMessages;
  backend.folder.move_folder 									= FolderMove;
  backend.folder.copy_folder 									= FolderCopy;
  backend.folder.get_deleted_fmids 						= FolderGetDeletedFmids;
  backend.folder.get_child_count 							= FolderGetChildCount;
  backend.folder.open_table 									= FolderCreateTable;
  backend.folder.modify_permissions 					= FolderModifyPermissions;
  backend.folder.preload_message_bodies 			= FolderPreloadMessageBodies;
  // Message
  backend.message.create_attachment 					= MessageCreateAttachment;
  backend.message.get_attachment_table 				= MessageGetAttachmentTable;
  backend.message.open_attachment 						= MessageOpenAttachment;
  backend.message.open_embedded_message 			= MessageAttachmentOpenEmbeddedMessage;
  backend.message.create_embedded_message 		= MessageAttachmentCreateEmbeddedMessage;
  backend.message.get_message_data 						= MessageGetMessageData;
  backend.message.modify_recipients 					= MessageModifyRecipients;
  backend.message.set_read_flag 							= MessageSetReadFlag;
  backend.message.save 												= MessageSave;
  backend.message.submit 											= MessageSubmit;
  // Tables
  backend.table.get_available_properties 			= TableGetAvailableProperties;
  backend.table.set_restrictions 							= TableSetRestrictions;
  backend.table.set_sort_order 								= TableSetSortOrder;
  backend.table.set_columns 									= TableSetColumns;
  backend.table.get_row 											= TableGetRow;
  backend.table.get_row_count 								= TableGetRowCount;
  backend.table.handle_destructor 						= TableHandleDestructor;
  // Properties
  backend.properties.get_available_properties = PropertiesGetAvailableProperties;
  backend.properties.get_properties 					= PropertiesGetProperties;
  backend.properties.set_properties 					= PropertiesSetProperties;
  // Manager
  backend.manager.generate_uri 								= ManagerGenerateUri;

  /* Register ourselves with the MAPISTORE subsystem */
  ret = mapistore_backend_register(&backend);
  if (ret != MAPISTORE_SUCCESS  )
    DEBUG(0, ("MAPIEasyLinux : Failed to register the '%s' MAPIStore backend!\n", backend.backend.name));
  }

return ret;
}
