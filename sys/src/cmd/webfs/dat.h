typedef struct Client Client;
typedef struct Ctl Ctl;
typedef struct Ibuf Ibuf;
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
	char *authenticate;
	char *ext;
	int npostbody;
	int havepostbody;
	int iobusy;
	int bodyopened;
	Ioproc *io;
	int ref;
	void *aux;
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
	STACK = 16384,
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

