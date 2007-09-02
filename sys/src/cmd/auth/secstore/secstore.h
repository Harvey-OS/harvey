#define LOG		"secstore"
#define SECSTORE_DIR	"/adm/secstore"

enum {
	MAXFILESIZE = 10*1024*1024,
};

/* PW status bits */
enum {
	Enabled 	= 1<<0,
	STA 		= 1<<1,	/* extra SecurID step */
};

typedef struct PW {
	char	*id;		/* user id */
	ulong	expire;		/* expiration time (epoch seconds) */
	ushort	status;		/* Enabled, STA, ... */
	ushort	failed;		/* number of failed login attempts */
	char	*other;		/* other information, e.g. sponsor */
	mpint	*Hi;  		/* H(passphrase)^-1 mod p */
} PW;

void	freePW(PW*);
PW	*getPW(char*, int);
char	*getpassm(char*);
int	putPW(PW*);
char	*validatefile(char*f);

/*
 * *client: SConn, client name, passphrase
 * *server: SConn, (partial) 1st msg, PW entry
 * *setpass: Username, hashed passphrase, PW entry
 */
int	PAKclient(SConn*, char*, char*, char**);
int	PAKserver(SConn*, char*, char*, PW**);
char*	PAK_Hi(char*, char*, mpint*, mpint*);
