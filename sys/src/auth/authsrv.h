#pragma lib "./libauth.$O"
enum{
	PASSLEN		= 10,
	MAXNETCHAL	= 100000,		/* max securenet challenge */
};

#define	KEYDB		"/mnt/keys"
#define NETKEYDB	"/mnt/netkeys"
#define KEYDBBUF	(sizeof NETKEYDB)	/* enough for any keydb prefix */
#define AUTHLOG		"auth"

int	encrypt(void*, void*, int);
int	decrypt(void*, void*, int);
char	*findkey(char*);
char	*findnetkey(char*);
int	ishost(char*);
char	*netresp(char*, long, char*);
char	*netdecimal(char*);
int	netcheck(char*, long, char*);
int	mkkey(char[DESKEYLEN], char*);
int	okpasswd(char*);
void	logfail(char*);
void	fail(char*);
void	succeed(char*);
void	error(char*, ...);
int	readarg(int, char*, int);
void	readln(char*, char*, int);
void	getpass(char*, int);
int	getauthkey(char*);
int	keyconv(void *, Fconv*);
long	tm2sec(Tm tm);
