#pragma	src	"/sys/src/libauthsrv"
#pragma	lib	"libauthsrv.a"

/*
 * Interface for talking to authentication server.
 */
typedef struct	Ticket		Ticket;
typedef struct	Ticketreq	Ticketreq;
typedef struct	Authenticator	Authenticator;
typedef struct	Nvrsafe		Nvrsafe;
typedef struct	Passwordreq	Passwordreq;
typedef struct	OChapreply	OChapreply;
typedef struct	OMSchapreply	OMSchapreply;

enum
{
	ANAMELEN=	28,	/* name max size in previous proto */
	AERRLEN=	64,	/* errstr max size in previous proto */
	DOMLEN=		48,	/* authentication domain name length */
	DESKEYLEN=	7,	/* encrypt/decrypt des key length */
	CHALLEN=	8,	/* plan9 sk1 challenge length */
	NETCHLEN=	16,	/* max network challenge length (used in AS protocol) */
	CONFIGLEN=	14,
	SECRETLEN=	32,	/* secret max size */

	KEYDBOFF=	8,	/* bytes of random data at key file's start */
	OKEYDBLEN=	ANAMELEN+DESKEYLEN+4+2,	/* old key file entry length */
	KEYDBLEN=	OKEYDBLEN+SECRETLEN,	/* key file entry length */
	OMD5LEN=	16,
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
	AuthCram=12,	/* CRAM verification for IMAP (RFC2195 & rfc2104) */
	AuthHttp=13,	/* http domain login */
	AuthVNC=14,	/* VNC server login (deprecated) */


	AuthTs=64,	/* ticket encrypted with server's key */
	AuthTc,		/* ticket encrypted with client's key */
	AuthAs,		/* server generated authenticator */
	AuthAc,		/* client generated authenticator */
	AuthTp,		/* ticket encrypted with client's key for password change */
	AuthHr,		/* http reply */
};

struct Ticketreq
{
	char	type;
	char	authid[ANAMELEN];	/* server's encryption id */
	char	authdom[DOMLEN];	/* server's authentication domain */
	char	chal[CHALLEN];		/* challenge from server */
	char	hostid[ANAMELEN];	/* host's encryption id */
	char	uid[ANAMELEN];		/* uid of requesting user on host */
};
#define	TICKREQLEN	(3*ANAMELEN+CHALLEN+DOMLEN+1)

struct Ticket
{
	char	num;			/* replay protection */
	char	chal[CHALLEN];		/* server challenge */
	char	cuid[ANAMELEN];		/* uid on client */
	char	suid[ANAMELEN];		/* uid on server */
	char	key[DESKEYLEN];		/* nonce DES key */
};
#define	TICKETLEN	(CHALLEN+2*ANAMELEN+DESKEYLEN+1)

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
	char	old[ANAMELEN];
	char	new[ANAMELEN];
	char	changesecret;
	char	secret[SECRETLEN];	/* new secret */
};
#define	PASSREQLEN	(2*ANAMELEN+1+1+SECRETLEN)

struct	OChapreply
{
	uchar	id;
	char	uid[ANAMELEN];
	char	resp[OMD5LEN];
};

struct	OMSchapreply
{
	char	uid[ANAMELEN];
	char	LMresp[24];		/* Lan Manager response */
	char	NTresp[24];		/* NT response */
};

/*
 *  convert to/from wire format
 */
extern	int	convT2M(Ticket*, char*, char*);
extern	void	convM2T(char*, Ticket*, char*);
extern	void	convM2Tnoenc(char*, Ticket*);
extern	int	convA2M(Authenticator*, char*, char*);
extern	void	convM2A(char*, Authenticator*, char*);
extern	int	convTR2M(Ticketreq*, char*);
extern	void	convM2TR(char*, Ticketreq*);
extern	int	convPR2M(Passwordreq*, char*, char*);
extern	void	convM2PR(char*, Passwordreq*, char*);

/*
 *  convert ascii password to DES key
 */
extern	int	opasstokey(char*, char*);
extern	int	passtokey(char*, char*);

/*
 *  Nvram interface
 */
enum {
	NVread		= 0,	/* just read */
	NVwrite		= 1<<0,	/* always prompt and rewrite nvram */
	NVwriteonerr	= 1<<1,	/* prompt and rewrite nvram when corrupt */
	NVwritemem	= 1<<2,	/* don't prompt, write nvram from argument */
};

/* storage layout */
struct Nvrsafe
{
	char	machkey[DESKEYLEN];	/* was file server's authid's des key */
	uchar	machsum;
	char	authkey[DESKEYLEN];	/* authid's des key from password */
	uchar	authsum;
	/*
	 * file server config string of device holding full configuration;
	 * secstore key on non-file-servers.
	 */
	char	config[CONFIGLEN];
	uchar	configsum;
	char	authid[ANAMELEN];	/* auth userid, e.g., bootes */
	uchar	authidsum;
	char	authdom[DOMLEN]; /* auth domain, e.g., cs.bell-labs.com */
	uchar	authdomsum;
};

extern	uchar	nvcsum(void*, int);
extern int	readnvram(Nvrsafe*, int);

/*
 *  call up auth server
 */
extern	int	authdial(char *netroot, char *authdom);

/*
 *  exchange messages with auth server
 */
extern	int	_asgetticket(int, char*, char*);
extern	int	_asrdresp(int, char*, int);
extern	int	sslnegotiate(int, Ticket*, char**, char**);
extern	int	srvsslnegotiate(int, Ticket*, char**, char**);
