/*
*/
  
struct EasyLinuxUser {
	char 				*uid;
	char    		*displayName;
	int					uidNumber;
	int					gidNumber;
	char				*homeDirectory;
	char 				*Server;
	};
	
struct EasyLinuxFolder {
  int					Valid;
	char 				*Uri;
	uint64_t		FID;
	char 				*displayName;
	};
	  
struct EasyLinuxBackendContext {
  int 										Test;
	struct EasyLinuxUser	  User;
	struct EasyLinuxFolder  Folder;
	};
	
/*
 * MAPIStoreEasyLinux functions
 */
int mapistore_init_backend(void);
 	
/*
 * EasyLinux_Ldap_Funcs functions
 */
int SetUserInformation(struct EasyLinuxBackendContext *, TALLOC_CTX *, char *, struct ldb_context *);
int InitialiseRootFolder(struct EasyLinuxBackendContext *, TALLOC_CTX *, char *, struct ldb_context *,const char *);



