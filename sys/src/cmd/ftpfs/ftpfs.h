typedef struct File	File;
typedef struct Node	Node;
typedef struct OS	OS;

/* a tree for file path's - this mirrors the directory structure */
struct Node
{
	Dir	d;
	Node	*parent;
	Node	*sibs;
	Node	*children;
	File	*fp;
	short	depth;
	char	*longname;
};

/* OS types */
enum
{
	Unix=		1,
	Tops=		2,
	Plan9=		3,
	VM=		4,
	VMS=		5,
	MVS=		6,
	NetWare=	7,
	OS½=		8,
	TSO=		9,
	NT=		10,
	Unknown=	11,
};

struct OS
{
	int	os;
	char	*name;
};
extern OS oslist[];

/* temporary files */
void	filefree(Node*);
int	fileread(Node*, char*, long, int);
int	filewrite(Node*, char*, long, int);
void	filedirty(Node*);
void	fileclean(Node*);
int	fileisdirty(Node*);

/* time conversion */
ulong	tm2sec(Tm);

/* ftp protocol */
void	hello(char*);
void	quit(void);
void	preamble(char*);
void	login(void);
void	clogin(char *, char *);
void	nop(void);
int	readdir(Node*);
int	createdir(Node*);
int	readfile(Node*);
int	createfile(Node*);
int	changedir(Node*);
int	removefile(Node*);
int	removedir(Node*);

/* misc */
void*	safecpy(void*, void*, int);
void	fatal(char*, ...);
int	seterr(char*, ...);
Node*	extendpath(Node*, char*);
Node*	newnode(Node*, char*);
void	uncache(Node*);
void	invalidate(Node*);
void	uncachedir(Node*, Node*);
Node*	newtopsdir(char*);
void	fixsymbolic(Node*);

extern Node *remdir;	/* current directory on remote side */
extern Node *remroot;	/* root on remote side */
extern int os;		/* remote os */
extern int debug;	/* non-zero triggers debugging output */
extern int usenlst;
extern char *nosuchfile;
extern char *ext;	/* add to names of non-dir files */
extern int defos;
extern int quiet;

#define ISCACHED(x) ((x)->d.type)
#define UNCACHED(x) (x)->d.type = 0
#define CACHED(x) { (x)->d.type = 1; (x)->d.atime = time(0); }
#define ISOLD(x) (x)->d.atime + TIMEOUT < time(0)
#define ISVALID(x) ((x)->d.dev)
#define INVALID(x) (x)->d.dev = 0
#define VALID(x) (x)->d.dev = 1
#define TIMEOUT 5*60
#define CHSYML 0x10000000

