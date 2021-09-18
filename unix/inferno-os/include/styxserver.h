
#define Qroot	0

#define MSGMAX	((((8192+128)*2)+3) & ~3)

extern char Enomem[];	/* out of memory */
extern char Eperm[];		/* permission denied */
extern char Enodev[];	/* no free devices */
extern char Ehungup[];	/* i/o on hungup channel */
extern char Eexist[];		/* file exists */
extern char Enonexist[];	/* file does not exist */
extern char Ebadcmd[];	/* bad command */
extern char Ebadarg[];	/* bad arguments */

typedef uvlong	Path;
typedef struct Styxserver	Styxserver;
typedef struct Styxops Styxops;
typedef struct Styxfile Styxfile;
typedef struct Client Client;
typedef struct Fid Fid;

struct Styxserver
{
	Styxops *ops;
	Path qidgen;
	int connfd;
	int needfile;
	Client *clients;
	Client *curc;
	Styxfile *root;
	Styxfile **ftab;
	void	*priv;	/* private */
};

struct Client
{
	Styxserver *server;
	Client *next;
	int		fd;
	char	msg[MSGMAX];
	uint		nread;		/* valid bytes in msg (including nc)*/
	int		nc;			/* bytes consumed from front of msg by convM2S */
	char	data[MSGMAX];	/* Tread/Rread data */
	int		state;
	Fid		*fids;
	char		*uname;	/* uid */
	char		*aname;	/* attach name */
	void		*u;
};

struct Styxops
{
	char *(*newclient)(Client *c);
	char *(*freeclient)(Client *c);

	char *(*attach)(char *uname, char *aname);
	char *(*walk)(Qid *qid, char *name);
	char *(*open)(Qid *qid, int mode);
	char *(*create)(Qid *qid, char *name, int perm, int mode);
	char *(*read)(Qid qid, char *buf, ulong *n, vlong offset);
	char *(*write)(Qid qid, char *buf, ulong *n, vlong offset);
	char *(*close)(Qid qid, int mode);
	char *(*remove)(Qid qid);
	char *(*stat)(Qid qid, Dir *d);
	char *(*wstat)(Qid qid, Dir *d);
};

struct Styxfile
{
	Dir	d;
	Styxfile *parent;
	Styxfile *child;
	Styxfile *sibling;
	Styxfile *next;
	int ref;
	int open;
	void	*u;
};

char *styxinit(Styxserver *server, Styxops *ops, char *port, int perm, int needfile);
char *styxwait(Styxserver *server);
char *styxprocess(Styxserver *server);
char *styxend(Styxserver *server);

Client *styxclient(Styxserver *server);

Styxfile *styxaddfile(Styxserver *server, Path pqid, Path qid, char *name, int mode, char *owner);
Styxfile *styxadddir(Styxserver *server, Path pqid, Path qid, char *name, int mode, char *owner);
int styxrmfile(Styxserver *server, Path qid);
Styxfile *styxfindfile(Styxserver *server, Path qid);

int	styxperm(Styxfile *file, char *uid, int mode);
long styxreadstr(ulong off, char *buf, ulong n, char *str);
Qid styxqid(int path, int isdir);
void *styxmalloc(int n);
void styxfree(void *p);
void styxdebug(void);
void styxsetowner(char*);
