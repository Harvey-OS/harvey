typedef struct Client Client;
typedef struct Ctl Ctl;
typedef struct Ibuf Ibuf;
typedef struct Ioproc Ioproc;
typedef struct Url Url;

/* simple buffered i/o for network connections; shared by http, ftp */
struct Ibuf
{
	int fd;
	Ioproc *io;
	char buf[4096];
	char *rp, *wp;
};

struct Ctl
{
	int	acceptcookies;
	int	sendcookies;
	int	redirectlimit;
	char	*useragent;
};

struct Client
{
	Url	*url;
	Url	*baseurl;
	Ctl ctl;
	Channel *creq;	/* chan(Req*) */
	int num;
	int plumbed;
	char *contenttype;
	char *postbody;
	char *redirect;
	char *ext;
	int npostbody;
	int havepostbody;
	int iobusy;
	int bodyopened;
	Ioproc *io;
	int ref;
	void *aux;
};

/* asynchronous i/o in another proc */
struct Ioproc
{
	long (*read)(Ioproc*, int, void*, long);
	long (*write)(Ioproc*, int, void*, long);
	int (*dial)(Ioproc*, char*, char*, char*, int*, int);
	int (*close)(Ioproc*, int);
	int (*open)(Ioproc*, char*, int);
	int (*print)(Ioproc*, int, char*, ...);
	void (*interrupt)(Ioproc*);

	int pid;	/* internal */
	Channel *c;
	int inuse;
	void (*op)(Ioproc*);
	long arg[10];
	long ret;
	char err[ERRMAX];
	Ioproc *next;
};

/*
 * If ischeme is USunknown, then the given URL is a relative
 * URL which references the "current document" in the context of the base.
 * If this is the case, only the "fragment" and "url" members will have
 * meaning, and the given URL structure may not be used as a base URL itself.
 */
enum
{
	USunknown,
	UShttp,
	UShttps,
	USftp,
	USfile,
	UScurrent,
};

struct Url
{
	int		ischeme;
	char*	url;
	char*	scheme;
	int		(*open)(Client*, Url*);
	int		(*read)(Client*, Req*);
	void		(*close)(Client*);
	char*	schemedata;
	char*	authority;
	char*	user;
	char*	passwd;
	char*	host;
	char*	port;
	char*	path;
	char*	query;
	char*	fragment;
	union {
		struct {
			char *page_spec;
		} http;
		struct {
			char *path_spec;
			char *type;
		} ftp;
	};
};

enum
{
	STACK = 8192,
};

extern	Client**	client;
extern	int		cookiedebug;
extern	Srv		fs;
extern	int		fsdebug;
extern	Ctl		globalctl;
extern	int		nclient;
extern	int		urldebug;
extern	int		httpdebug;
extern	char*	status[];

