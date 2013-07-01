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
 * Functions regarding Xml files
 * NB: Xml file is used to keep trace of Context for user
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
#include <libxml/parser.h>
#include <sys/types.h>
#include <dirent.h>

#define isFile 0x8
#define isDir  0x4


/*
 * \detail  Create Contexts.xml file for a user
 *
 * This function is called once on first backend launch,
 * the function defines main contexts used by Openchange
 *
 * \param *mem_ctx						memory context
 * \param *ContextsXml				fullname of Xml file to create
 */
int CreateContextsXml(TALLOC_CTX *mem_ctx, char *ContextsXml)
{
xmlDocPtr 	doc = NULL;       	// document pointer 
xmlNodePtr 	root_node = NULL, context_node = NULL;		// node pointers 
int i;
char *Url, *iRole;
char *ContextRole[] = {"INBOX","INBOX/Drafts", "INBOX/Sent", "INBOX/Outbox","INBOX/Trash","CALENDAR","CONTACTS","TASKS",
                     "NOTES","JOURNAL","FALLBACK"};
char *NameRole[] = {"Inbox","Drafts", "Sent", "Outbox","Trash","Calendar","Contacts","Tasks",
                     "Notes","Journal","Fallback"};
enum mapistore_context_role  Roles[]= {MAPISTORE_MAIL_ROLE, 	MAPISTORE_DRAFTS_ROLE, 	MAPISTORE_SENTITEMS_ROLE,
                     MAPISTORE_OUTBOX_ROLE, MAPISTORE_DELETEDITEMS_ROLE, MAPISTORE_CALENDAR_ROLE,
                     MAPISTORE_CONTACTS_ROLE, MAPISTORE_TASKS_ROLE, MAPISTORE_NOTES_ROLE, MAPISTORE_JOURNAL_ROLE,
                     MAPISTORE_FALLBACK_ROLE, MAPISTORE_MAX_ROLES };

// Creates a new document, a node and set it as a root node
doc = xmlNewDoc(BAD_CAST "1.0");
root_node = xmlNewNode(NULL, BAD_CAST "Contexts");
xmlDocSetRootElement(doc, root_node);

for(i=0 ; i<MAPISTORE_MAX_ROLES ; i++ )
  {
  context_node = xmlNewChild(root_node, NULL, BAD_CAST "Context", NULL);  
  xmlSetProp(context_node, BAD_CAST "name", BAD_CAST NameRole[i]);
  xmlNewChild(context_node, NULL, BAD_CAST "Main", BAD_CAST "true");
  Url = talloc_asprintf(mem_ctx,"EasyLinux://%s",ContextRole[i]);
  xmlNewChild(context_node, NULL, BAD_CAST "Url", BAD_CAST Url);
  talloc_free(Url);
  xmlNewChild(context_node, NULL, BAD_CAST "Tag", BAD_CAST "tag");  
  iRole = talloc_asprintf(mem_ctx,"%i",Roles[i]);
  xmlNewChild(context_node, NULL, BAD_CAST "Role", BAD_CAST iRole);  
  talloc_free(iRole);
  }

// Dumping document to file
xmlSaveFormatFileEnc(ContextsXml, doc, "UTF-8", 1);
xmlFreeDoc(doc);
xmlCleanupParser();
xmlMemoryDump();

return MAPISTORE_SUCCESS;
}


/*
 * 
 */
int AddXmlContext(TALLOC_CTX *mem_ctx, char *XmlFile, char *ContextName, char *Main, char *Url,  char *Tag)
{
xmlDocPtr xmldoc = NULL;
xmlNodePtr root_node, context_node, n, m;
char *iRole;
xmlChar *Content;
int Role=0, Val;

xmldoc = xmlParseFile(XmlFile);
root_node = xmlDocGetRootElement(xmldoc);
// We need to allocate next RoleID
for (n = root_node->children; n != NULL; n = n->next) 
  { 
  if( n->type == XML_ELEMENT_NODE)
    {
  	// retrieve role informations 
    for( m = n->children ; m != NULL ; m = m->next )
      {
      if( m->type == XML_ELEMENT_NODE )
        {
        Content = xmlNodeGetContent(m);
        if( strcmp((char *)m->name,"Role") == 0 )
          {
          Val = atoi((char *)Content);
          if( Val > Role )
            Role = Val;
          }
        xmlFree(Content);
        }
      }
    }
  }
// Role contains now max(Role)
Role++;

context_node = xmlNewChild(root_node, NULL, BAD_CAST "Context", NULL);  
xmlSetProp(context_node, BAD_CAST "name", BAD_CAST ContextName);
xmlNewChild(context_node, NULL, BAD_CAST "Main", BAD_CAST Main);
xmlNewChild(context_node, NULL, BAD_CAST "Url", BAD_CAST Url);
xmlNewChild(context_node, NULL, BAD_CAST "Tag", BAD_CAST Tag);  
iRole = talloc_asprintf(mem_ctx,"%i",Role);
xmlNewChild(context_node, NULL, BAD_CAST "Role", BAD_CAST iRole);  
talloc_free(iRole);

// Dumping document to file
xmlSaveFormatFileEnc(XmlFile, xmldoc, "UTF-8", 1);
xmlFreeDoc(xmldoc);
xmlCleanupParser();
xmlMemoryDump();

return MAPISTORE_SUCCESS;
}

/*
 * \detail List Context for user
 * This function is called during backend initialization, 
 * when a user has been already connected, he has a Contexts.xml in his MAPI personnal folder.
 * This function opens the file and fill the mapistore_contexts_list structure with relevant 
 * datas
 *
 * \param *mem_ctx						memory context
 * \param *XmlFile  					fullname of Xml file to open
 * \param **contexts_listp		context list returned by the function
 */
int ListContextsFromXml(TALLOC_CTX *mem_ctx, char *XmlFile, struct mapistore_contexts_list **contexts_listp)
{
xmlDocPtr xmldoc = NULL;
xmlNodePtr racine;
xmlNodePtr n, m;
xmlChar *cName, *Content;
struct mapistore_contexts_list *Context, *PrevContext;


xmldoc = xmlParseFile(XmlFile);
if (!xmldoc)
  {
  DEBUG(0, ("ERROR - MAPIEasyLinux : can't open xml file '%s' \n",XmlFile));
  return MAPISTORE_ERR_BACKEND_INIT;
  }
   
// Get to xml root
racine = xmlDocGetRootElement(xmldoc);
if (racine == NULL) 
  {
  DEBUG(0, ("ERROR - MAPIEasyLinux : Document XML is empty !\n"));
  xmlFreeDoc(xmldoc);
  return MAPISTORE_ERR_BACKEND_INIT;
  }
DEBUG(3 ,("MAPIEasyLinux :     --> %s\n", racine->name));

// Scan contexts
PrevContext = NULL;
for (n = racine->children; n != NULL; n = n->next) 
  { 
  if( n->type == XML_ELEMENT_NODE)
    {
    Context = (struct mapistore_contexts_list *)talloc_zero_size(mem_ctx, sizeof(struct mapistore_contexts_list) );
  	Context->prev	= PrevContext;

  	if( PrevContext != NULL )
  	  PrevContext->next = Context;
  	else
  	  *contexts_listp = Context;
  	  
  	PrevContext = Context;  // Prepare for next Context
  	Context->next	= NULL;
  	
  	// retrieve informations and fill structure
    cName = xmlGetProp(n, (const xmlChar*)"name");
    Context->name	= talloc_strdup(mem_ctx, (char *)cName);
    DEBUG(0 ,("MAPIEasyLinux :       --> %s\n", cName));
    for( m = n->children ; m != NULL ; m = m->next )
      {
      if( m->type == XML_ELEMENT_NODE )
        {
        Content = xmlNodeGetContent(m);
        if( strcmp((char *)m->name,"Url") == 0 )
          Context->url 				 		= talloc_strdup(mem_ctx,(char *)Content);
        if( (strcmp((char *)m->name,"Main") == 0) && (strcmp((char *)Content, "true") == 0) )
          Context->main_folder 		= true;
        if( (strcmp((char *)m->name,"Main") == 0) && (strcmp((char *)Content, "true") != 0) )
          Context->main_folder 		= false;
        if( strcmp((char *)m->name,"Tag") == 0 )
          Context->tag 				 		= talloc_strdup(mem_ctx,(char *)Content);
        if( strcmp((char *)m->name,"Role") == 0 )
          Context->role 				 		= atoi((char *)Content);
        DEBUG(3, ("MAPIEasyLinux :        --> %s - %s\n", m->name, Content));
        xmlFree(Content);
        }
      }
    xmlFree(cName);
    }
  }

xmlFreeDoc (xmldoc);
return MAPISTORE_SUCCESS;
}




























int SaveMessageXml(struct EasyLinuxMessage *elMessage, TALLOC_CTX *mem_ctx)
{
int i;
DEBUG(0, ("MAPIEasyLinux : Saving XML Message\n"));
DEBUG(0, ("MAPIEasyLinux :     MID: %lX\n",elMessage->MID));
DEBUG(0, ("MAPIEasyLinux :     Folder: %s\n",elMessage->Parent->RelPath));
DEBUG(0, ("MAPIEasyLinux :     File: %s\n",elMessage->Parent->FullPath));
DEBUG(0, ("MAPIEasyLinux :     Table with %i records\n",elMessage->Table->rowCount));

return 0;
}




/*
 * Close xml file, if needed save actual memory
 *
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




for(i=0 ; i<elMessage->Table->rowCount ; i++)
  {
  DEBUG(0, ("MAPIEasyLinux :         0x%08X: %s\n",elMessage->Table->Props[i]->PropTag, elMessage->Table->Props[i]->PropValue));
  }






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

return 0;
}

*
 * Create an empty Xml file
 *
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
} */

