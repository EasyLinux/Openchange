/*
*/
 
enum EasyLinux_Struct_Type {
	EASYLINUX_USER=(int)1,
	EASYLINUX_FOLDER=(int)2,
	EASYLINUX_MSG=(int)3,
	EASYLINUX_TABLE=(int)4,
	EASYLINUX_BACKEND=(int)5
};

  
struct EasyLinuxUser {
  enum EasyLinux_Struct_Type		stType;
	char 													*uid;													// uid utilisateur
	char    											*displayName;									// displayName from Ldap
	int														uidNumber;
	int														gidNumber;
	char													*homeDirectory;
	char 													*Server;
	};
	
struct EasyLinuxFolder {
  enum EasyLinux_Struct_Type			stType;
  int															Valid;
	char 														*Uri;
	uint64_t												FID;
	char 														*displayName;
	struct EasyLinuxBackendContext  *elContext;
	};

struct EasyLinuxMessage {
  enum EasyLinux_Struct_Type			stType;
	uint64_t												MID;
	uint8_t 												associated;
	char 														*displayName;
	struct EasyLinuxFolder					*parent_folder;
	struct EasyLinuxBackendContext  *elContext;
	};
	  
struct EasyLinuxTable {
  enum EasyLinux_Struct_Type			stType;
	int															IdTable;
	int															tType;
	uint32_t												rowCount;
	char 														*displayName;
	struct EasyLinuxFolder					*parent_folder;
	struct EasyLinuxBackendContext  *elContext;
	};
	
struct EasyLinuxBackendContext {
  enum EasyLinux_Struct_Type			stType;
  TALLOC_CTX 											*mem_ctx;
	struct EasyLinuxUser	  				User;
	struct EasyLinuxFolder  				RootFolder;
//	struct EasyLinuxMessage Message;
//	struct EasyLinuxTable   Table;
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
 * EasyLinux_Ldap_Funcs functions
 */
int SetUserInformation(struct EasyLinuxBackendContext *, TALLOC_CTX *, char *, struct ldb_context *);
int InitialiseRootFolder(struct EasyLinuxBackendContext *, TALLOC_CTX *, char *, struct ldb_context *,const char *);

/*
 * EasyLinux_Maildir functions
 */
int GetMaildirChildCount(struct EasyLinuxFolder *, uint32_t);
int CreateXml(struct EasyLinuxBackendContext *, char *);
int RecursiveMkDir(struct EasyLinuxBackendContext *, char *, mode_t);
int CreateXmlFile(struct EasyLinuxBackendContext *, char *);

