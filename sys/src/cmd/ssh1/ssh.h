#include <u.h>
#include <libc.h>
#include <mp.h>
#include <auth.h>
#include <libsec.h>

enum		/* internal debugging flags */
{
	DBG=			1<<0,
	DBG_CRYPTO=		1<<1,
	DBG_PACKET=		1<<2,
	DBG_AUTH=		1<<3,
	DBG_PROC=		1<<4,
	DBG_PROTO=		1<<5,
	DBG_IO=			1<<6,
	DBG_SCP=		1<<7,
};

enum		/* protocol packet types */
{
/* 0 */
	SSH_MSG_NONE=0,
	SSH_MSG_DISCONNECT,
	SSH_SMSG_PUBLIC_KEY,
	SSH_CMSG_SESSION_KEY,
	SSH_CMSG_USER,
	SSH_CMSG_AUTH_RHOSTS,
	SSH_CMSG_AUTH_RSA,
	SSH_SMSG_AUTH_RSA_CHALLENGE,
	SSH_CMSG_AUTH_RSA_RESPONSE,
	SSH_CMSG_AUTH_PASSWORD,

/* 10 */
	SSH_CMSG_REQUEST_PTY,
	SSH_CMSG_WINDOW_SIZE,
	SSH_CMSG_EXEC_SHELL,
	SSH_CMSG_EXEC_CMD,
	SSH_SMSG_SUCCESS,
	SSH_SMSG_FAILURE,
	SSH_CMSG_STDIN_DATA,
	SSH_SMSG_STDOUT_DATA,
	SSH_SMSG_STDERR_DATA,
	SSH_CMSG_EOF,

/* 20 */
	SSH_SMSG_EXITSTATUS,
	SSH_MSG_CHANNEL_OPEN_CONFIRMATION,
	SSH_MSG_CHANNEL_OPEN_FAILURE,
	SSH_MSG_CHANNEL_DATA,
	SSH_MSG_CHANNEL_INPUT_EOF,
	SSH_MSG_CHANNEL_OUTPUT_CLOSED,
	SSH_MSG_UNIX_DOMAIN_X11_FORWARDING,	/* obsolete */
	SSH_SMSG_X11_OPEN,
	SSH_CMSG_PORT_FORWARD_REQUEST,
	SSH_MSG_PORT_OPEN,

/* 30 */
	SSH_CMSG_AGENT_REQUEST_FORWARDING,
	SSH_SMSG_AGENT_OPEN,
	SSH_MSG_IGNORE,
	SSH_CMSG_EXIT_CONFIRMATION,
	SSH_CMSG_X11_REQUEST_FORWARDING,
	SSH_CMSG_AUTH_RHOSTS_RSA,
	SSH_MSG_DEBUG,
	SSH_CMSG_REQUEST_COMPRESSION,
	SSH_CMSG_MAX_PACKET_SIZE,
	SSH_CMSG_AUTH_TIS,

/* 40 */
	SSH_SMSG_AUTH_TIS_CHALLENGE,
	SSH_CMSG_AUTH_TIS_RESPONSE,
	SSH_CMSG_AUTH_KERBEROS,
	SSH_SMSG_AUTH_KERBEROS_RESPONSE,
	SSH_CMSG_HAVE_KERBEROS_TGT,
};

enum		/* protocol flags */
{
	SSH_PROTOFLAG_SCREEN_NUMBER=1<<0,
	SSH_PROTOFLAG_HOST_IN_FWD_OPEN=1<<1,
};

enum		/* agent protocol packet types */
{
	SSH_AGENTC_NONE = 0,
	SSH_AGENTC_REQUEST_RSA_IDENTITIES,
	SSH_AGENT_RSA_IDENTITIES_ANSWER,
	SSH_AGENTC_RSA_CHALLENGE,
	SSH_AGENT_RSA_RESPONSE,
	SSH_AGENT_FAILURE,
	SSH_AGENT_SUCCESS,
	SSH_AGENTC_ADD_RSA_IDENTITY,
	SSH_AGENTC_REMOVE_RSA_IDENTITY,
};

enum		/* protocol constants */
{
	SSH_MAX_DATA = 256*1024,
	SSH_MAX_MSG = SSH_MAX_DATA+4,

	SESSKEYLEN = 32,
	SESSIDLEN = 16,
	
	COOKIELEN = 8,
};

enum		/* crypto ids */
{
	SSH_CIPHER_NONE = 0,
	SSH_CIPHER_IDEA,
	SSH_CIPHER_DES,
	SSH_CIPHER_3DES,
	SSH_CIPHER_TSS,
	SSH_CIPHER_RC4,
	SSH_CIPHER_BLOWFISH,
	SSH_CIPHER_TWIDDLE,		/* for debugging */
};

enum		/* auth method ids */
{
	SSH_AUTH_RHOSTS = 1,
	SSH_AUTH_RSA = 2,
	SSH_AUTH_PASSWORD = 3,
	SSH_AUTH_RHOSTS_RSA = 4,
	SSH_AUTH_TIS = 5,
	SSH_AUTH_USER_RSA = 6,
};

typedef struct Auth Auth;
typedef struct Authsrv Authsrv;
typedef struct Cipher Cipher;
typedef struct CipherState CipherState;
typedef struct Conn Conn;
typedef struct Msg Msg;

#pragma incomplete CipherState

struct Auth
{
	int id;
	char *name;
	int (*fn)(Conn*);
};

struct Authsrv
{
	int id;
	char *name;
	int firstmsg;
	AuthInfo *(*fn)(Conn*, Msg*);
};

struct Cipher
{
	int id;
	char *name;
	CipherState *(*init)(Conn*, int isserver);
	void (*encrypt)(CipherState*, uchar*, int);
	void (*decrypt)(CipherState*, uchar*, int);
};

struct Conn
{
	QLock;
	int fd[2];
	CipherState *cstate;
	uchar cookie[COOKIELEN];
	uchar sessid[SESSIDLEN];
	uchar sesskey[SESSKEYLEN];
	RSApub *serverkey;
	RSApub *hostkey;
	ulong flags;
	ulong ciphermask;
	Cipher *cipher;		/* chosen cipher */
	Cipher **okcipher;	/* list of acceptable ciphers */
	int nokcipher;
	ulong authmask;
	Auth **okauth;
	int nokauth;
	char *user;
	char *host;
	char *aliases;
	int interactive;
	Msg *unget;

	RSApriv *serverpriv;		/* server only */
	RSApriv *hostpriv;
	Authsrv **okauthsrv;
	int nokauthsrv;
};

struct Msg
{
	Conn *c;
	uchar type;
	ulong len;		/* output: #bytes before pos, input: #bytes after pos */
	uchar *bp;	/* beginning of allocated space */
	uchar *rp;		/* read pointer */
	uchar *wp;	/* write pointer */
	uchar *ep;	/* end of allocated space */
	Msg *link;		/* for sshnet */
};

#define LONG(p)	(((p)[0]<<24)|((p)[1]<<16)|((p)[2]<<8)|((p)[3]))
#define PLONG(p, l) \
	(((p)[0]=(l)>>24),((p)[1]=(l)>>16),\
	 ((p)[2]=(l)>>8),((p)[3]=(l)))
#define SHORT(p) (((p)[0]<<8)|(p)[1])
#define PSHORT(p,l) \
	(((p)[0]=(l)>>8),((p)[1]=(l)))

extern char Edecode[];
extern char Eencode[];
extern char Ememory[];
extern char Ehangup[];
extern int doabort;
extern int debuglevel;

extern Auth authpassword;
extern Auth authrsa;
extern Auth authtis;

extern Authsrv authsrvpassword;
extern Authsrv authsrvtis;

extern Cipher cipher3des;
extern Cipher cipherblowfish;
extern Cipher cipherdes;
extern Cipher cipherrc4;
extern Cipher ciphernone;
extern Cipher ciphertwiddle;

/* msg.c */
Msg*	allocmsg(Conn*, int, int);
void		badmsg(Msg*, int);
Msg*	recvmsg(Conn*, int);
void		unrecvmsg(Conn*, Msg*);
int		sendmsg(Msg*);
uchar	getbyte(Msg*);
ushort	getshort(Msg*);
ulong	getlong(Msg*);
char*	getstring(Msg*);
void*	getbytes(Msg*, int);
mpint*	getmpint(Msg*);
RSApub*	getRSApub(Msg*);
void		putbyte(Msg*, uchar);
void		putshort(Msg*, ushort);
void		putlong(Msg*, ulong);
void		putstring(Msg*, char*);
void		putbytes(Msg*, void*, long);
void		putmpint(Msg*, mpint*);
void		putRSApub(Msg*, RSApub*);
mpint*	rsapad(mpint*, int);
mpint*	rsaunpad(mpint*);
void		mptoberjust(mpint*, uchar*, int);
mpint*	rsaencryptbuf(RSApub*, uchar*, int);

/* cmsg.c */
void		sshclienthandshake(Conn*);
void		requestpty(Conn*);
int		readgeom(int*, int*, int*, int*);
void		sendwindowsize(Conn*, int, int, int, int);
int		rawhack;

/* smsg.c */
void		sshserverhandshake(Conn*);

/* pubkey.c */
enum
{
	KeyOk,
	KeyWrong,
	NoKey,
	NoKeyFile,
};
int		appendkey(char*, char*, RSApub*);
int		findkey(char*, char*, RSApub*);
int		replacekey(char*, char*, RSApub*);

/* agent.c */
int		startagent(Conn*);
void		handleagentmsg(Msg*);
void		handleagentopen(Msg*);
void		handleagentieof(Msg*);
void		handleagentoclose(Msg*);

/* util.c */
void		debug(int, char*, ...);
void*	emalloc(long);
void*	erealloc(void*, long);
void		error(char*, ...);
RSApriv*	readsecretkey(char*);
int		readstrnl(int, char*, int);
void		atexitkill(int);
void		atexitkiller(void);
void		calcsessid(Conn*);
void		sshlog(char*, ...);
void		setaliases(Conn*, char*);
void		privatefactotum(void);

#pragma varargck argpos debug 2
#pragma varargck argpos error 1
#pragma varargck argpos sshlog 2
