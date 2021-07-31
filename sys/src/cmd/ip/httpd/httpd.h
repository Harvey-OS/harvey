typedef struct Connect	Connect;
typedef struct Content	Content;
typedef struct Contents	Contents;
typedef struct ETag	ETag;
typedef struct HFields	HFields;
typedef struct Hio	Hio;
typedef struct Htmlesc	Htmlesc;
typedef struct HttpHead	HttpHead;
typedef struct HttpReq	HttpReq;
typedef struct Range	Range;
typedef struct SPairs	SPairs;

enum
{
	MaxWord	= 32*1024,
	BufSize	= 32*1024,

	/*
	 * error messages
	 */
	Internal	= 0,
	TempFail,
	Unimp,
	BadReq,
	BadSearch,
	NotFound,
	Unauth,
	Syntax,
	NoSearch,
	NoData,
	ExpectFail,
	UnkVers,
	BadCont,
	OK,
};

/*
 * table of html character escape codes
 */
struct Htmlesc
{
	char	*name;
	Rune	value;
};

struct Content
{
	Content	*next;
	char	*generic;
	char	*specific;
	float	q;		/* desirability of this kind of file */
	int	mxb;		/* max uchars until worthless */
};

struct Contents
{
	Content	*type;
	Content *encoding;
};

/*
 * generic http header with a list of tokens,
 * each with an optional list of parameters
 */
struct HFields
{
	char	*s;
	SPairs	*params;
	HFields	*next;
};

/*
 * list of pairs a strings
 * used for tag=val pairs for a search or form submission,
 * and attribute=value pairs in headers.
 */
struct SPairs
{
	char	*s;
	char	*t;
	SPairs	*next;
};

/*
 * uchar ranges within a file
 */
struct Range
{
	int	suffix;		/* is this a suffix request? */
	ulong	start;
	ulong	stop;		/* ~0UL -> not given */
	Range	*next;
};

/*
 * list of http/1.1 entity tags
 */
struct ETag
{
	char	*etag;
	int	weak;
	ETag	*next;
};

/*
 * HTTP custom IO
 * supports chunked transfer encoding
 * and initialization of the input buffer from a string.
 */
enum
{
	Hnone,
	Hread,
	Hend,
	Hwrite,
	Herr,

	Hsize = BufSize
};

struct Hio {
	Hio	*hh;			/* next lower layer Hio, or nil if reads from fd */
	int	fd;			/* associated file descriptor */
	ulong	seek;			/* of start */
	uchar	state;			/* state of the file */
	uchar	xferenc;		/* chunked transfer encoding state */
	uchar	*pos;			/* current position in the buffer */
	uchar	*stop;			/* last character active in the buffer */
	uchar	*start;			/* start of data buffer */
	ulong	bodylen;		/* remaining length of message body */
	uchar	buf[Hsize+32];
};

char	*hunload(Hio*);
int	hload(Hio*, char*);
int	hgetc(Hio*);
void	*hreadbuf(Hio *h, void *vsave);
int	hungetc(Hio *h);
int	hputc(Hio*, int);
int	hprint(Hio*, char*, ...);
#pragma	varargck	argpos	hprint	2
int	hwrite(Hio*, void*, int);
int	hinit(Hio*, int, int);
int	hflush(Hio*);
int	hxferenc(Hio*, int);
void	hclose(Hio*);
int	hiserror(Hio *h);
int	hbuflen(Hio *h, void *p);
Hio	*hbodypush(Hio *hh, ulong len, HFields *te);

/*
 * request line
 */
struct HttpReq
{
	/*
	 * request line
	 */
	char	*meth;
	char	*uri;
	char	*urihost;
	char	*search;
	int	vermaj;
	int	vermin;
};

/*
 * header lines
 */
struct HttpHead
{
	int	closeit;		/* http1.1 close connection after this request? */
	uchar	persist;		/* <http/1.1 requests a persistent connection */

	uchar	expectcont;		/* expect a 100-continue */
	uchar	expectother;		/* expect anything else; should reject with ExpectFail */
	ulong	contlen;		/* if != ~0UL, length of included message body */
	HFields	*transenc;		/* if present, encoding of included message body */
	char	*client;
	char	*host;
	Content	*okencode;
	Content	*oklang;
	Content	*oktype;
	Content	*okchar;
	ulong	ifmodsince;
	ulong	ifunmodsince;
	ulong	ifrangedate;
	ETag	*ifmatch;
	ETag	*ifnomatch;
	ETag	*ifrangeetag;
	Range	*range;
	char	*authuser;		/* authorization info */
	char	*authpass;

	/*
	 * experimental headers
	 */
	int	fresh_thresh;
	int	fresh_have;
};

/*
 * all of the state for a particular connection
 */
struct Connect
{
	HttpReq		req;
	HttpHead	head;

	char		*remotesys;
	ulong		reqtime;		/* time at start of request */
	char		xferbuf[BufSize];	/* buffer for making up or transferring data */
	uchar		header[BufSize + 2];	/* room for \n\0 */
	uchar		*hpos;
	uchar		*hstop;
	Hio		hin;
	Hio		hout;
};

/*
 * configuration for all connections within the server
 */
extern	char		netdir[];
extern	char*		mydomain;
extern	char*		mysysname;
extern	char*		namespace;
extern	int		logall[2];
extern	char*		HTTPLOG;
extern	char*		version;
extern	Htmlesc		htmlesc[];
extern	char*		webroot;
extern	int		verbose;

#define 		STRLEN(s)	(sizeof(s)-1)

char			*estrdup(char*);
char			*hstrdup(char *s);
void			*halloc(ulong size);

void			anonymous(Connect*);
int			authcheck(Connect *c);
char*			authrealm(char *path);
vlong			Bfilelen(void*);
int			checkcontent(Content*, Content*, char*, int);
int			checkreq(Connect *c, Content *type, Content *enc, long mtime, char *etag);
int			cistrcmp(char*, char*);
int			cistrncmp(char*, char*, int);
void			contentinit(void);
Contents		dataclass(char*, int);
ulong			date2sec(char*);
int			dateconv(va_list*, Fconv*);
void*			ezalloc(ulong);
int			etagmatch(int, ETag*, char*);
int			fail(Connect*, int, ...);
Range			*fixrange(Range *h, long length);
int			gethead(Connect *c, int many);
void			hintprint(Hio*, char *, int, int);
int			http11(Connect*);
int			httpconv(va_list*, Fconv*);
void			httpheaders(Connect*);
char*			httpunesc(char*);
Connect*		init(int, char**);
void			logit(Connect*, char*, ...);
char*			lower(char*);
#pragma	varargck	argpos	logit	2
Content*		mkcontent(char*, char*, Content*);
char*			mkmimeboundary(void);
HFields*		mkhfields(char *s, SPairs *params, HFields *next);
SPairs*			mkspairs(char *s, char *t, SPairs *next);
int			moved(Connect *c, char *uri);
int			notfound(Connect *c, char *url);
void			okheaders(Connect *c);
SPairs*			parsequery(char*);
char*			redirect(char*);
int			redirected(Connect *c, char *how, char *uri);
void			redirectinit(void);
void			reqcleanup(Connect *c);
HFields*		revhfields(HFields *hf);
SPairs*			revspairs(SPairs *sp);
int			sendfd(Connect *c, int fd, Dir *dir, Content *type, Content *enc);
void			statsinit(void);
int			unallowed(Connect *, char *allowed);
int			updateQid(int, Qid*);
void			urlcanon(char *url);
Contents		uriclass(char*);
int			urlconv(va_list*, Fconv*);
void			urlinit(void);
char*			urlunesc(char*);
void			writelog(Connect*, char*, ...);
#pragma	varargck	argpos	writelog	2

#pragma	varargck	type	"D"	long
#pragma	varargck	type	"D"	ulong
#pragma	varargck	type	"U"	char*
#pragma	varargck	type	"H"	char*
