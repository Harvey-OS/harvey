typedef struct Msg Msg;
struct Msg
{
	Msg *link;
	uchar *rp;
	uchar *ep;
};

typedef struct Client Client;
struct Client
{
	int moribund;
	int activethread;
	int num;
	int ref;
	int status;
	int pid;
	char *cmd;
	int fd[2];
	char err[ERRMAX];

	Req *execreq;
	Channel *execpid;

	Req *rq, **erq;		/* reading */
	Msg *mq, **emq;
	Ioproc *readerproc;

	Channel *writerkick;
	Req *wq, **ewq;	/* writing */
	Req *curw;		/* currently writing */
	Ioproc *writerproc;	/* writing */
};
extern int nclient;
extern Client **client;
extern void dataread(Req*, Client*);
extern int newclient(void);
extern void closeclient(Client*);
extern void datawrite(Req*, Client*);
extern void ctlwrite(Req*, Client*);
extern void clientflush(Req*, Client*);

#define emalloc emalloc9p
#define estrdup estrdup9p
#define erealloc erealloc9p

extern Srv fs;
extern void initfs(void);
extern void setexecname(char*);

enum
{
	STACK = 8192,
};

enum	/* Client.status */
{
	Closed,
	Exec,
	Established,
	Hangup,
};

