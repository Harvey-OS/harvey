/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

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
	int8_t	type;
	int8_t	authid[ANAMELEN];	/* server's encryption id */
	int8_t	authdom[DOMLEN];	/* server's authentication domain */
	int8_t	chal[CHALLEN];		/* challenge from server */
	int8_t	hostid[ANAMELEN];	/* host's encryption id */
	int8_t	uid[ANAMELEN];		/* uid of requesting user on host */
};
#define	TICKREQLEN	(3*ANAMELEN+CHALLEN+DOMLEN+1)

struct Ticket
{
	int8_t	num;			/* replay protection */
	int8_t	chal[CHALLEN];		/* server challenge */
	int8_t	cuid[ANAMELEN];		/* uid on client */
	int8_t	suid[ANAMELEN];		/* uid on server */
	int8_t	key[DESKEYLEN];		/* nonce DES key */
};
#define	TICKETLEN	(CHALLEN+2*ANAMELEN+DESKEYLEN+1)

struct Authenticator
{
	int8_t	num;			/* replay protection */
	int8_t	chal[CHALLEN];
	uint32_t	id;			/* authenticator id, ++'d with each auth */
};
#define	AUTHENTLEN	(CHALLEN+4+1)

struct Passwordreq
{
	int8_t	num;
	int8_t	old[ANAMELEN];
	int8_t	new[ANAMELEN];
	int8_t	changesecret;
	int8_t	secret[SECRETLEN];	/* new secret */
};
#define	PASSREQLEN	(2*ANAMELEN+1+1+SECRETLEN)

struct	OChapreply
{
	uint8_t	id;
	int8_t	uid[ANAMELEN];
	int8_t	resp[OMD5LEN];
};

struct	OMSchapreply
{
	int8_t	uid[ANAMELEN];
	int8_t	LMresp[24];		/* Lan Manager response */
	int8_t	NTresp[24];		/* NT response */
};

/*
 *  convert to/from wire format
 */
extern	int	convT2M(Ticket*, int8_t*, int8_t*);
extern	void	convM2T(int8_t*, Ticket*, int8_t*);
extern	void	convM2Tnoenc(int8_t*, Ticket*);
extern	int	convA2M(Authenticator*, int8_t*, int8_t*);
extern	void	convM2A(int8_t*, Authenticator*, int8_t*);
extern	int	convTR2M(Ticketreq*, int8_t*);
extern	void	convM2TR(int8_t*, Ticketreq*);
extern	int	convPR2M(Passwordreq*, int8_t*, int8_t*);
extern	void	convM2PR(int8_t*, Passwordreq*, int8_t*);

/*
 *  convert ascii password to DES key
 */
extern	int	opasstokey(int8_t*, int8_t*);
extern	int	passtokey(int8_t*, int8_t*);

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
	int8_t	machkey[DESKEYLEN];	/* was file server's authid's des key */
	uint8_t	machsum;
	int8_t	authkey[DESKEYLEN];	/* authid's des key from password */
	uint8_t	authsum;
	/*
	 * file server config string of device holding full configuration;
	 * secstore key on non-file-servers.
	 */
	int8_t	config[CONFIGLEN];
	uint8_t	configsum;
	int8_t	authid[ANAMELEN];	/* auth userid, e.g., bootes */
	uint8_t	authidsum;
	int8_t	authdom[DOMLEN]; /* auth domain, e.g., cs.bell-labs.com */
	uint8_t	authdomsum;
};

extern	uint8_t	nvcsum(void*, int);
extern int	readnvram(Nvrsafe*, int);

/*
 *  call up auth server
 */
extern	int	authdial(int8_t *netroot, int8_t *authdom);

/*
 *  exchange messages with auth server
 */
extern	int	_asgetticket(int, int8_t*, int8_t*);
extern	int	_asrdresp(int, int8_t*, int);
extern	int	sslnegotiate(int, Ticket*, int8_t**, int8_t**);
extern	int	srvsslnegotiate(int, Ticket*, int8_t**, int8_t**);
