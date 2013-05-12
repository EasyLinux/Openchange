/*
*/
 
enum EasyLinux_Struct_Type {
	EASYLINUX_USER=(int)1,
	EASYLINUX_FOLDER=(int)2,
	EASYLINUX_MSG=(int)3,
	EASYLINUX_TABLE=(int)4,
	EASYLINUX_BACKEND=(int)5
};

enum EasyLinux_Backend_Type {
	EASYLINUX_FALLBACK=(int)1,
	EASYLINUX_MAILDIR=(int)2,
	EASYLINUX_CALENDAR=(int)3,
	EASYLINUX_CONTACTS=(int)4,
	EASYLINUX_TASKS=(int)5,
	EASYLINUX_NOTES=(int)6,
	EASYLINUX_JOURNAL=(int)7,
	EASYLINUX_UNKNOW=(int)999
	};
  
struct EasyLinuxUser {
  enum EasyLinux_Struct_Type			stType;
	char 														*uid;													// uid utilisateur
	char    												*displayName;									// displayName from Ldap
	int															uidNumber;
	int															gidNumber;
	char														*homeDirectory;
	char 														*Server;
	};
	
struct EasyLinuxFolder {
  enum EasyLinux_Struct_Type			stType;
	char 														*Uri;
	uint64_t												FID;
	char 														*displayName;
	char 														*Comment;
	char 														*RelPath;
	char														*FullPath;
	int 														FolderType;   // FOLDER_ROOT=0x0, FOLDER_GENERIC=0x1, FOLDER_SEARCH=0x2
	struct EasyLinuxFolder					*Parent;
	struct EasyLinuxTable           *Table;
	struct EasyLinuxContext         *elContext;
	};

struct EasyLinuxMessage {
  enum EasyLinux_Struct_Type			stType;
	uint64_t												MID;
	uint8_t 												associated;
	char 														*displayName;
	char														*MessageClass;
	struct EasyLinuxFolder					*parent_folder;
	struct EasyLinuxTable           *Table;
	struct EasyLinuxContext         *elContext;
	};

/*
 *   Table represent a data in Openchange 
 *
 *   stType                 identify structure as Table (needed when void * is use by function)
 *   elParent               point to parent (Folder or Message) object
 *   elContext							point to Context object
 *   rowCount               number of records in Table
 *   Props                  point to Properties
 */
	  
struct EasyLinuxTable {
  enum EasyLinux_Struct_Type			stType;
	void                  					*elParent;
	struct EasyLinuxContext         *elContext;
	uint32_t												rowCount;
	struct EasyLinuxProp						*Props;
	};
	
struct EasyLinuxProp {
	uint32_t												PropTag;
	int															PropType;
	void														*PropValue;
	};	
	
	
	
struct EasyLinuxContext {
  enum EasyLinux_Struct_Type			stType;
  enum EasyLinux_Backend_Type			bkType;
  TALLOC_CTX 											*mem_ctx;
  void														*bkContext;
	struct EasyLinuxUser	  				User;
	struct EasyLinuxFolder  				RootFolder;
	struct EasyLinuxTable           Table;
	};
	
struct EasyLinuxGeneric {
  enum EasyLinux_Struct_Type			stType;
  };	

// - a backend object
// - a context object
// - a folder object
// - a message object
// - etc.
	
	
	
	
	
	
/*
 * MAPIStoreEasyLinux functions
 */
int mapistore_init_backend(void);
 	
/*
 * EasyLinux_Common functions
 */
enum EasyLinux_Backend_Type GetBkType(char *);
int RecursiveMkDir(struct EasyLinuxContext *, char *, mode_t);
void Dump(void *);
  	
/*
 * EasyLinux_Ldap functions
 */
int SetUserInformation(struct EasyLinuxContext *, TALLOC_CTX *, char *, struct ldb_context *);
int InitialiseRootFolder(struct EasyLinuxContext *, TALLOC_CTX *, char *, struct ldb_context *,const char *);

/*
 * EasyLinux Calendar functions 
 */
void OpenCalendar(struct EasyLinuxContext *);
int  GetCalendars(struct EasyLinuxFolder *);
 
/*
 * EasyLinux Notes functions 
 */
void OpenNotes(struct EasyLinuxContext *);
int  GetNotes(struct EasyLinuxFolder *);

/*
 * EasyLinux Tasks functions 
 */
void OpenTasks(struct EasyLinuxContext *);
int  GetTasks(struct EasyLinuxFolder *);

/*
 * EasyLinux Contacts functions 
 */
void OpenContacts(struct EasyLinuxContext *);
int  GetContacts(struct EasyLinuxFolder *);

/*
 * EasyLinux Journal functions 
 */
void OpenJournal(struct EasyLinuxContext *);
int  GetJournal(struct EasyLinuxFolder *);

/*
 * EasyLinux_Maildir functions
 */
enum mapistore_error OpenMailDir(struct EasyLinuxContext *);
int OmmitSpecialFolder(struct EasyLinuxFolder *, char *);
int GetMaildirChildCount(struct EasyLinuxFolder *, uint32_t);
enum mapistore_error MailDirCreateFolder(struct EasyLinuxFolder *);
char *ImapToMaildir(TALLOC_CTX *, char *);

/*
 *  EasyLinux_Xml functions
 */
int OpenFallBack(struct EasyLinuxContext *);
int CreateXmlFile(struct EasyLinuxContext *);
int SaveMessageXml(struct EasyLinuxMessage *, TALLOC_CTX *);
int CloseXml(struct EasyLinuxContext *);

