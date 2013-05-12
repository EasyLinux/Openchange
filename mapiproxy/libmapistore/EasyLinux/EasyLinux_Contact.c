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
#include <sys/types.h>
#include <dirent.h>

void OpenContacts(struct EasyLinuxContext *elContext)
{
elContext->bkType = EASYLINUX_CONTACTS;
elContext->RootFolder.stType = EASYLINUX_FOLDER;
elContext->RootFolder.FolderType = 0; // FOLDER_ROOT
elContext->RootFolder.RelPath  = talloc_strdup(elContext->mem_ctx,"/");
elContext->RootFolder.FullPath = talloc_asprintf(elContext->mem_ctx,"%s/Maildir/CONTACTS/Default.xml",elContext->User.homeDirectory);
}

int GetContacts(struct EasyLinuxFolder *elContext)
{
return 0;
}
