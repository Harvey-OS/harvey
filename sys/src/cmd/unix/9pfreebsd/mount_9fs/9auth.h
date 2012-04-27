typedef struct	Ticket		Ticket;
typedef struct	Ticketreq	Ticketreq;
typedef struct	Authenticator	Authenticator;
typedef struct	Nvrsafe		Nvrsafe;
typedef struct	Passwordreq	Passwordreq;
typedef struct	Chalstate	Chalstate;
typedef struct	Apopchalstate	Apopchalstate;
typedef struct	Chapreply	Chapreply;
typedef struct	MSchapreply	MSchapreply;

enum
{
	DOMLEN=		48,		/* length of an authentication domain name */
	U9AUTH_DESKEYLEN=	7,		/* length of a des key for encrypt/decrypt */
	CHALLEN=	8,		/* length of a challenge */
	NETCHLEN=	16,		/* max network challenge length	*/
	CONFIGLEN=	14,
	SECRETLEN=	32,		/* max length of a secret */
	APOPCHLEN=	256,
	MD5LEN=		16,

	KEYDBOFF=	8,		/* length of random data at the start of key file */
	OKEYDBLEN=	U9FS_NAMELEN+U9AUTH_DESKEYLEN+4+2,	/* length of an entry in old key file */
	KEYDBLEN=	OKEYDBLEN+SECRETLEN,	/* length of an entry in key file */
	U9AUTH_TCPPORT= 567,
	U9AUTH_ILPORT=  566,
};

/* encryption numberings (anti-replay) */
enum
{
	AuthTreq=1,	/* ticket request */
	AuthChal=2,	/* challenge box request */
	AuthPass=3,	/* change password */
	AuthOK=4,	/* fixed length reply follows */
	AuthErr=5,	/* error follows */
	AuthMod=6,	/* modify user */
	AuthApop=7,	/* apop authentication for pop3 */
	AuthOKvar=9,	/* variable length reply follows */
	AuthChap=10,	/* chap authentication for ppp */
	AuthMSchap=11,	/* MS chap authentication for ppp */


	AuthTs=64,	/* ticket encrypted with server's key */
	AuthTc,		/* ticket encrypted with client's key */
	AuthAs,		/* server generated authenticator */
	AuthAc,		/* client generated authenticator */
	AuthTp,		/* ticket encrypted with clien's key for password change */
};

struct Ticketreq
{
	char	type;
	char	authid[U9FS_NAMELEN];	/* server's encryption id */
	char	authdom[DOMLEN];	/* server's authentication domain */
	char	chal[CHALLEN];		/* challenge from server */
	char	hostid[U9FS_NAMELEN];	/* host's encryption id */
	char	uid[U9FS_NAMELEN];		/* uid of requesting user on host */
};
#define	TICKREQLEN	(3*U9FS_NAMELEN+CHALLEN+DOMLEN+1)

struct Ticket
{
	char	num;			/* replay protection */
	char	chal[CHALLEN];		/* server challenge */
	char	cuid[U9FS_NAMELEN];		/* uid on client */
	char	suid[U9FS_NAMELEN];		/* uid on server */
	char	key[U9AUTH_DESKEYLEN];		/* nonce DES key */
};
#define	TICKETLEN	(CHALLEN+2*U9FS_NAMELEN+U9AUTH_DESKEYLEN+1)

struct Authenticator
{
	char	num;			/* replay protection */
	char	chal[CHALLEN];
	u_long	id;			/* authenticator id, ++'d with each auth */
};
#define	AUTHENTLEN	(CHALLEN+4+1)

struct Passwordreq
{
	char	num;
	char	old[U9FS_NAMELEN];
	char	new[U9FS_NAMELEN];
	char	changesecret;
	char	secret[SECRETLEN];	/* new secret */
};
#define	PASSREQLEN	(2*U9FS_NAMELEN+1+1+SECRETLEN)

struct Nvrsafe
{
	char	machkey[U9AUTH_DESKEYLEN];
	u_char	machsum;
	char	authkey[U9AUTH_DESKEYLEN];
	u_char	authsum;
	char	config[CONFIGLEN];
	u_char	configsum;
	char	authid[U9FS_NAMELEN];
	u_char	authidsum;
	char	authdom[DOMLEN];
	u_char	authdomsum;
};

struct Chalstate
{
	int	afd;			/* /dev/authenticate */
	int	asfd;			/* authdial() */
	char	chal[NETCHLEN];		/* challenge/response */
};

struct Apopchalstate
{
	int	afd;			/* /dev/authenticate */
	int	asfd;			/* authdial() */
	char	chal[APOPCHLEN];	/* challenge/response */
};

struct	Chapreply
{
	u_char	id;
	char	uid[U9FS_NAMELEN];
	char	resp[MD5LEN];
};

struct	MSchapreply
{
	char	uid[U9FS_NAMELEN];
	char	LMresp[24];		/* Lan Manager response */
	char	NTresp[24];		/* NT response */
};

extern	int	convT2M(Ticket*, char*, char*);
extern	void	convM2T(char*, Ticket*, char*);
extern	void	convM2Tnoenc(char*, Ticket*);
extern	int	convA2M(Authenticator*, char*, char*);
extern	void	convM2A(char*, Authenticator*, char*);
extern	int	convTR2M(Ticketreq*, char*);
extern	void	convM2TR(char*, Ticketreq*);
extern	int	convPR2M(Passwordreq*, char*, char*);
extern	void	convM2PR(char*, Passwordreq*, char*);
extern	u_char	nvcsum(void*, int);
extern	int	opasstokey(char*, char*);
extern	int	passtokey(char*, char*);
extern	int	authenticate(int, int);
extern	int	newns(char*, char*);
extern	int	addns(char*, char*);
extern	int	authdial(void);
extern	int	auth(int);
extern	int	srvauth(int, char*);
extern	int	nauth(int, Ticket*);
extern	int	nsrvauth(int, char*, Ticket*);
extern	int	getchal(Chalstate*, char*);
extern	int	chalreply(Chalstate*, char*);
extern	int	amount(int, char*, int, char*);
extern	int	apopchal(Apopchalstate*);
extern	int	apopreply(Apopchalstate*, char*, char*);
extern	int	login(char*, char*, char*);
extern	int	sslnegotiate(int, Ticket*, char**, char**);
extern	int	srvsslnegotiate(int, Ticket*, char**, char**);
