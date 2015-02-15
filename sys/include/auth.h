/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#pragma	src	"/sys/src/libauth"
#pragma	lib	"libauth.a"

/*
 * Interface for typical callers.
 */

typedef struct	AuthInfo	AuthInfo;
typedef struct	Chalstate	Chalstate;
typedef struct	Chapreply	Chapreply;
typedef struct	MSchapreply	MSchapreply;
typedef struct	UserPasswd	UserPasswd;
typedef struct	AuthRpc		AuthRpc;

enum
{
	MAXCHLEN=	256,		/* max challenge length	*/
	MAXNAMELEN=	256,		/* maximum name length */
	MD5LEN=		16,

	ARok = 0,			/* rpc return values */
	ARdone,
	ARerror,
	ARneedkey,
	ARbadkey,
	ARwritenext,
	ARtoosmall,
	ARtoobig,
	ARrpcfailure,
	ARphase,

	AuthRpcMax = 4096,
};

struct AuthRpc
{
	int afd;
	int8_t ibuf[AuthRpcMax+1];	/* +1 for NUL in auth_rpc.c */
	int8_t obuf[AuthRpcMax];
	int8_t *arg;
	uint narg;
};

struct AuthInfo
{
	int8_t	*cuid;		/* caller id */
	int8_t	*suid;		/* server id */
	int8_t	*cap;		/* capability (only valid on server side) */
	int	nsecret;	/* length of secret */
	uint8_t	*secret;	/* secret */
};

struct Chalstate
{
	int8_t	*user;
	int8_t	chal[MAXCHLEN];
	int	nchal;
	void	*resp;
	int	nresp;

/* for implementation only */
	int	afd;			/* to factotum */
	AuthRpc	*rpc;			/* to factotum */
	int8_t	userbuf[MAXNAMELEN];	/* temp space if needed */
	int	userinchal;		/* user was sent to obtain challenge */
};

struct	Chapreply		/* for protocol "chap" */
{
	uint8_t	id;
	int8_t	resp[MD5LEN];
};

struct	MSchapreply	/* for protocol "mschap" */
{
	int8_t	LMresp[24];		/* Lan Manager response */
	int8_t	NTresp[24];		/* NT response */
};

struct	UserPasswd
{
	int8_t	*user;
	int8_t	*passwd;
};

extern	int	newns(int8_t*, int8_t*);
extern	int	addns(int8_t*, int8_t*);

extern	int	noworld(int8_t*);
extern	int	amount(int, int8_t*, int, int8_t*);

/* these two may get generalized away -rsc */
extern	int	login(int8_t*, int8_t*, int8_t*);
extern	int	httpauth(int8_t*, int8_t*);

typedef struct Attr Attr;
enum {
	AttrNameval,		/* name=val -- when matching, must have name=val */
	AttrQuery,		/* name? -- when matching, must be present */
	AttrDefault,		/* name:=val -- when matching, if present must match INTERNAL */
};
struct Attr
{
	int type;
	Attr *next;
	int8_t *name;
	int8_t *val;
};

typedef int AuthGetkey(int8_t*);

int	_attrfmt(Fmt*);
Attr	*_copyattr(Attr*);
Attr	*_delattr(Attr*, int8_t*);
Attr	*_findattr(Attr*, int8_t*);
void	_freeattr(Attr*);
Attr	*_mkattr(int, int8_t*, int8_t*, Attr*);
Attr	*_parseattr(int8_t*);
int8_t	*_strfindattr(Attr*, int8_t*);
#pragma varargck type "A" Attr*

extern AuthInfo*	fauth_proxy(int, AuthRpc *rpc, AuthGetkey *getkey,
				    int8_t *params);
extern AuthInfo*	auth_proxy(int fd, AuthGetkey *getkey, int8_t *fmt,
				   ...);
extern int		auth_getkey(int8_t*);
extern int		(*amount_getkey)(int8_t*);
extern void		auth_freeAI(AuthInfo *ai);
extern int		auth_chuid(AuthInfo *ai, int8_t *ns);
extern Chalstate	*auth_challenge(int8_t*, ...);
extern AuthInfo*	auth_response(Chalstate*);
extern int		auth_respond(void*, uint, int8_t*, uint, void*,
				       uint, AuthGetkey *getkey, int8_t*,
				       ...);
extern void		auth_freechal(Chalstate*);
extern AuthInfo*	auth_userpasswd(int8_t *user, int8_t *passwd);
extern UserPasswd*	auth_getuserpasswd(AuthGetkey *getkey, int8_t*,
					     ...);
extern AuthInfo*	auth_getinfo(AuthRpc *rpc);
extern AuthRpc*		auth_allocrpc(int afd);
extern Attr*		auth_attr(AuthRpc *rpc);
extern void		auth_freerpc(AuthRpc *rpc);
extern uint		auth_rpc(AuthRpc *rpc, int8_t *verb, void *a,
				    int n);
extern int		auth_wep(int8_t*, int8_t*, ...);
#pragma varargck argpos auth_proxy 3
#pragma varargck argpos auth_challenge 1
#pragma varargck argpos auth_respond 8
#pragma varargck argpos auth_getuserpasswd 2
