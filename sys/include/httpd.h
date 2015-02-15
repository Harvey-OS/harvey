/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

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

#pragma incomplete Bin

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
	int8_t		*name;
	Rune		value;
};

struct HContent
{
	HContent	*next;
	int8_t		*generic;
	int8_t		*specific;
	float		q;		/* desirability of this kind of file */
	int		mxb;		/* max uchars until worthless */
};

struct HContents
{
	HContent	*type;
	HContent	 *encoding;
};

/*
 * generic http header with a list of tokens,
 * each with an optional list of parameters
 */
struct HFields
{
	int8_t		*s;
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
	int8_t		*s;
	int8_t		*t;
	HSPairs		*next;
};

/*
 * byte ranges within a file
 */
struct HRange
{
	int		suffix;			/* is this a suffix request? */
	uint32_t		start;
	uint32_t		stop;			/* ~0UL -> not given */
	HRange		*next;
};

/*
 * list of http/1.1 entity tags
 */
struct HETag
{
	int8_t		*etag;
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
	Hio		*hh; /* next lower layer Hio, or nil if reads from fd */
	int		fd;		/* associated file descriptor */
	uint32_t		seek;		/* of start */
	uint8_t		state;		/* state of the file */
	uint8_t		xferenc;	/* chunked transfer encoding state */
	uint8_t		*pos;		/* current position in the buffer */
	uint8_t		*stop;		/* last character active in the buffer */
	uint8_t		*start;		/* start of data buffer */
	uint32_t		bodylen;	/* remaining length of message body */
	uint8_t		buf[Hsize+32];
};

/*
 * request line
 */
struct HttpReq
{
	int8_t		*meth;
	int8_t		*uri;
	int8_t		*urihost;
	int8_t		*search;
	int		vermaj;
	int		vermin;
	HSPairs		*searchpairs;
};

/*
 * header lines
 */
struct HttpHead
{
	int	closeit;	/* http1.1 close connection after this request? */
	uint8_t	persist;	/* http/1.1 requests a persistent connection */

	uint8_t	expectcont;	/* expect a 100-continue */
	uint8_t	expectother; /* expect anything else; should reject with ExpectFail */
	uint32_t	contlen;	/* if != ~0UL, length of included message body */
	HFields	*transenc;  /* if present, encoding of included message body */
	int8_t	*client;
	int8_t	*host;
	HContent *okencode;
	HContent *oklang;
	HContent *oktype;
	HContent *okchar;
	uint32_t	ifmodsince;
	uint32_t	ifunmodsince;
	uint32_t	ifrangedate;
	HETag	*ifmatch;
	HETag	*ifnomatch;
	HETag	*ifrangeetag;
	HRange	*range;
	int8_t	*authuser;		/* authorization info */
	int8_t	*authpass;
	HSPairs	*cookie;	/* if present, list of cookies */
	HSPairs		*authinfo;		/* digest authorization */

	/*
	 * experimental headers
	 */
	int	fresh_thresh;
	int	fresh_have;
};

/*
 * all of the state for a particular connection
 */
struct HConnect
{
	void	*private;		/* for the library clients */
	void	(*replog)(HConnect*, int8_t*, ...); /* called when reply sent */

	int8_t	*scheme;		/* "http" vs. "https" */
	int8_t	*port;		/* may be arbitrary, i.e., neither 80 nor 443 */

	HttpReq	req;
	HttpHead head;

	Bin	*bin;

	uint32_t	reqtime;		/* time at start of request */
	int8_t	xferbuf[HBufSize]; /* buffer for making up or transferring data */
	uint8_t	header[HBufSize + 2];	/* room for \n\0 */
	uint8_t	*hpos;
	uint8_t	*hstop;
	Hio	hin;
	Hio	hout;
};

/*
 * configuration for all connections within the server
 */
extern	int8_t*		hmydomain;
extern	int8_t*		hversion;
extern	Htmlesc		htmlesc[];

/*
 * .+2,/^$/ | sort -bd +1
 */
void			*halloc(HConnect *c, uint32_t size);
Hio			*hbodypush(Hio *hh, uint32_t len, HFields *te);
int			hbuflen(Hio *h, void *p);
int			hcheckcontent(HContent*, HContent*, int8_t*, int);
void			hclose(Hio*);
uint32_t			hdate2sec(int8_t*);
int			hdatefmt(Fmt*);
int			hfail(HConnect*, int, ...);
int			hflush(Hio*);
int			hgetc(Hio*);
int			hgethead(HConnect *c, int many);
int			hinit(Hio*, int, int);
int			hiserror(Hio *h);
int			hlflush(Hio*);
int			hload(Hio*, int8_t*);
int8_t			*hlower(int8_t*);
HContent		*hmkcontent(HConnect *c, int8_t *generic,
				    int8_t *specific, HContent *next);
HFields			*hmkhfields(HConnect *c, int8_t *s,
					   HSPairs *p, HFields *next);
int8_t			*hmkmimeboundary(HConnect *c);
HSPairs			*hmkspairs(HConnect *c, int8_t *s, int8_t *t,
					  HSPairs *next);
int			hmoved(HConnect *c, int8_t *uri);
void			hokheaders(HConnect *c);
int			hparseheaders(HConnect*, int timeout);
HSPairs			*hparsequery(HConnect *c, int8_t *search);
int			hparsereq(HConnect *c, int timeout);
int			hprint(Hio*, int8_t*, ...);
int			hputc(Hio*, int);
void			*hreadbuf(Hio *h, void *vsave);
int			hredirected(HConnect *c, int8_t *how, int8_t *uri);
void			hreqcleanup(HConnect *c);
HFields			*hrevhfields(HFields *hf);
HSPairs			*hrevspairs(HSPairs *sp);
int8_t			*hstrdup(HConnect *c, int8_t *s);
int			http11(HConnect*);
int			httpfmt(Fmt*);
int8_t			*httpunesc(HConnect *c, int8_t *s);
int			hunallowed(HConnect *, int8_t *allowed);
int			hungetc(Hio *h);
int8_t			*hunload(Hio*);
int			hurlfmt(Fmt*);
int8_t			*hurlunesc(HConnect *c, int8_t *s);
int			hwrite(Hio*, void*, int);
int			hxferenc(Hio*, int);

#pragma			varargck	argpos	hprint	2

/*
 * D is httpd format date conversion
 * U is url escape convertsion
 * H is html escape conversion
 */
#pragma	varargck	type	"D"	long
#pragma	varargck	type	"D"	ulong
#pragma	varargck	type	"U"	char*
#pragma	varargck	type	"H"	char*
