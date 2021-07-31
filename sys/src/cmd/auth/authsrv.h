#pragma lib "./lib.$O.a"

typedef Biobuf;

enum{
	PASSLEN		= 10,
	MAXNETCHAL	= 100000,		/* max securenet challenge */
};

#define	KEYDB		"/mnt/keys"
#define NETKEYDB	"/mnt/netkeys"
#define KEYDBBUF	(sizeof NETKEYDB)	/* enough for any keydb prefix */
#define AUTHLOG		"auth"

enum
{
	Nemail		= 10,
	Plan9		= 1,
	Securenet	= 2,
};

typedef struct
{
	char	*user;
	char	*name;
	char	*dept;
	char	*email[Nemail];
} Acctbio;

typedef struct {
	char	*keys;
	char	*msg;
	char	*who;
	Biobuf 	*b;
} Fs;

extern Fs fs[2];

char	*findkey(char*, char*, char*);
char	*setkey(char*, char*, char*);
char	*netresp(char*, long, char*);
char	*netdecimal(char*);
int	netcheck(void*, long, char*);
int	smartcheck(void*, long, char*);
char	*okpasswd(char*);
void	logfail(char*);
void	fail(char*);
void	succeed(char*);
void	error(char*, ...);
int	readarg(int, char*, int);
int	readn(int, char*, int);
void	readln(char*, char*, int, int);
void	getpass(char*, int);
int	getauthkey(char*);
int	keyconv(void *, Fconv*);
long	tm2sec(Tm tm);
int	readfile(char*, char*, int);
int	writefile(char*, char*, int);
int	readn(int, char*, int);
void	checksum(char*, char*);
void	rdbio(char*, char*, Acctbio*);
int	querybio(char*, char*, Acctbio*);
void	wrbio(char*, Acctbio*);
