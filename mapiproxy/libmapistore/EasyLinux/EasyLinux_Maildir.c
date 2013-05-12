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
#include <sys/types.h>
#include <dirent.h>

#define isFile 0x8
#define isDir  0x4

/* 
 * OpenMaildir 
 */
enum mapistore_error OpenMailDir(struct EasyLinuxContext *elContext)
{
char *File;
// Uri   EasyLinux://INBOX/Sent
File = talloc_asprintf(elContext->mem_ctx,"%s",&elContext->RootFolder.Uri[18]);

// Fill in folder structure
elContext->bkType = EASYLINUX_MAILDIR;
elContext->RootFolder.stType = EASYLINUX_FOLDER;
elContext->RootFolder.FolderType = 0; // FOLDER_ROOT
elContext->RootFolder.RelPath = talloc_asprintf(elContext->mem_ctx,".%s",File);
if( strcmp(elContext->RootFolder.RelPath,".") == 0 )
  {
  elContext->RootFolder.RelPath[0] = '/';
 	elContext->RootFolder.FullPath = talloc_asprintf(elContext->mem_ctx,"%s/Maildir/",elContext->User.homeDirectory);
 	}
else
 	elContext->RootFolder.FullPath = talloc_asprintf(elContext->mem_ctx,"%s/Maildir/.%s",elContext->User.homeDirectory, File);
DEBUG(3, ("MAPIEasyLinux :   OpenMailDir(%s)\n", elContext->RootFolder.FullPath));  

// Test if Maildir !exist --> if no we have to create it

talloc_unlink(elContext->mem_ctx, File);
return MAPISTORE_SUCCESS;
}

/*
 * Get number of childs of actual Folder
 */
int GetMaildirChildCount(struct EasyLinuxFolder *elFolder, uint32_t table_type)
{
int Val=0;
DIR *dDir;
char *sDir;
struct dirent *dEntry;

switch( table_type )
  {
  case 2:   // MAPISTORE_MESSAGE_TABLE
    // Find Messages in cur & new
    sDir = talloc_asprintf(elFolder->elContext->mem_ctx, "%scur", elFolder->FullPath);
    dDir = opendir(sDir);
    while ((dEntry = readdir(dDir))) 
      {
      if ( dEntry->d_type == isFile)
        {
        Val++;
        }
      }
    closedir(dDir);
    talloc_unlink(elFolder->elContext->mem_ctx, sDir);
    sDir = talloc_asprintf(elFolder->elContext->mem_ctx, "%snew", elFolder->FullPath);
    dDir = opendir(sDir);
    while ((dEntry = readdir(dDir))) 
      {
      if ( dEntry->d_type == isFile)
        {
        Val++;
        }
      }
    closedir(dDir);
    DEBUG(3,("MAPIEasyLinux :      Find %i Messages in %s\n",Val,elFolder->FullPath)); 
    talloc_unlink(elFolder->elContext->mem_ctx, sDir);
    break;
    
  case 3:   // MAPISTORE_FAI_TABLE
    DEBUG(0,("MAPIEasyLinux :    Ask for GetMaildirChildCount\n"));
    break;
    
  case 1:   // MAPISTORE_FOLDER_TABLE
    dDir = opendir(elFolder->FullPath);
    while ((dEntry = readdir(dDir))) 
      {
      if ( dEntry->d_type == isDir)
        Val += OmmitSpecialFolder(elFolder,dEntry->d_name);
      }
    closedir(dDir);
    DEBUG(3,("MAPIEasyLinux :      Find %i Folders in %s\n",Val,elFolder->FullPath)); 
    break;
  }  
return Val;
}

/* 
 * Return 1 if Name is not a special folder
 */
int OmmitSpecialFolder(struct EasyLinuxFolder *elFolder,char *Name)
{
char *SpecialNames[] = {".", "..", ".Sent",".Trash",".Outbox",".Drafts"};
char *Search;
int i, len=2;  // len: Normal mail dir ommit only . and ..

if( Name[0] != '.' )
  return 0; // Not a Maildir subdirectory 

// If current folder is Maildir root, we have to omit .Sent, .Trash, .Outbox and .Drafts  
Search = talloc_asprintf(elFolder->elContext->mem_ctx,"%s/Maildir/",elFolder->elContext->User.homeDirectory);
if( strcmp(Search,elFolder->FullPath) == 0 )
  len=6; // This is the root maildir ommit all Special names
talloc_unlink(elFolder->elContext->mem_ctx, Search);

for(i=0; i<len ; i++)  // If Name is one of Special folders, omit it
  if( strcmp(Name, SpecialNames[i]) == 0 )
    return 0;

return 1;
}

enum mapistore_error MailDirCreateFolder(struct EasyLinuxFolder *elNewFolder)
{
int i, hSub;
char *SubDirs[] = {"new", "cur", "tmp"};
char *FullPath, *sSubscriptions;

if( mkdir(elNewFolder->FullPath, 0775) && (EEXIST != errno) )
  {
  DEBUG(0,("ERROR : MAPIEasyLinux - cannot create '%s' folder\n", elNewFolder->FullPath));
  talloc_unlink(elNewFolder->elContext->mem_ctx, elNewFolder->FullPath);
  return MAPISTORE_ERROR;
  }
if( chown(elNewFolder->FullPath, elNewFolder->elContext->User.uidNumber, elNewFolder->elContext->User.gidNumber) )
  {
  DEBUG(0,("ERROR : MAPIEasyLinux - cannot chown '%s' folder\n", elNewFolder->FullPath));
  talloc_unlink(elNewFolder->elContext->mem_ctx, elNewFolder->FullPath);
  return MAPISTORE_ERROR;
  }

for (i = 0; i<3 ; ++i) 
  {
  FullPath = talloc_asprintf(elNewFolder->elContext->mem_ctx,"%s%s",elNewFolder->FullPath, SubDirs[i]);
  if( mkdir(FullPath, 0775) && (EEXIST != errno) )
    {
    DEBUG(0,("ERROR : MAPIEasyLinux - cannot create '%s' folder\n", FullPath));
    talloc_unlink(elNewFolder->elContext->mem_ctx, FullPath);
    return MAPISTORE_ERROR;
    }
  if( chown(FullPath, elNewFolder->elContext->User.uidNumber, elNewFolder->elContext->User.gidNumber) )
    {
    DEBUG(0,("ERROR : MAPIEasyLinux - cannot chown '%s' folder\n", FullPath));
    talloc_unlink(elNewFolder->elContext->mem_ctx, FullPath);
    return MAPISTORE_ERROR;
    }
  talloc_unlink(elNewFolder->elContext->mem_ctx, FullPath);
  }
// Register new folder to Dovecot  
sSubscriptions = (char *)talloc_asprintf(elNewFolder->elContext->mem_ctx, "%ssubscriptions", elNewFolder->elContext->RootFolder.FullPath);
hSub = open(sSubscriptions,O_RDWR | O_APPEND);
FullPath = (char *)talloc_asprintf(elNewFolder->elContext->mem_ctx,"%s\n", &elNewFolder->RelPath[1]);
write(hSub,FullPath, strlen(FullPath));
close(hSub);
talloc_unlink(elNewFolder->elContext->mem_ctx, FullPath);
talloc_unlink(elNewFolder->elContext->mem_ctx, sSubscriptions);
return MAPISTORE_SUCCESS;
}


/*
 * Convert an Imap string to a filesystem string
 * ex INBOX/Test/Test2 -> .Test.Test2
 */
char *ImapToMaildir(TALLOC_CTX *MemCtx, char *sImap)
{
char *returnString;
int i;

returnString = (char *)talloc_zero_size(MemCtx, strlen(sImap));
i=0;
while(sImap[i] != 0)
  {
  if( sImap[i+5] == '/' )
    returnString[i] = '.';
  else
    returnString[i] = sImap[i+5];
  i++;
  }
returnString[i] = 0;
return( returnString ); 
}




/*
if (XmlDoc == NULL) 
  {
  DEBUG(0,("ERROR: MAPIEasyLinux - Invalid Xml file %s.xml\n",File));
  }
// Open root of Xml File
XmlRoot = xmlDocGetRootElement(XmlDoc);
// void xmlNodeSetContent(xmlNodePtr cur, const xmlChar * content);
if (XmlRoot == NULL) 
  {
  DEBUG(0,("MAPIEasyLinux - Empty Xml file\n"));
  xmlFreeDoc(XmlDoc);
  }
DEBUG(3,("MAPIEasyLinux : Xml root document is '%s'\n", XmlRoot->name));
// Free memory


#include "sys_defs.h"
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>

// Utility library. 

#include <msg.h>
#include <mymalloc.h>
#include <stringops.h>
#include <vstream.h>
#include <vstring.h>
#include <make_dirs.h>
#include <set_eugid.h>
#include <get_hostname.h>
#include <sane_fsops.h>

// Global library. 

#include <mail_copy.h>
#include <bounce.h>
#include <defer.h>
#include <sent.h>
#include <mail_params.h>
#include <mbox_open.h>
#include <dsn_util.h>

// Application-specific. 

#include "virtual.h"
*/
/*
 * deliver_maildir - delivery to maildir-style mailbox
 */ 
/*
int     deliver_maildir(LOCAL_STATE state, USER_ATTR usr_attr)
{
    const char *myname = "deliver_maildir";
    char   *newdir;
    char   *tmpdir;
    char   *curdir;
    char   *tmpfile;
    char   *newfile;
    DSN_BUF *why = state.msg_attr.why;
    VSTRING *buf;
    VSTREAM *dst;
    int     mail_copy_status;
    int     deliver_status;
    int     copy_flags;
    struct stat st;
    struct timeval starttime;

    GETTIMEOFDAY(&starttime);

    / *
     * Make verbose logging easier to understand.
     * /
    state.level++;
    if (msg_verbose)
	MSG_LOG_STATE(myname, state);

    / *
     * Don't deliver trace-only requests.
     * /
    if (DEL_REQ_TRACE_ONLY(state.request->flags)) {
	dsb_simple(why, "2.0.0", "delivers to maildir");
	return (sent(BOUNCE_FLAGS(state.request),
		     SENT_ATTR(state.msg_attr)));
    }

    / *
     * Initialize. Assume the operation will fail. Set the delivered
     * attribute to reflect the final recipient.
     * /
    if (vstream_fseek(state.msg_attr.fp, state.msg_attr.offset, SEEK_SET) < 0)
	msg_fatal("seek message file %s: %m", VSTREAM_PATH(state.msg_attr.fp));
    state.msg_attr.delivered = state.msg_attr.rcpt.address;
    mail_copy_status = MAIL_COPY_STAT_WRITE;
    buf = vstring_alloc(100);

    copy_flags = MAIL_COPY_TOFILE | MAIL_COPY_RETURN_PATH
	| MAIL_COPY_DELIVERED | MAIL_COPY_ORIG_RCPT;

    newdir = concatenate(usr_attr.mailbox, "new/", (char *) 0);
    tmpdir = concatenate(usr_attr.mailbox, "tmp/", (char *) 0);
    curdir = concatenate(usr_attr.mailbox, "cur/", (char *) 0);

    / *
     * Create and write the file as the recipient, so that file quota work.
     * Create any missing directories on the fly. The file name is chosen
     * according to ftp://koobera.math.uic.edu/www/proto/maildir.html:
     * 
     * "A unique name has three pieces, separated by dots. On the left is the
     * result of time(). On the right is the result of gethostname(). In the
     * middle is something that doesn't repeat within one second on a single
     * host. I fork a new process for each delivery, so I just use the
     * process ID. If you're delivering several messages from one process,
     * use starttime.pid_count.host, where starttime is the time that your
     * process started, and count is the number of messages you've
     * delivered."
     * 
     * Well, that stopped working on fast machines, and on operating systems
     * that randomize process ID values. When creating a file in tmp/ we use
     * the process ID because it still is an exclusive resource. When moving
     * the file to new/ we use the device number and inode number. I do not
     * care if this breaks on a remote AFS file system, because people should
     * know better.
     * 
     * On January 26, 2003, http://cr.yp.to/proto/maildir.html said:
     * 
     * A unique name has three pieces, separated by dots. On the left is the
     * result of time() or the second counter from gettimeofday(). On the
     * right is the result of gethostname(). (To deal with invalid host
     * names, replace / with \057 and : with \072.) In the middle is a
     * delivery identifier, discussed below.
     * 
     * [...]
     * 
     * Modern delivery identifiers are created by concatenating enough of the
     * following strings to guarantee uniqueness:
     * 
     * [...]
     * 
     * In, where n is (in hexadecimal) the UNIX inode number of this file.
     * Unfortunately, inode numbers aren't always available through NFS.
     * 
     * Vn, where n is (in hexadecimal) the UNIX device number of this file.
     * Unfortunately, device numbers aren't always available through NFS.
     * (Device numbers are also not helpful with the standard UNIX
     * filesystem: a maildir has to be within a single UNIX device for link()
     * and rename() to work.)
     * 
     * Mn, where n is (in decimal) the microsecond counter from the same
     * gettimeofday() used for the left part of the unique name.
     * 
     * Pn, where n is (in decimal) the process ID.
     * 
     * [...]
     * /
    set_eugid(usr_attr.uid, usr_attr.gid);
    vstring_sprintf(buf, "%lu.P%d.%s",
		 (unsigned long) starttime.tv_sec, var_pid, get_hostname());
    tmpfile = concatenate(tmpdir, STR(buf), (char *) 0);
    newfile = 0;
    if ((dst = vstream_fopen(tmpfile, O_WRONLY | O_CREAT | O_EXCL, 0600)) == 0
	&& (errno != ENOENT
	    || make_dirs(tmpdir, 0700) < 0
	    || (dst = vstream_fopen(tmpfile, O_WRONLY | O_CREAT | O_EXCL, 0600)) == 0)) {
	dsb_simple(why, mbox_dsn(errno, "4.2.0"),
		   "create maildir file %s: %m", tmpfile);
    } else if (fstat(vstream_fileno(dst), &st) < 0) {

	/ *
	 * Coverity 200604: file descriptor leak in code that never executes.
	 * Code replaced by msg_fatal(), as it is not worthwhile to continue
	 * after an impossible error condition.
	 * /
	msg_fatal("fstat %s: %m", tmpfile);
    } else {
	vstring_sprintf(buf, "%lu.V%lxI%lxM%lu.%s",
			(unsigned long) starttime.tv_sec,
			(unsigned long) st.st_dev,
			(unsigned long) st.st_ino,
			(unsigned long) starttime.tv_usec,
			get_hostname());
	newfile = concatenate(newdir, STR(buf), (char *) 0);
	if ((mail_copy_status = mail_copy(COPY_ATTR(state.msg_attr),
					  dst, copy_flags, "\n",
					  why)) == 0) {
	    if (sane_link(tmpfile, newfile) < 0
		&& (errno != ENOENT
		    || (make_dirs(curdir, 0700), make_dirs(newdir, 0700)) < 0
		    || sane_link(tmpfile, newfile) < 0)) {
		dsb_simple(why, mbox_dsn(errno, "4.2.0"),
			   "create maildir file %s: %m", newfile);
		mail_copy_status = MAIL_COPY_STAT_WRITE;
	    }
	}
	if (unlink(tmpfile) < 0)
	    msg_warn("remove %s: %m", tmpfile);
    }
    set_eugid(var_owner_uid, var_owner_gid);

    / *
     * The maildir location is controlled by the mail administrator. If
     * delivery fails, try again later. We would just bounce when the maildir
     * location possibly under user control.
     * /
    if (mail_copy_status & MAIL_COPY_STAT_CORRUPT) {
	deliver_status = DEL_STAT_DEFER;
    } else if (mail_copy_status != 0) {
	if (errno == EACCES) {
	    msg_warn("maildir access problem for UID/GID=%lu/%lu: %s",
		     (long) usr_attr.uid, (long) usr_attr.gid,
		     STR(why->reason));
	    msg_warn("perhaps you need to create the maildirs in advance");
	}
	vstring_sprintf_prepend(why->reason, "maildir delivery failed: ");
	deliver_status =
	    (STR(why->status)[0] == '4' ?
	     defer_append : bounce_append)
	    (BOUNCE_FLAGS(state.request),
	     BOUNCE_ATTR(state.msg_attr));
    } else {
	dsb_simple(why, "2.0.0", "delivered to maildir");
	deliver_status = sent(BOUNCE_FLAGS(state.request),
			      SENT_ATTR(state.msg_attr));
    }
    vstring_free(buf);
    myfree(newdir);
    myfree(tmpdir);
    myfree(curdir);
    myfree(tmpfile);
    if (newfile)
	myfree(newfile);
    return (deliver_status);
}

// -*-mode: c; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*-* 

/ *
** Copyright (C) 2008-2013 Dirk-Jan C. Binnema <djcb@djcbsoftware.nl>
**
** This program is free software; you can redistribute it and/or modify it
** under the terms of the GNU General Public License as published by the
** Free Software Foundation; either version 3, or (at your option) any
** later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software Foundation,
** Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
**
* /

#if HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H 

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <string.h>
#include <errno.h>
#include <glib/gprintf.h>

#include "mu-util.h"
#include "mu-maildir.h"
#include "mu-str.h"

#define MU_MAILDIR_NOINDEX_FILE ".noindex"
#define MU_MAILDIR_NOUPDATE_FILE ".noupdate"


/ * On Linux (and some BSD), we have entry->d_type, but some file
* systems (XFS, ReiserFS) do not support it, and set it DT_UNKNOWN.
* On other OSs, notably Solaris, entry->d_type is not present at all.
* For these cases, we use lstat (in get_dtype) as a slower fallback,
* and return it in the d_type parameter
* /
#ifdef HAVE_STRUCT_DIRENT_D_TYPE
#define GET_DTYPE(DE,FP) \
((DE)->d_type == DT_UNKNOWN ? mu_util_get_dtype_with_lstat((FP)) : \
(DE)->d_type)
#else
#define GET_DTYPE(DE,FP) \
mu_util_get_dtype_with_lstat((FP))
#endif // HAVE_STRUCT_DIRENT_D_TYPE


static gboolean create_maildir (const char *path, mode_t mode, GError **err)
{
int i;
const gchar* subdirs[] = {"new", "cur", "tmp"};

for (i = 0; i != G_N_ELEMENTS(subdirs); ++i) {

const char *fullpath;
int rv;

// static buffer 
fullpath = mu_str_fullpath_s (path, subdirs[i]);

/ * if subdir already exists, don't try to re-create
* it * /
if (mu_util_check_dir (fullpath, TRUE, TRUE))
continue;

rv = g_mkdir_with_parents (fullpath, (int)mode);

/ * note, g_mkdir_with_parents won't detect an error if
* there's already such a dir, but with the wrong
* permissions; so we need to check * /
if (rv != 0 || !mu_util_check_dir(fullpath, TRUE, TRUE))
return mu_util_g_set_error
(err,MU_ERROR_FILE_CANNOT_MKDIR,
"creating dir failed for %s: %s",
fullpath, strerror (errno));
}

return TRUE;
}

static gboolean create_noindex (const char *path, GError **err)
{
// create a noindex file if requested 
int fd;
const char *noindexpath;

// static buffer 
noindexpath = mu_str_fullpath_s (path, MU_MAILDIR_NOINDEX_FILE);

fd = creat (noindexpath, 0644);

/ * note, if the 'close' failed, creation may still have
* succeeded...* /
if (fd < 0 || close (fd) != 0)
return mu_util_g_set_error (err, MU_ERROR_FILE_CANNOT_CREATE,
"error in create_noindex: %s",
strerror (errno));
return TRUE;
}

gboolean mu_maildir_mkdir (const char* path, mode_t mode, gboolean noindex, GError **err)
{
g_return_val_if_fail (path, FALSE);

MU_WRITE_LOG ("%s (%s, %o, %s)", __FUNCTION__,
path, mode, noindex ? "TRUE" : "FALSE");

if (!create_maildir (path, mode, err))
return FALSE;

if (noindex && !create_noindex (path, err))
return FALSE;

return TRUE;
}

/ * determine whether the source message is in 'new' or in 'cur';
* we ignore messages in 'tmp' for obvious reasons * /
static gboolean check_subdir (const char *src, gboolean *in_cur, GError **err)
{
gboolean rv;
gchar *srcpath;

srcpath = g_path_get_dirname (src);
*in_cur = FALSE;
rv = TRUE;

if (g_str_has_suffix (srcpath, "cur"))
*in_cur = TRUE;
else if (!g_str_has_suffix (srcpath, "new"))
rv = mu_util_g_set_error (err,
MU_ERROR_FILE_INVALID_SOURCE,
"invalid source message '%s'",
src);
g_free (srcpath);
return rv;
}

static gchar*
get_target_fullpath (const char* src, const gchar *targetpath, GError **err)
{
gchar *targetfullpath, *srcfile;
gboolean in_cur;

if (!check_subdir (src, &in_cur, err))
return NULL;

srcfile = g_path_get_basename (src);

/ * create targetpath; note: make the filename cough* unique by
*including a hash * of the srcname in the targetname. This
*helps if there are * copies of a message (which all have the
*same basename)* /
targetfullpath = g_strdup_printf ("%s%c%s%c%u_%s",
targetpath,
G_DIR_SEPARATOR,
in_cur ? "cur" : "new",
G_DIR_SEPARATOR,
g_str_hash(src),
srcfile);
g_free (srcfile);

return targetfullpath;
}


gboolean
mu_maildir_link (const char* src, const char *targetpath, GError **err)
{
gchar *targetfullpath;
int rv;

g_return_val_if_fail (src, FALSE);
g_return_val_if_fail (targetpath, FALSE);

targetfullpath = get_target_fullpath (src, targetpath, err);
if (!targetfullpath)
return FALSE;

rv = symlink (src, targetfullpath);

if (rv != 0)
mu_util_g_set_error (err, MU_ERROR_FILE_CANNOT_LINK,
"error creating link %s => %s: %s",
targetfullpath, src, strerror (errno));
g_free (targetfullpath);

return rv == 0 ? TRUE: FALSE;
}


static MuError
process_dir (const char* path, const gchar *mdir,
MuMaildirWalkMsgCallback msg_cb,
MuMaildirWalkDirCallback dir_cb, gboolean full,
void *data);

static MuError
process_file (const char* fullpath, const gchar* mdir,
MuMaildirWalkMsgCallback msg_cb, void *data)
{
MuError result;
struct stat statbuf;

if (!msg_cb)
return MU_OK;

if (G_UNLIKELY(access(fullpath, R_OK) != 0)) {
g_warning ("cannot access %s: %s", fullpath,
strerror(errno));
return MU_ERROR;
}

if (G_UNLIKELY(stat (fullpath, &statbuf) != 0)) {
g_warning ("cannot stat %s: %s", fullpath, strerror(errno));
return MU_ERROR;
}

result = (msg_cb)(fullpath, mdir, &statbuf, data);
if (result == MU_STOP)
g_debug ("callback said 'MU_STOP' for %s", fullpath);
else if (result == MU_ERROR)
g_warning ("%s: error in callback (%s)",
__FUNCTION__, fullpath);

return result;
}


/ *
* determine if path is a maildir leaf-dir; ie. if it's 'cur' or 'new'
* (we're skipping 'tmp' for obvious reasons)
* /
G_GNUC_CONST static gboolean
is_maildir_new_or_cur (const char *path)
{
size_t len;

g_return_val_if_fail (path, FALSE);

/ * path is the full path; it cannot possibly be shorter
* than 4 for a maildir (/cur or /new) * /
len = strlen (path);
if (G_UNLIKELY(len < 4))
return FALSE;

/ * optimization; one further idea would be cast the 4 bytes to an integer
* and compare that -- need to think about alignment, endianness * /

if (path[len - 4] == G_DIR_SEPARATOR &&
path[len - 3] == 'c' &&
path[len - 2] == 'u' &&
path[len - 1] == 'r')
return TRUE;

if (path[len - 4] == G_DIR_SEPARATOR &&
path[len - 3] == 'n' &&
path[len - 2] == 'e' &&
path[len - 1] == 'w')
return TRUE;

return FALSE;
}


/ * check if there path contains file; used for checking if there is
* MU_MAILDIR_NOINDEX_FILE or MU_MAILDIR_NOUPDATE_FILE in this
* dir; * /
static gboolean dir_contains_file (const char *path, const char *file)
{
const char* fullpath;

// static buffer 
fullpath = mu_str_fullpath_s (path, file);

if (access (fullpath, F_OK) == 0)
return TRUE;
else if (G_UNLIKELY(errno != ENOENT && errno != EACCES))
g_warning ("error testing for %s/%s: %s",
fullpath, file, strerror(errno));
return FALSE;
}

static gboolean is_dotdir_to_ignore (const char* dir)
{
int i;
const char* ignore[] = {
".notmuch",
".nnmaildir",
".#evolution"
}; // when adding names, check the optimization below 

if (dir[0] != '.')
return FALSE; // not a dotdir 

if (dir[1] == '\0' || (dir[1] == '.' && dir[2] == '\0'))
return TRUE; // ignore '.' and '..' 

// optimization: special dirs have 'n' or '#' in pos 1 
if (dir[1] != 'n' && dir[1] != '#')
return FALSE; // not special: don't ignore 

for (i = 0; i != G_N_ELEMENTS(ignore); ++i)
if (strcmp(dir, ignore[i]) == 0)
return TRUE;

return FALSE; // don't ignore 
}

static gboolean
ignore_dir_entry (struct dirent *entry, unsigned char d_type)
{
if (G_LIKELY(d_type == DT_REG)) {

// ignore emacs tempfiles 
if (entry->d_name[0] == '#')
return TRUE;
// ignore dovecot metadata 
if (entry->d_name[0] == 'd' &&
strncmp (entry->d_name, "dovecot", 7) == 0)
return TRUE;
// ignore special files 
if (entry->d_name[0] == '.')
return TRUE;
// ignore core files 
if (entry->d_name[0] == 'c' &&
strncmp (entry->d_name, "core", 4) == 0)
return TRUE;

return FALSE; // other files: don't ignore 

} else if (d_type == DT_DIR)
return is_dotdir_to_ignore (entry->d_name);
else
return TRUE; // ignore non-normal files, non-dirs 
}

/ *
* return the maildir value for the the path - this is the directory
* for the message (with the top-level dir as "/"), and without the
* leaf "/cur" or "/new". In other words, contatenate old_mdir + "/" + dir,
* unless dir is either 'new' or 'cur'. The value will be used in queries.
* /
static gchar* get_mdir_for_path (const gchar *old_mdir, const gchar *dir)
{
/ * if the current dir is not 'new' or 'cur', contatenate
* old_mdir an dir * /
if ((dir[0] == 'n' && strcmp(dir, "new") == 0) ||
(dir[0] == 'c' && strcmp(dir, "cur") == 0) ||
(dir[0] == 't' && strcmp(dir, "tmp") == 0))
return strdup (old_mdir ? old_mdir : G_DIR_SEPARATOR_S);
else
return g_strconcat (old_mdir ? old_mdir : "",
G_DIR_SEPARATOR_S, dir, NULL);

}


static MuError
process_dir_entry (const char* path, const char* mdir, struct dirent *entry,
MuMaildirWalkMsgCallback cb_msg,
MuMaildirWalkDirCallback cb_dir,
gboolean full, void *data)
{
const char *fp;
char* fullpath;
unsigned char d_type;

/ * we have to copy the buffer from fullpath_s, because it
* returns a static buffer, and we maybe called reentrantly * /
fp = mu_str_fullpath_s (path, entry->d_name);
fullpath = g_newa (char, strlen(fp) + 1);
strcpy (fullpath, fp);

d_type = GET_DTYPE(entry, fullpath);

// ignore special files/dirs 
if (ignore_dir_entry (entry, d_type))
return MU_OK;

switch (d_type) {
case DT_REG: // we only want files in cur/ and new/ 
if (!is_maildir_new_or_cur (path))
return MU_OK;

return process_file (fullpath, mdir, cb_msg, data);

case DT_DIR: {
char *my_mdir;
MuError rv;
/ * my_mdir is the search maildir (the dir starting
* with the top-level maildir as /, and without the
* /tmp, /cur, /new * /
my_mdir = get_mdir_for_path (mdir, entry->d_name);
rv = process_dir (fullpath, my_mdir, cb_msg, cb_dir, full, data);
g_free (my_mdir);

return rv;
}

default:
return MU_OK; // ignore other types 
}
}


static const size_t DIRENT_ALLOC_SIZE =
offsetof (struct dirent, d_name) + PATH_MAX;

static struct dirent*
dirent_new (void)
{
return (struct dirent*) g_slice_alloc (DIRENT_ALLOC_SIZE);
}


static void
dirent_destroy (struct dirent *entry)
{
g_slice_free1 (DIRENT_ALLOC_SIZE, entry);
}

#ifdef HAVE_STRUCT_DIRENT_D_INO
static int
dirent_cmp (struct dirent *d1, struct dirent *d2)
{
/ * we do it his way instead of a simple d1->d_ino - d2->d_ino
* because this way, we don't need 64-bit numbers for the
* actual sorting * /
if (d1->d_ino < d2->d_ino)
return -1;
else if (d1->d_ino > d2->d_ino)
return 1;
else
return 0;
}
#endif // HAVE_STRUCT_DIRENT_D_INO

static MuError
process_dir_entries (DIR *dir, const char* path, const char* mdir,
MuMaildirWalkMsgCallback msg_cb,
MuMaildirWalkDirCallback dir_cb,
gboolean full, void *data)
{
MuError result;
GSList *lst, *c;

for (lst = NULL;;) {
int rv;
struct dirent *entry, *res;
entry = dirent_new ();
rv = readdir_r (dir, entry, &res);
if (rv == 0) {
if (res)
lst = g_slist_prepend (lst, entry);
else {
dirent_destroy (entry);
break; // last direntry reached 
}
} else {
dirent_destroy (entry);
g_warning ("error scanning dir: %s", strerror(rv));
return MU_ERROR_FILE;
}
}

/ * we sort by inode; this makes things much faster on
* extfs2,3 * /
#if HAVE_STRUCT_DIRENT_D_INO
c = lst = g_slist_sort (lst, (GCompareFunc)dirent_cmp);
#endif // HAVE_STRUCT_DIRENT_D_INO

for (c = lst, result = MU_OK; c && result == MU_OK; c = g_slist_next(c))
result = process_dir_entry (path, mdir, (struct dirent*)c->data,
msg_cb, dir_cb, full, data);

g_slist_foreach (lst, (GFunc)dirent_destroy, NULL);
g_slist_free (lst);

return result;
}


static MuError
process_dir (const char* path, const char* mdir,
MuMaildirWalkMsgCallback msg_cb, MuMaildirWalkDirCallback dir_cb,
gboolean full, void *data)
{
MuError result;
DIR* dir;

// if it has a noindex file, we ignore this dir 
if (dir_contains_file (path, MU_MAILDIR_NOINDEX_FILE) ||
(!full && dir_contains_file (path, MU_MAILDIR_NOUPDATE_FILE))) {
g_debug ("found noindex/noupdate: ignoring dir %s", path);
return MU_OK;
}

dir = opendir (path);
if (!dir) {
g_warning ("cannot access %s: %s", path, strerror(errno));
return MU_OK;
}

if (dir_cb) {
MuError rv;
rv = dir_cb (path, TRUE, data);
if (rv != MU_OK) {
closedir (dir);
return rv;
}
}

result = process_dir_entries (dir, path, mdir, msg_cb, dir_cb, full, data);
closedir (dir);

// only run dir_cb if it exists and so far, things went ok 
if (dir_cb && result == MU_OK)
return dir_cb (path, FALSE, data);

return result;
}


MuError
mu_maildir_walk (const char *path, MuMaildirWalkMsgCallback cb_msg,
MuMaildirWalkDirCallback cb_dir, gboolean full,
void *data)
{
MuError rv;
char *mypath;

g_return_val_if_fail (path && cb_msg, MU_ERROR);
g_return_val_if_fail (mu_util_check_dir(path, TRUE, FALSE), MU_ERROR);

// strip the final / or \ 
mypath = g_strdup (path);
if (mypath[strlen(mypath)-1] == G_DIR_SEPARATOR)
mypath[strlen(mypath)-1] = '\0';

rv = process_dir (mypath, NULL, cb_msg, cb_dir, full, data);
g_free (mypath);

return rv;
}


static gboolean
clear_links (const gchar* dirname, DIR *dir, GError **err)
{
struct dirent *entry;
gboolean rv;

rv = TRUE;
errno = 0;
while ((entry = readdir (dir))) {

const char *fp;
char *fullpath;
unsigned char d_type;

// ignore empty, dot thingies 
if (!entry->d_name || entry->d_name[0] == '.')
continue;

/ * we have to copy the buffer from fullpath_s, because
* it returns a static buffer and we are
* recursive * /
fp = mu_str_fullpath_s (dirname, entry->d_name);
fullpath = g_newa (char, strlen(fp) + 1);
strcpy (fullpath, fp);

d_type = GET_DTYPE (entry, fullpath);

// ignore non-links / non-dirs 
if (d_type != DT_LNK && d_type != DT_DIR)
continue;

if (d_type == DT_LNK) {
if (unlink (fullpath) != 0) {
// don't use err 
g_warning ("error unlinking %s: %s",
fullpath, strerror(errno));
rv = FALSE;
}
} else // DT_DIR, see check before
rv = mu_maildir_clear_links (fullpath, err);
}

if (errno != 0)
mu_util_g_set_error (err, MU_ERROR_FILE,
"file error: %s", strerror(errno));

return (rv == FALSE && errno == 0);
}


gboolean
mu_maildir_clear_links (const gchar* path, GError **err)
{
DIR *dir;
gboolean rv;

g_return_val_if_fail (path, FALSE);

dir = opendir (path);
if (!dir)
return mu_util_g_set_error (err, MU_ERROR_FILE_CANNOT_OPEN,
"failed to open %s: %s", path,
strerror(errno));

rv = clear_links (path, dir, err);
closedir (dir);

return rv;
}

MuFlags
mu_maildir_get_flags_from_path (const char *path)
{
g_return_val_if_fail (path, MU_FLAG_INVALID);

// try to find the info part 
/ * note that we can use either the ':' or '!' as separator;
* the former is the official, but as it does not work on e.g. VFAT
* file systems, some Maildir implementations use the latter instead
* (or both). For example, Tinymail/modest does this. The python
* documentation at http://docs.python.org/lib/mailbox-maildir.html
* mentions the '!' as well as a 'popular choice'
* /

// we check the dir -- 
if (strstr (path, G_DIR_SEPARATOR_S "new" G_DIR_SEPARATOR_S)) {

char *dir, *dir2;
MuFlags flags;

dir = g_path_get_dirname (path);
dir2 = g_path_get_basename (dir);

if (g_strcmp0 (dir2, "new") == 0)
flags = MU_FLAG_NEW;

g_free (dir);
g_free (dir2);

/ * NOTE: new/ message should not have :2,-stuff, as
* per http://cr.yp.to/proto/maildir.html. If they, do
* we ignore it
* /
if (flags == MU_FLAG_NEW)
return flags;
}

// get the file flags 
{
char *info;

info = strrchr (path, '2');
if (!info || info == path ||
(info[-1] != ':' && info[-1] != '!') ||
(info[1] != ','))
return MU_FLAG_NONE;
else
return mu_flags_from_str
(&info[2], MU_FLAG_TYPE_MAILFILE,
TRUE); // ignore invalid 
}
}


/ *
* take an exising message path, and return a new path, based on
* whether it should be in 'new' or 'cur'; ie.
*
* /home/user/Maildir/foo/bar/cur/abc:2,F and flags == MU_FLAG_NEW
* => /home/user/Maildir/foo/bar/new
* and
* /home/user/Maildir/foo/bar/new/abc and flags == MU_FLAG_REPLIED
* => /home/user/Maildir/foo/bar/cur
*
* so the difference is whether MuFlags matches MU_FLAG_NEW is set or
* not; and in the latter case, no other flags are allowed.
*
* /
static gchar*
get_new_path (const char *mdir, const char *mfile, MuFlags flags,
const char* custom_flags)
{
if (flags & MU_FLAG_NEW)
return g_strdup_printf ("%s%cnew%c%s",
mdir, G_DIR_SEPARATOR, G_DIR_SEPARATOR,
mfile);
else {
const char *flagstr;
flagstr = mu_flags_to_str_s (flags, MU_FLAG_TYPE_MAILFILE);

return g_strdup_printf ("%s%ccur%c%s:2,%s%s",
mdir, G_DIR_SEPARATOR, G_DIR_SEPARATOR,
mfile, flagstr,
custom_flags ? custom_flags : "");
}
}


char*
mu_maildir_get_maildir_from_path (const char* path)
{
gchar *mdir;

// determine the maildir 
mdir = g_path_get_dirname (path);
if (!g_str_has_suffix (mdir, "cur") &&
!g_str_has_suffix (mdir, "new")) {
g_warning ("%s: not a valid maildir path: %s",
__FUNCTION__, path);
g_free (mdir);
return NULL;
}

// remove the 'cur' or 'new'
mdir[strlen(mdir) - 4] = '\0';

return mdir;
}



char*
mu_maildir_get_new_path (const char *oldpath, const char *new_mdir,
MuFlags newflags)
{
char *mfile, *mdir, *custom_flags, *newpath, *cur;

g_return_val_if_fail (oldpath, NULL);

mfile = newpath = custom_flags = NULL;

// determine the maildir 
mdir = mu_maildir_get_maildir_from_path (oldpath);
if (!mdir)
return NULL;

/ * determine the name of the mailfile, stripped of its flags, as well
* as any custom (non-standard) flags * /
mfile = g_path_get_basename (oldpath);
for (cur = &mfile[strlen(mfile)-1]; cur > mfile; --cur) {
if ((*cur == ':' || *cur == '!') &&
(cur[1] == '2' && cur[2] == ',')) {
// get the custom flags (if any)
custom_flags = mu_flags_custom_from_str (cur + 3);
cur[0] = '\0'; // strip the flags 
break;
}
}

newpath = get_new_path (new_mdir ? new_mdir : mdir,
mfile, newflags, custom_flags);
g_free (mfile);
g_free (mdir);
g_free (custom_flags);

return newpath;
}

static gboolean
msg_move_check_pre (const gchar *src, const gchar *dst, GError **err)
{
if (!g_path_is_absolute(src))
return mu_util_g_set_error
(err, MU_ERROR_FILE,
"source is not an absolute path: '%s'", src);

if (!g_path_is_absolute(dst))
return mu_util_g_set_error
(err, MU_ERROR_FILE,
"target is not an absolute path: '%s'", dst);

if (access (src, R_OK) != 0)
return mu_util_g_set_error (err, MU_ERROR_FILE,
"cannot read %s", src);

if (access (dst, F_OK) == 0)
return mu_util_g_set_error (err, MU_ERROR_FILE,
"%s already exists", dst);

return TRUE;
}

static gboolean
msg_move_check_post (const char *src, const char *dst, GError **err)
{
// double check -- is the target really there? 
if (access (dst, F_OK) != 0)
return mu_util_g_set_error
(err, MU_ERROR_FILE, "can't find target (%s)", dst);

if (access (src, F_OK) == 0)
return mu_util_g_set_error
(err, MU_ERROR_FILE, "source still there (%s)", src);

return TRUE;
}


static gboolean
msg_move (const char* src, const char *dst, GError **err)
{
if (!msg_move_check_pre (src, dst, err))
return FALSE;

if (rename (src, dst) != 0)
return mu_util_g_set_error
(err, MU_ERROR_FILE,"error moving %s to %s", src, dst);

return msg_move_check_post (src, dst, err);
}

gchar*
mu_maildir_move_message (const char* oldpath, const char* targetmdir,
MuFlags newflags, gboolean ignore_dups,
GError **err)
{
char *newfullpath;
gboolean rv;
gboolean src_is_target;

g_return_val_if_fail (oldpath, FALSE);

newfullpath = mu_maildir_get_new_path (oldpath, targetmdir,
newflags);
if (!newfullpath) {
mu_util_g_set_error (err, MU_ERROR_FILE,
"failed to determine targetpath");
return NULL;
}

src_is_target = (g_strcmp0 (oldpath, newfullpath) == 0);

if (!ignore_dups && src_is_target) {
mu_util_g_set_error (err, MU_ERROR_FILE_TARGET_EQUALS_SOURCE,
"target equals source");
return NULL;
}

if (!src_is_target) {
rv = msg_move (oldpath, newfullpath, err);
if (!rv) {
g_free (newfullpath);
return NULL;
}
}

return newfullpath;
}
*/

