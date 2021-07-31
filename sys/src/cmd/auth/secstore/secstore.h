enum{ MAXFILESIZE = 10*1024*1024 };

enum{// PW status bits
	Enabled 	= (1<<0),
	STA 		= (1<<1),	// extra SecurID step
};

typedef struct PW {
	char *id;		// user id
	ulong expire;	// expiration time (epoch seconds)
	ushort status;	// Enabled, STA, ...
	ushort failed;	// number of failed login attempts
	char *other;	// other information, e.g. sponsor
	mpint *Hi;  	// H(passphrase)^-1 mod p
} PW;

PW *getPW(char *);
int putPW(PW *);
void freePW(PW *);
int getpasswd(char*, char*, int);

// *client: SConn, client name, passphrase
// *server: SConn, (partial) 1st msg, PW entry
// *setpass: Username, hashed passphrase, PW entry
int PAKclient(SConn *, char *, char *, char **);
int PAKserver(SConn *, char *, char *, PW **);
char *PAK_Hi(char *, char *, mpint *, mpint *);

#define LOG "secstore"
#define SECSTORE_DIR	"/adm/secstore"
