#pragma	lib	"libhttpd.a"
#pragma	src	"/sys/src/libhttpd"

typedef struct HConnect		HConnect;
typedef struct HContent		HContent;
typedef struct HContents	HContents;
typedef struct HETag		HETag;
typedef struct HFields		HFields;
typedef struct Hio		Hio;
typedef struct Htmlesc		Htmlesc;
typedef struct HttpHead		HttpHead;
typedef struct HttpReq		HttpReq;
typedef struct HRange		HRange;
typedef struct HSPairs		HSPairs;

typedef struct Bin		Bin;

enum
{
	HMaxWord	= 32*1024,
	HBufSize	= 32*1024,

	/*
	 * error messages
	 */
	HInternal	= 0,
	HTempFail,
	HUnimp,
	HBadReq,
	HBadSearch,
	HNotFound,
	HUnauth,
	HSyntax,
	HNoSearch,
	HNoData,
	HExpectFail,
	HUnkVers,
	HBadCont,
	HOK,
};

/*
 * table of html character escape codes
 */
struct Htmlesc
{
	char	*name;
	Rune	value;
};

struct HContent
{
	HContent	*next;
	char		*generic;
	char		*specific;
	float		q;			/* desirability of this kind of file */
	int		mxb;			/* max uchars until worthless */
};

struct HContents
{
	HContent	*type;
	HContent *encoding;
};

/*
 * generic http header with a list of tokens,
 * each with an optional list of parameters
 */
struct HFields
{
	char		*s;
	HSPairs		*params;
	HFields		*next;
};

/*
 * list of pairs a strings
 * used for tag=val pairs for a search or form submission,
 * and attribute=value pairs in headers.
 */
struct HSPairs
{
	char		*s;
	char		*t;
	HSPairs		*next;
};

/*
 * uchar ranges within a file
 */
struct HRange
{
	int		suffix;			/* is this a suffix request? */
	ulong		start;
	ulong		stop;			/* ~0UL -> not given */
	HRange		*next;
};

/*
 * list of http/1.1 entity tags
 */
struct HETag
{
	char		*etag;
	int		weak;
	HETag		*next;
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

	Hsize = HBufSize
};

struct Hio {
	Hio		*hh;			/* next lower layer Hio, or nil if reads from fd */
	int		fd;			/* associated file descriptor */
	ulong		seek;			/* of start */
	uchar		state;			/* state of the file */
	uchar		xferenc;		/* chunked transfer encoding state */
	uchar		*pos;			/* current position in the buffer */
	uchar		*stop;			/* last character active in the buffer */
	uchar		*start;			/* start of data buffer */
	ulong		bodylen;		/* remaining length of message body */
	uchar		buf[Hsize+32];
};

/*
 * request line
 */
struct HttpReq
{
	/*
	 * request line
	 */
	char		*meth;
	char		*uri;
	char		*urihost;
	char		*search;
	int		vermaj;
	int		vermin;
};

/*
 * header lines
 */
struct HttpHead
{
	int		closeit;		/* http1.1 close connection after this request? */
	uchar		persist;		/* <http/1.1 requests a persistent connection */

	uchar		expectcont;		/* expect a 100-continue */
	uchar		expectother;		/* expect anything else; should reject with ExpectFail */
	ulong		contlen;		/* if != ~0UL, length of included message body */
	HFields		*transenc;		/* if present, encoding of included message body */
	char		*client;
	char		*host;
	HContent	*okencode;
	HContent	*oklang;
	HContent	*oktype;
	HContent	*okchar;
	ulong		ifmodsince;
	ulong		ifunmodsince;
	ulong		ifrangedate;
	HETag		*ifmatch;
	HETag		*ifnomatch;
	HETag		*ifrangeetag;
	HRange		*range;
	char		*authuser;		/* authorization info */
	char		*authpass;

	/*
	 * experimental headers
	 */
	int		fresh_thresh;
	int		fresh_have;
};

/*
 * all of the state for a particular connection
 */
struct HConnect
{
	void		*private;		/* for the library clients */
	void		(*replog)(HConnect*, char*, ...);	/* called when reply sent */

	HttpReq		req;
	HttpHead	head;

	Bin		*bin;

	ulong		reqtime;		/* time at start of request */
	char		xferbuf[HBufSize];	/* buffer for making up or transferring data */
	uchar		header[HBufSize + 2];	/* room for \n\0 */
	uchar		*hpos;
	uchar		*hstop;
	Hio		hin;
	Hio		hout;
};

/*
 * configuration for all connections within the server
 */
extern	char*		hmydomain;
extern	char*		hversion;
extern	Htmlesc		htmlesc[];

char			*hstrdup(HConnect *c, char *s);
void			*halloc(HConnect *c, ulong size);

char			*hunload(Hio*);
int			hload(Hio*, char*);
int			hgetc(Hio*);
void			*hreadbuf(Hio *h, void *vsave);
int			hungetc(Hio *h);
int			hputc(Hio*, int);
int			hprint(Hio*, char*, ...);
#pragma			varargck	argpos	hprint	2
int			hwrite(Hio*, void*, int);
int			hinit(Hio*, int, int);
int			hflush(Hio*);
int			hxferenc(Hio*, int);
void			hclose(Hio*);
int			hiserror(Hio *h);
int			hbuflen(Hio *h, void *p);
Hio			*hbodypush(Hio *hh, ulong len, HFields *te);
int			hcheckcontent(HContent*, HContent*, char*, int);
ulong			hdate2sec(char*);
int			hdateconv(va_list*, Fconv*);
int			hfail(HConnect*, int, ...);
int			hgethead(HConnect *c, int many);
int			http11(HConnect*);
int			httpconv(va_list*, Fconv*);
char*			httpunesc(HConnect *c, char *s);
char*			hlower(char*);
HContent*		hmkcontent(HConnect *c, char *generic, char *specific, HContent *next);
char*			hmkmimeboundary(HConnect *c);
HFields*		hmkhfields(HConnect *c, char *s, HSPairs *p, HFields *next);
HSPairs*		hmkspairs(HConnect *c, char *s, char *t, HSPairs *next);
int			hmoved(HConnect *c, char *uri);
void			hokheaders(HConnect *c);
int			hparsereq(HConnect *c, int timeout);
int			hparseheaders(HConnect*, int timeout);
HSPairs*		hparsequery(HConnect *c, char *search);
int			hredirected(HConnect *c, char *how, char *uri);
void			hreqcleanup(HConnect *c);
HFields*		hrevhfields(HFields *hf);
HSPairs*		hrevspairs(HSPairs *sp);
int			hunallowed(HConnect *, char *allowed);
int			hurlconv(va_list*, Fconv*);
char*			hurlunesc(HConnect *c, char *s);

/*
 * D is httpd format date conversion
 * U is url escape convertsion
 * H is html escape conversion
 */
#pragma	varargck	type	"D"	long
#pragma	varargck	type	"D"	ulong
#pragma	varargck	type	"U"	char*
#pragma	varargck	type	"H"	char*
