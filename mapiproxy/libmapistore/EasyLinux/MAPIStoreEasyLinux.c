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

return MAPISTORE_SUCCESS;
}

/*
You will notice there is no function to delete a context explicitly within the backend. This is because the mapistore interface deletes a backend on its own by free'ing the memory context used by this backend.

However, backend's developers are required to allocate their internal data with the memory context passed during create_context and associate a talloc destructor function. If additional operations need to be performed such as close a file descriptor, socket or call the language specific memory release system (garbage collector, pool etc.)

When mapistore will delete the context (free up memory), the hierarchy will automatically be processed and the destructor function for the backend will be called. This is in this internal function that you need to implement actions such as close (fd, socket) etc.

To create a talloc destructor, call the following function:

talloc_set_destructor((void *)context, (int (*)(void *))destructor_fct);

In this example context is the structure allocated with the memory context passed in parameter of the create_context function. An example of the destructor function would looks like:
*/
static int BackendDestructor(void *data)
{
// perform operations to clean-up everything properly 
DEBUG(3, ("SNOEL : Appel du destructeur\n"));

return 0;
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
 */

static enum mapistore_error BackendCreateContext(TALLOC_CTX *mem_ctx,
                            struct mapistore_connection_info *conn_info,
                            struct tdb_wrap *indexingTdb,
                            const char *uri, void **backend_object)
{
int rc=MAPISTORE_SUCCESS;  
struct EasyLinuxBackendContext *elBackendContext;

DEBUG(3, ("SNOEL : CreateContext Uri(%s)\n",uri ));

elBackendContext = (struct EasyLinuxBackendContext *)talloc_zero_size(mem_ctx, sizeof(struct EasyLinuxBackendContext) );
//*backend_object = (void *)&elBackendContext;
backend_object = (void *)&elBackendContext;
// Retreive user informations from Ldap
SetUserInformation(elBackendContext, mem_ctx, conn_info->username, conn_info->sam_ctx);
// Retrieve MAPI letteral Name
InitialiseRootFolder(elBackendContext, mem_ctx, conn_info->username, conn_info->oc_ctx, uri);
elBackendContext->Folder.Valid=1;
elBackendContext->Test=1;
// MAPIStore backend don't include a destructor function capability, we want one 
talloc_set_destructor((void *)mem_ctx, (int (*)(void *))BackendDestructor);
DEBUG(3, ("SNOEL : Set destructor function\n"));

return rc;
}

/**
 * \details Create root folder 
 */
static enum mapistore_error BackendCreateRootFolder (const char *username,
                                 enum mapistore_context_role role,
                                 uint64_t fid, 
                                 const char *name,
                                 // struct tdb_wrap *indexingTdb,
                                 TALLOC_CTX *mem_ctx, 
                                 char **mapistore_urip)
{
int rc=MAPISTORE_SUCCESS; // MAPISTORE_SUCCESS   MAPISTORE_ERR_NOT_IMPLEMENTED

DEBUG(0, ("BackendCreateRootFolder Role(%i) FID(%lX) Name(%s)\n",role, fid, name));
return rc;
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
char *ContextRole[]={"INBOX","INBOX/Draft", "INBOX/Sent", "INBOX/Outbox","INBOX/Trash","CALENDAR","CONTACTS","TASKS",
                     "NOTES","JOURNAL","FALLBACK","MAX"};
enum mapistore_context_role  Roles[]= {MAPISTORE_MAIL_ROLE, 	MAPISTORE_DRAFTS_ROLE, 	MAPISTORE_SENTITEMS_ROLE,
                     MAPISTORE_OUTBOX_ROLE, MAPISTORE_DELETEDITEMS_ROLE, MAPISTORE_CALENDAR_ROLE,
                     MAPISTORE_CONTACTS_ROLE, MAPISTORE_TASKS_ROLE, MAPISTORE_NOTES_ROLE, MAPISTORE_JOURNAL_ROLE,
                     MAPISTORE_FALLBACK_ROLE, MAPISTORE_MAX_ROLES };

DEBUG(3, ("Registering roles "));
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
	Context->name				 		= talloc_strdup(mem_ctx, ContextRole[i]);
	Context->main_folder 		= true;
	Context->role 					= Roles[i];
	Context->tag						= talloc_strdup(mem_ctx, ContextRole[i]);
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
	
	DEBUG(3, ("."));
	}

DEBUG(3, ("BackendListContexts %s done! \n",username));
return rc;
}


/**
 * \details return the mapistore path associated to a given message or folder ID
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
static enum mapistore_error ContextGetPath(void *backend_object, TALLOC_CTX *mem_ctx, uint64_t fmid, char **path)
{
char *Path;
int rc=MAPISTORE_SUCCESS;
struct EasyLinuxBackendContext *elBackendContext;

elBackendContext = (struct EasyLinuxBackendContext *)backend_object;
//DEBUG(0, ("Dans ContextGetPath  (%i)\n",elBackendContext->Folder.Valid));
//Path = talloc_asprintf(mem_ctx, "%s",elBackendContext->Folder.displayName);
//path = &Path;
DEBUG(0, ("ContextGetPath (%lX) \n",fmid));
return rc;
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
int rc=MAPISTORE_SUCCESS;  // MAPISTORE_ERR_NO_DIRECTORY    MAPISTORE_SUCCESS
struct EasyLinuxBackendContext *elBackendContext;

elBackendContext = (struct EasyLinuxBackendContext *)backend_object;
//folder_object = (void *)&elBackendContext->Folder;

DEBUG(0, ("ContextGetRootFolder (%lX) Valid(%i)\n",fid, elBackendContext->Test));
return rc;
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
DEBUG(0, ("FolderOpen\n"));
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
 */
static enum mapistore_error FolderCreate(void *parent_folder, TALLOC_CTX *mem_ctx, uint64_t fid, struct SRow *aRow, void **childfolder)
{
int rc=MAPISTORE_ERR_NOT_IMPLEMENTED;
DEBUG(0, ("FolderCreate\n"));
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
DEBUG(0, ("FolderDelete\n"));
return rc;
}

/**
 * This function retrieves either the number of messages, FAI messages or folders within specified folder. The function takes in parameters:
 *
 * \param *folder_object	  void				the folder on which the count has to be retrieved
 * \param table_type  			uint8_t			the type of item to retrieve. Possible values are listed below
 * \param *rowCount  				uint32_t 		the number of elements retrieved

Possible values for table_type are:
- MAPISTORE_MESSAGE_TABLE: the number of messages within the folder
- MAPISTORE_FAI_TABLE: the number of FAI messages within the folder
- MAPISTORE_FOLDER_TABLE: the number of folders within the folder
*/
static enum mapistore_error FolderGetChildCount(void *folder_object, enum mapistore_table_type table_type, uint32_t *rowCount)
{
int rc=MAPISTORE_ERR_NOT_IMPLEMENTED;
DEBUG(0, ("FolderGetChildCount\n"));
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
DEBUG(0, ("FolderOpenMessage\n"));
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
static enum mapistore_error FolderCreateMessage(void *folder_object,
                           TALLOC_CTX *mem_ctx,
                           uint64_t mid,
                           uint8_t associated,
                           void **message_object)
{
int rc=MAPISTORE_ERR_NOT_IMPLEMENTED;
DEBUG(0, ("FolderCreateMessage\n"));
return rc;
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
DEBUG(0, ("FolderDeleteMessage\n"));
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
DEBUG(0, ("FolderMoveCopyMessages\n"));
return rc;
}

/**

*/
static enum mapistore_error FolderMove(void *folder_object, void *target_folder_object,
                        TALLOC_CTX *mem_ctx, const char *new_folder_name)
{
int rc=MAPISTORE_SUCCESS;
DEBUG(0, ("FolderMoveFolder\n"));
return rc;
}

/**

*/
static enum mapistore_error FolderCopy(void *folder_object, void *target_folder_object, TALLOC_CTX *mem_ctx,
                        bool recursive, const char *new_folder_name)
{
int rc=MAPISTORE_ERR_NOT_IMPLEMENTED;
DEBUG(0, ("FolderCopyFolder\n"));
return rc;
}

/**

*/
static enum mapistore_error FolderGetDeletedFmids(void *folder_object, TALLOC_CTX *mem_ctx,
                              enum mapistore_table_type table_type, uint64_t change_num,
                              struct UI8Array_r **fmidsp, uint64_t *cnp)
{
int rc=MAPISTORE_ERR_NOT_IMPLEMENTED;
DEBUG(0, ("FolderGetDeletedFmids\n"));
return rc;
}

/**
This function creates a table object to be used along with backend's table operations. The function takes in parameter:
- void *folder_object: the folder on which the table has to be created
- TALLOC_CTX *: the memory context to use to create the table
- uint8_t table_type: the table type to create. See below for possible table_type values
- uint32_t handle_id: Exchange temporary id for the object. Used for the moment for notifications on table
- void **table_object: the table object to return
- uint32_t *row_count: the number of row of the table to be returned. It is used by OpenChange server and returned clients, so they know how many rows are available and how much they can query.

Possible values for table_type are:
- MAPISTORE_MESSAGE_TABLE: creates a table which only lists messages
- MAPISTORE_FAI_TABLE: creates a table which only lists FAI messages
- MAPISTORE_FOLDER_TABLE: creates a table which only lists folders
- MAPISTORE_PERMISSION_TABLE: creates a table which list permissions for the folder

For the handle_id, it is a hackish implementation from SOGo. It is used to process notifications on tables when registered by clients. This number is associated with the table objects and the couple is used to find a table and trigger a notification payload upon table changes.

The handling of handle_id can be avoided for now as it is likely to change with OCSManager final implementation.

OpenChange is waiting for a table_object (void **) which it doesn't care about, but will pass it to your backend when a new table operation occurs. It is also waiting for the number of rows in the table (uint32_t *row_count)
*/
static enum mapistore_error FolderOpenTable(void *folder_object, TALLOC_CTX *mem_ctx,
                       enum mapistore_table_type table_type, uint32_t handle_id,
                       void **table_object, uint32_t *row_count)
{
int rc=MAPISTORE_ERR_NOT_IMPLEMENTED;
DEBUG(0, ("FolderOpenTable\n"));
return rc;
}

/**

*/
static enum mapistore_error FolderModifyPermissions(void *folder_object, uint8_t flags,
                               uint16_t pcount,
                               struct PermissionData *permissions)
{
int rc=MAPISTORE_ERR_NOT_IMPLEMENTED;
DEBUG(0, ("FolderModifyPermissions\n"));
return rc;
}

/**

*/
static enum mapistore_error FolderPreloadMessageBodies(void *folder_object, enum mapistore_table_type table_type, const struct UI8Array_r *mids)
{
int rc=MAPISTORE_ERR_NOT_IMPLEMENTED;
DEBUG(0, ("FolderPreloadMessageBodies\n"));
return rc;
}

/**

*/
static enum mapistore_error MessageGetMessageData(void *message_object,
                              TALLOC_CTX *mem_ctx,
                              struct mapistore_message **msg_dataP)
{
int rc=MAPISTORE_ERR_NOT_IMPLEMENTED;
DEBUG(0, ("MessageGetMessageData\n"));
return rc;
}

/**
*/
static enum mapistore_error MessageCreateAttachment (void *message_object, TALLOC_CTX *mem_ctx, void **attachment_object, uint32_t *aidp)
{
int rc=MAPISTORE_ERR_NOT_IMPLEMENTED;
DEBUG(0, ("MessageCreateAttachment\n"));
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
DEBUG(0, ("MessageOpenAttachment\n"));
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
DEBUG(0, ("MessageGetAttachmentTable\n"));
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
DEBUG(0, ("MessageModifyRecipients\n"));
return rc;
}

/**

*/
static enum mapistore_error MessageSetReadFlag (void *message_object, uint8_t flag)
{
int rc=MAPISTORE_ERR_NOT_IMPLEMENTED;
DEBUG(0, ("MessageSetReadFlag  Message Id     flag %x\n",flag));
    //folders that are containers and which are of the mstoredb kind.
    //folders that are actually heavily used such as Inbox, Calendar, Outbox etc and which can be of any backend type.
return rc;
}

/**
This operation saves a message on remote/local storage system your backend handles. It takes all the temporary information associated to the message that has been modified/created and dump replicate the change/creation on the remote system. This function takes in parameter the message object. This is for example used when you create or edit a calendar, note, task or draft item.
*/
static enum mapistore_error MessageSave (void *message_object, TALLOC_CTX *mem_ctx)
{
int rc=MAPISTORE_ERR_NOT_IMPLEMENTED;
DEBUG(0, ("MessageSave\n"));
return rc;
}

/**
This operation push a message for dispatch. It only apply to emails or appointment invitation. The client expects the message to be sent to the spooler and dispatched to specified recipients.
The function takes in parameters:

    void *message_object: the message object to submit
    enum SubmitFlags: unused so far

*/
static enum mapistore_error MessageSubmit (void *message_object, enum SubmitFlags flags)
{
int rc=MAPISTORE_ERR_NOT_IMPLEMENTED;
DEBUG(0, ("MessageSubmit\n"));
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
DEBUG(0, ("MessageAttachmentOpenEmbeddedMessage\n"));
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
DEBUG(0, ("MessageAttachmentCreateEmbeddedMessage\n"));
return rc;
}

/**

*/
static enum mapistore_error TableGetAvailableProperties(void *table_object,
                                               TALLOC_CTX *mem_ctx, struct SPropTagArray **propertiesP)
{
int rc=MAPISTORE_ERR_NOT_IMPLEMENTED;
DEBUG(0, ("TableGetAvailableProperties\n"));
return rc;
}

static enum mapistore_error TableSetColumns (void *table_object, uint16_t count, enum MAPITAGS *properties)
{
int rc=MAPISTORE_ERR_NOT_IMPLEMENTED;
DEBUG(0, ("TableSetColumns\n"));
return rc;
}

static enum mapistore_error TableSetRestrictions (void *table_object, struct mapi_SRestriction *restrictions, uint8_t *table_status)
{
int rc=MAPISTORE_ERR_NOT_IMPLEMENTED;
DEBUG(0, ("TableSetRestrictions\n"));
return rc;
}

static enum mapistore_error TableSetSortOrder (void *table_object, struct SSortOrderSet *sort_order, uint8_t *table_status)
{
int rc=MAPISTORE_ERR_NOT_IMPLEMENTED;
DEBUG(0, ("TableSetSortOrder\n"));
return rc;
}


static enum mapistore_error TableGetRow (void *table_object, TALLOC_CTX *mem_ctx,
                    enum mapistore_query_type query_type, uint32_t row_id,
                    struct mapistore_property_data **data)
{
int rc=MAPISTORE_ERR_NOT_IMPLEMENTED;
DEBUG(0, ("TableGetRow\n"));
return rc;
}

static enum mapistore_error TableGetRowCount (void *table_object,
                          enum mapistore_query_type query_type,
                          uint32_t *row_countp)
{
int rc=MAPISTORE_ERR_NOT_IMPLEMENTED;
DEBUG(0, ("TableGetRowCount\n"));
return rc;
}

static enum mapistore_error TableHandleDestructor (void *table_object, uint32_t handle_id)
{
int rc=MAPISTORE_ERR_NOT_IMPLEMENTED;
DEBUG(0, ("TableHandleDestructor\n"));
return rc;
}

static enum mapistore_error PropertiesGetAvailableProperties(void *object,
                                                    TALLOC_CTX *mem_ctx,
                                                    struct SPropTagArray **propertiesP)
{
int rc=MAPISTORE_ERR_NOT_IMPLEMENTED;
DEBUG(0, ("PropertiesGetAvailableProperties\n"));
return rc;
}

static enum mapistore_error PropertiesGetProperties (void *object,
                                TALLOC_CTX *mem_ctx,
                                uint16_t count, enum MAPITAGS *properties,
                                struct mapistore_property_data *data)
{
int rc=MAPISTORE_ERR_NOT_IMPLEMENTED;
DEBUG(0, ("PropertiesGetProperties\n"));
return rc;
}

static enum mapistore_error PropertiesSetProperties (void *object, struct SRow *aRow)
{
DEBUG(0, ("PropertiesSetProperties\n"));
int rc=MAPISTORE_SUCCESS;
return rc;
}

static enum mapistore_error ManagerGenerateUri (TALLOC_CTX *mem_ctx, 
                           const char *user, 
                           const char *folder, 
                           const char *message, 
                           const char *rootURI,
                           char **uri)
{
DEBUG(0, ("ManagerGenerateUri\n"));
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
  backend.backend.init 												= BackendInit;
  backend.backend.create_context 							= BackendCreateContext;
  backend.backend.create_root_folder 					= BackendCreateRootFolder;
  backend.backend.list_contexts 							= BackendListContexts;
  // Context
  backend.context.get_path 										= ContextGetPath;
  backend.context.get_root_folder 						= ContextGetRootFolder;
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
  backend.folder.open_table 									= FolderOpenTable;
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
    DEBUG(0, ("\nFailed to register the '%s' MAPIStore backend!\n", backend.backend.name));
  }

return ret;
}
