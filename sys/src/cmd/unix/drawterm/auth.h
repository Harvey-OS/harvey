
typedef struct	Ticket		Ticket;
typedef struct	Ticketreq	Ticketreq;
typedef struct	Authenticator	Authenticator;
typedef struct	Nvrsafe		Nvrsafe;
typedef struct	Passwordreq	Passwordreq;
typedef struct	Chalstate	Chalstate;

enum
{
	DOMLEN=		48,		/* length of an authentication domain name */
	DESKEYLEN=	7,		/* length of a des key for encrypt/decrypt */
	CHALLEN=	8,		/* length of a challenge */
	NETCHLEN=	16,		/* max network challenge length	*/
	CONFIGLEN=	14,

	KEYDBLEN=	NAMELEN+DESKEYLEN+4+2
};

/* encryption numberings (anti-replay) */
enum
{
	AuthTreq=1,	/* ticket request */
	AuthChal=2,	/* challenge box request */
	AuthPass=3,	/* change password */
	AuthMod=6,	/* modify user */

	AuthOK=4,	/* reply follows */
	AuthErr=5,	/* error follows */


	AuthTs=64,	/* ticket encrypted with server's key */
	AuthTc,		/* ticket encrypted with client's key */
	AuthAs,		/* server generated authenticator */
	AuthAc		/* client generated authenticator */
};

struct Ticketreq
{
	char	type;
	char	authid[NAMELEN];	/* server's encryption id */
	char	authdom[DOMLEN];	/* server's authentication domain */
	char	chal[CHALLEN];		/* challenge from server */
	char	hostid[NAMELEN];	/* host's encryption id */
	char	uid[NAMELEN];		/* uid of requesting user on host */
};
#define	TICKREQLEN	(3*NAMELEN+CHALLEN+DOMLEN+1)

struct Ticket
{
	char	num;			/* replay protection */
	char	chal[CHALLEN];		/* server challenge */
	char	cuid[NAMELEN];		/* uid on client */
	char	suid[NAMELEN];		/* uid on server */
	char	key[DESKEYLEN];		/* nonce DES key */
};
#define	TICKETLEN	(CHALLEN+2*NAMELEN+DESKEYLEN+1)

struct Authenticator
{
	char	num;			/* replay protection */
	char	chal[CHALLEN];
	ulong	id;			/* authenticator id, ++'d with each auth */
};
#define	AUTHENTLEN	(CHALLEN+4+1)

struct Passwordreq
{
	char	num;
	char	old[NAMELEN];
	char	new[NAMELEN];
};
#define	PASSREQLEN	(2*NAMELEN+1)

struct Nvrsafe
{
	char	machkey[DESKEYLEN];
	uchar	machsum;
	char	authkey[DESKEYLEN];
	uchar	authsum;
	char	config[CONFIGLEN];
	uchar	configsum;
	char	authid[NAMELEN];
	uchar	authidsum;
	char	authdom[DOMLEN];
	uchar	authdomsum;
};

struct Chalstate
{
	int	afd;			/* /dev/authenticate */
	int	asfd;			/* authdial() */
	char	chal[NETCHLEN];		/* challenge/response */
};

extern	int	convT2M(Ticket*, char*, char*);
extern	void	convM2T(char*, Ticket*, char*);
extern	int	convA2M(Authenticator*, char*, char*);
extern	void	convM2A(char*, Authenticator*, char*);
extern	int	convTR2M(Ticketreq*, char*);
extern	void	convM2TR(char*, Ticketreq*);
extern	int	convPR2M(Passwordreq*, char*, char*);
extern	void	convM2PR(char*, Passwordreq*, char*);
extern	uchar	nvcsum(void*, int);
extern	int	opasstokey(void*, char*);
extern	int	passtokey(void*, char*);
extern	int	authenticate(int, int);
extern	int	newns(char*, char*);
extern	int	authdial(void);
extern	int	auth(int);
extern	int	srvauth(int, char*);
extern	int	getchal(Chalstate*, char*);
extern	int	chalreply(Chalstate*, char*);
extern	int	amount(int, char*, int, char*);
