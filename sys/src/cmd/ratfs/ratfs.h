#include <u.h>
#include <libc.h>
#include <auth.h>
#include <fcall.h>
#include <bio.h>

enum {
	MAXRPC = 8192,

	Qroot = 1,		/* fixed QID's */
	Qallow,
	Qdelay,
	Qblock,
	Qdial,
	Qdeny,
	Qtrusted,
	Qctl,
	Qdummy,
	Qaddr,			/* Qid's for "ip" & "account" subdirs (Qaddr-99) */

	Qtrustedfile = 100,	/* Qid's for trusted files (100-999)*/
	Qaddrfile   = 1000,	/* Qid's for address files (> 1000) */

				/* type codes in node.d.type */
	Directory =	0,	/* normal directory */
	Addrdir,		/* contains "ip" and "account" directories */
	IPaddr,			/* contains IP address "files" */
	Acctaddr,		/* contains Account address "files" */
	Trusted,		/* contains trusted IP files */
	Trustedperm,		/* permanently trusted IP pseudo-file */
	Trustedtemp,		/* temporarily trusted IP pseudo-file */
	Ctlfile,		/* ctl file under root */
	Dummynode,		/* place holder for Address pseudo-files */
};

typedef struct Fid	Fid;
typedef struct Node	Node;
typedef	struct Address	Address;
typedef struct Cidraddr	Cidraddr;
typedef struct Keyword	Keyword;

	/* an active fid */
struct Fid
{
	int	fid;
	int	dirindex;
	Node	*node;		/* current position in path */
	int	busy;
	int	open;		/* directories only */
	char	*name;
	char *uid;
	Fid	*next;
};

struct	Cidraddr
{
	ulong	ipaddr;		/* CIDR base addr */
	ulong	mask;		/* CIDR mask */
};

	/* an address is either an account name (domain!user) or Ip address */
struct	Address
{
	char	*name;		/* from the control file */
	Cidraddr ip;		/* CIDR Address */
};

/* Fids point to either a directory or pseudo-file */
struct Node
{
	Dir	d;		/* d.name, d.uid, d.gid, d.muid are atoms */
	int	count;
	int	allocated;	/* number of Address structs allocated */
	ulong	baseqid;	/* base of Qid's in this set */
	Node	*parent;	/* points to self in root node*/
	Node	*sibs;		/* 0 in Ipaddr and Acctaddr dirs */
	union {
		Node	*children;	/* type == Directory || Addrdir || Trusted */
		Address	*addrs;		/* type == Ipaddr || Acctaddr */
		Cidraddr ip;		/* type == Trustedfile */
	};
};

struct Keyword {
	char	*name;
	int	code;
};

Node	*root;			/* root of directory tree */
Node	dummy;			/* dummy node for fid's pointing to an Address */
int	srvfd;			/* fd for 9fs */
uchar rbuf[IOHDRSZ+MAXRPC+1];
int	debugfd;
char	*ctlfile;
char	*conffile;
long	lastconftime;
long	lastctltime;
int	trustedqid;

char*	atom(char*);
void	cidrparse(Cidraddr*, char*);
void	cleantrusted(void);
Node*	dirwalk(char*, Node*);
int	dread(Fid*, int);
void	fatal(char*, ...);
Node*	finddir(int);
int	findkey(char*, Keyword*);
void	getconf(void);
int	hread(Fid*, int);
void	io(void);
Node*	newnode(Node*, char*, ushort, int, ulong);
void	printfid(Fid*);
void	printnode(Node*);
void	printtree(Node*);
void	reload(void);
char*	subslash(char*);
char*	walk(char*, Fid*);

