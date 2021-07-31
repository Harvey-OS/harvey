#include <u.h>
#include <libc.h>
#include <auth.h>
#include <authsrv.h>
#include <mp.h>
#include <libsec.h>
#include <String.h>
#include <thread.h>	/* only for 9p.h */
#include <fcall.h>
#include <9p.h>

enum
{
	Maxname = 128,
	Maxrpc = 4096,

	/* common protocol phases; proto-specific phases start at 0 */
	Notstarted = -3,
	Broken = -2,
	Established = -1,

	/* rpc read/write return values */
	RpcFailure = 0,
	RpcNeedkey,
	RpcOk,
	RpcErrstr,
	RpcToosmall,
	RpcPhase,
	RpcConfirm,

	/* key lookup */
	Kowner = 1<<0,
	Kuser = 1<<1,
	Kwho = 3<<0,
	Knoconf = 1<<2,
};

typedef struct Attr Attr;
typedef struct Domain Domain;
typedef struct Fsstate Fsstate;
typedef struct Key Key;
typedef struct Keyring Keyring;
typedef struct Logbuf Logbuf;
typedef struct Proto Proto;
typedef struct User User;
typedef struct State State;

struct Fsstate
{
	char *sysuser;	/* user according to system */

	/* keylist, protolist */
	int listoff;

	/* per-rpc transient information */
	int pending;
	struct {
		char *arg, buf[Maxrpc], *verb;
		int iverb, narg, nbuf, nwant;
	} rpc;
	
	/* persistent (cross-rpc) information */
	char err[ERRMAX];
	char keyinfo[3*Maxname];	/* key request */
	char **phasename;
	int haveai, maxphase, phase, seqnum, started;
	Attr *attr;
	AuthInfo ai;
	Proto *proto;
	State *ps;
	struct {		/* pending or finished key confirmations */
		Key *key;
		int canuse;
		ulong tag;
	} *conf;
	int nconf;
};

struct Key
{
	int ref;
	Attr *attr;
	Attr *privattr;	/* private attributes, like *data */
	Proto *proto;

	void *priv;	/* protocol-specific; a parsed key, perhaps */
};

struct Keyring
{
	Key **key;
	int nkey;
};

struct Logbuf
{
	Req *wait;
	Req **waitlast;
	int rp;
	int wp;
	char *msg[128];
};

struct Proto
{
	char *name;
	int (*init)(Proto*, Fsstate*);
	int (*addkey)(Key*);
	void (*closekey)(Key*);
	int (*write)(Fsstate*, void*, uint);
	int (*read)(Fsstate*, void*, uint*);
	void (*close)(Fsstate*);
	char *keyprompt;
};

extern char *owner;
extern char *authdom;

extern char Easproto[];
extern char Ebadarg[];
extern char Ebadkey[];
extern char Enegotiation[];
extern char Etoolarge[];

/* confirm.c */
void confirmread(Req*);
void confirmflush(Req*);
int confirmwrite(char*);
void confirmqueue(Req*, Fsstate*);
void needkeyread(Req*);
void needkeyflush(Req*);
int needkeywrite(char*);
int needkeyqueue(Req*, Fsstate*);

/* fs.c */
extern	int		askforkeys;
extern	char		*authaddr;
extern	int		*confirminuse;
extern	int		debug;
extern	int		gflag;
extern	int		kflag;
extern	int		*needkeyinuse;
extern	int		sflag;
extern	int		uflag;
extern	char		*mtpt;
extern	char		*service;
extern	Proto 	*prototab[];
extern	Keyring	*ring;

/* log.c */
void flog(char*, ...);
#pragma varargck argpos flog 1
void logread(Req*);
void logflush(Req*);
void logbufflush(Logbuf*, Req*);
void logbufread(Logbuf*, Req*);
void logbufproc(Logbuf*);
void logbufappend(Logbuf*, char*);
void needkeyread(Req*);
void needkeyflush(Req*);
int needkeywrite(char*);
int needkeyqueue(Req*, Fsstate*);

/* rpc.c */
int ctlwrite(char*);
void rpcrdwrlog(Fsstate*, char*, uint, int, int);
void rpcstartlog(Attr*, Fsstate*, int);
void rpcread(Req*);
void rpcwrite(Req*);

/* secstore.c */
int havesecstore(void);
int secstorefetch(char*);

/* util.c */
#define emalloc emalloc9p
#define estrdup estrdup9p
#define erealloc erealloc9p
#pragma varargck argpos failure 2
#pragma varargck argpos findkey 6
#pragma varargck argpos setattr 2

int		_authdial(char*, char*);
void		askuser(char*);
int		canusekey(Fsstate*, Key*);
void		closekey(Key*);
uchar	*convAI2M(AuthInfo*, uchar*, int);
int		ctlwrite(char*);
int		failure(Fsstate*, char*, ...);
int		findkey(Key**, Fsstate*, int, int, Attr*, char*, ...);
int		findp9authkey(Key**, Fsstate*);
Proto	*findproto(char*);
char		*getnvramkey(int, char**);
int		isclient(char*);
int		matchattr(Attr*, Attr*, Attr*);
void 		memrandom(void*, int);
char 		*mkcap(char*);
int 		phaseerror(Fsstate*, char*);
char		*phasename(Fsstate*, int, char*);
void 		promptforhostowner(void);
String	*readcons(char*, char*, int);
int		replacekey(Key*);
char		*safecpy(char*, char*, int);
int		secdial(void);
Attr		*setattr(Attr*, char*, ...);
Attr		*setattrs(Attr*, Attr*);
void		sethostowner(void);
void		setmalloctaghere(void*);
int		smatch(char*, char*);
Attr		*sortattr(Attr*);
int		toosmall(Fsstate*, uint);
void		writehostowner(char*);

/* protocols */
extern Proto apop, cram;		/* apop.c */
extern Proto p9any, p9sk1, p9sk2;	/* p9sk.c */
extern Proto chap, mschap;	/* chap.c */
extern Proto p9cr, vnc;		/* p9cr.c */
extern Proto pass;			/* pass.c */
extern Proto sshrsa;			/* sshrsa.c */
