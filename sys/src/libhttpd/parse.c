#include <u.h>
#include <libc.h>
#include <libsec.h>
#include <bin.h>
#include <httpd.h>
#include "escape.h"

typedef struct Error	Error;
typedef struct Hlex	Hlex;
typedef struct MimeHead	MimeHead;

enum
{
	/*
	 * tokens
	 */
	Word	= 1,
	QString,
};

#define UlongMax	4294967295UL

struct Error
{
	char	*num;
	char	*concise;
	char	*verbose;
};

struct Hlex
{
	int	tok;
	int	eoh;
	int	eol;			/* end of header line encountered? */
	uchar	*hstart;		/* start of header */
	jmp_buf	jmp;			/* jmp here to parse header */
	char	wordval[HMaxWord];
	HConnect *c;
};

struct MimeHead
{
	char	*name;
	void	(*parse)(Hlex*, char*);
	uchar	seen;
	uchar	ignore;
};

Error errormsg[] =
{
	[HInternal]	{"500 Internal Error", "Internal Error",
		"This server could not process your request due to an internal error."},
	[HTempFail]	{"500 Internal Error", "Temporary Failure",
		"The object %s is currently inaccessible.<p>Please try again later."},
	[HUnimp]	{"501 Not implemented", "Command not implemented",
		"This server does not implement the %s command."},
	[HUnkVers]	{"501 Not Implemented", "Unknown http version",
		"This server does not know how to respond to http version %s."},
	[HBadCont]	{"501 Not Implemented", "Impossible format",
		"This server cannot produce %s in any of the formats your client accepts."},
	[HBadReq]	{"400 Bad Request", "Strange Request",
		"Your client sent a query that this server could not understand."},
	[HSyntax]	{"400 Bad Request", "Garbled Syntax",
		"Your client sent a query with incoherent syntax."},
	[HBadSearch]	{"400 Bad Request", "Inapplicable Search",
		"Your client sent a search that cannot be applied to %s."},
	[HNotFound]	{"404 Not Found", "Object not found",
		"The object %s does not exist on this server."},
	[HNoSearch]	{"403 Forbidden", "Search not supported",
		"The object %s does not support the search command."},
	[HNoData]	{"403 Forbidden", "No data supplied",
		"Search or forms data must be supplied to %s."},
	[HExpectFail]	{"403 Expectation Failed", "Expectation Failed",
		"This server does not support some of your request's expectations."},
	[HUnauth]	{"403 Forbidden", "Forbidden",
		"You are not allowed to see the object %s."},
	[HOK]		{"200 OK", "everything is fine"},
};

static void	mimeaccept(Hlex*, char*);
static void	mimeacceptchar(Hlex*, char*);
static void	mimeacceptenc(Hlex*, char*);
static void	mimeacceptlang(Hlex*, char*);
static void	mimeagent(Hlex*, char*);
static void	mimeauthorization(Hlex*, char*);
static void	mimeconnection(Hlex*, char*);
static void	mimecontlen(Hlex*, char*);
static void	mimeexpect(Hlex*, char*);
static void	mimefresh(Hlex*, char*);
static void	mimefrom(Hlex*, char*);
static void	mimehost(Hlex*, char*);
static void	mimeifrange(Hlex*, char*);
static void	mimeignore(Hlex*, char*);
static void	mimematch(Hlex*, char*);
static void	mimemodified(Hlex*, char*);
static void	mimenomatch(Hlex*, char*);
static void	mimerange(Hlex*, char*);
static void	mimetransenc(Hlex*, char*);
static void	mimeunmodified(Hlex*, char*);

/*
 * headers seen also include
 * allow  cache-control chargeto
 * content-encoding content-language content-location content-md5 content-range content-type
 * date etag expires forwarded last-modified max-forwards pragma
 * proxy-agent proxy-authorization proxy-connection
 * ua-color ua-cpu ua-os ua-pixels
 * upgrade via x-afs-tokens x-serial-number
 */
static MimeHead	mimehead[] =
{
	{"accept",		mimeaccept},
	{"accept-charset",	mimeacceptchar},
	{"accept-encoding",	mimeacceptenc},
	{"accept-language",	mimeacceptlang},
	{"authorization",	mimeauthorization},
	{"connection",		mimeconnection},
	{"content-length",	mimecontlen},
	{"expect",		mimeexpect},
	{"fresh",		mimefresh},
	{"from",		mimefrom},
	{"host",		mimehost},
	{"if-match",		mimematch},
	{"if-modified-since",	mimemodified},
	{"if-none-match",	mimenomatch},
	{"if-range",		mimeifrange},
	{"if-unmodified-since",	mimeunmodified},
	{"range",		mimerange},
	{"transfer-encoding",	mimetransenc},
	{"user-agent",		mimeagent},
};

char*		hmydomain;
char*		hversion = "HTTP/1.1";

static	void	lexhead(Hlex*);
static	void	parsejump(Hlex*, char*);
static	int	getc(Hlex*);
static	void	ungetc(Hlex*);
static	int	wordcr(Hlex*);
static	int	wordnl(Hlex*);
static	void	word(Hlex*, char*);
static	int	lex1(Hlex*, int);
static	int	lex(Hlex*);
static	int	lexbase64(Hlex*);
static	ulong	digtoul(char *s, char **e);

/*
 * flush an clean up junk from a request
 */
void
hreqcleanup(HConnect *c)
{
	int i;

	hxferenc(&c->hout, 0);
	memset(&c->req, 0, sizeof(c->req));
	memset(&c->head, 0, sizeof(c->head));
	c->hpos = c->header;
	c->hstop = c->header;
	binfree(&c->bin);
	for(i = 0; i < nelem(mimehead); i++){
		mimehead[i].seen = 0;
		mimehead[i].ignore = 0;
	}
}

/*
 * list of tokens
 * if the client is HTTP/1.0,
 * ignore headers which match one of the tokens.
 * restarts parsing if necessary.
 */
static void
mimeconnection(Hlex *h, char *)
{
	char *u, *p;
	int reparse, i;

	reparse = 0;
	for(;;){
		while(lex(h) != Word)
			if(h->tok != ',')
				goto breakout;

		if(cistrcmp(h->wordval, "keep-alive") == 0)
			h->c->head.persist = 1;
		else if(cistrcmp(h->wordval, "close") == 0)
			h->c->head.closeit = 1;
		else if(!http11(h->c)){
			for(i = 0; i < nelem(mimehead); i++){
				if(cistrcmp(mimehead[i].name, h->wordval) == 0){
					reparse = mimehead[i].seen && !mimehead[i].ignore;
					mimehead[i].ignore = 1;
					if(cistrcmp(mimehead[i].name, "authorization") == 0){
						h->c->head.authuser = nil;
						h->c->head.authpass = nil;
					}
				}
			}
		}

		if(lex(h) != ',')
			break;
	}

breakout:;
	/*
	 * if need to ignore headers we've already parsed,
	 * reset & start over.  need to save authorization
	 * info because it's written over when parsed.
	 */
	if(reparse){
		u = h->c->head.authuser;
		p = h->c->head.authpass;
		memset(&h->c->head, 0, sizeof(h->c->head));
		h->c->head.authuser = u;
		h->c->head.authpass = p;

		h->c->hpos = h->hstart;
		longjmp(h->jmp, 1);
	}
}

int
hparseheaders(HConnect *c, int timeout)
{
	Hlex h;

	c->head.fresh_thresh = 0;
	c->head.fresh_have = 0;
	c->head.persist = 0;
	if(c->req.vermaj == 0){
		c->head.host = hmydomain;
		return 1;
	}

	memset(&h, 0, sizeof(h));
	h.c = c;
	alarm(timeout);
	hgethead(c, 1);
	alarm(0);
	h.hstart = c->hpos;

	if(setjmp(h.jmp) == -1)
		return -1;

	h.eol = 0;
	h.eoh = 0;
	h.tok = '\n';
	while(lex(&h) != '\n'){
		if(h.tok == Word && lex(&h) == ':')
			parsejump(&h, hstrdup(c, h.wordval));
		while(h.tok != '\n')
			lex(&h);
		h.eol = h.eoh;
	}

	if(http11(c)){
		/*
		 * according to the http/1.1 spec,
		 * these rules must be followed
		 */
		if(c->head.host == nil){
			hfail(c, HBadReq, nil);
			return -1;
		}
		if(c->req.urihost != nil)
			c->head.host = c->req.urihost;
		/*
		 * also need to check host is actually this one
		 */
	}else if(c->head.host == nil)
		c->head.host = hmydomain;
	return 1;
}

/*
 * mimeparams	: | mimeparams ";" mimepara
 * mimeparam	: token "=" token | token "=" qstring
 */
static HSPairs*
mimeparams(Hlex *h)
{
	HSPairs *p;
	char *s;

	p = nil;
	for(;;){
		if(lex(h) != Word)
			break;
		s = hstrdup(h->c, h->wordval);
		if(lex(h) != Word && h->tok != QString)
			break;
		p = hmkspairs(h->c, s, hstrdup(h->c, h->wordval), p);
	}
	return hrevspairs(p);
}

/*
 * mimehfields	: mimehfield | mimehfields commas mimehfield
 * mimehfield	: token mimeparams
 * commas	: "," | commas ","
 */
static HFields*
mimehfields(Hlex *h)
{
	HFields *f;

	f = nil;
	for(;;){
		while(lex(h) != Word)
			if(h->tok != ',')
				goto breakout;

		f = hmkhfields(h->c, hstrdup(h->c, h->wordval), nil, f);

		if(lex(h) == ';')
			f->params = mimeparams(h);
		if(h->tok != ',')
			break;
	}
breakout:;
	return hrevhfields(f);
}

/*
 * parse a list of acceptable types, encodings, languages, etc.
 */
static HContent*
mimeok(Hlex *h, char *name, int multipart, HContent *head)
{
	char *generic, *specific, *s;
	float v;

	/*
	 * each type is separated by one or more commas
	 */
	while(lex(h) != Word)
		if(h->tok != ',')
			return head;

	generic = hstrdup(h->c, h->wordval);
	lex(h);
	if(h->tok == '/' || multipart){
		/*
		 * at one time, IE5 improperly said '*' for single types
		 */
		if(h->tok != '/')
			return nil;
		if(lex(h) != Word)
			return head;
		specific = hstrdup(h->c, h->wordval);
		if(!multipart && strcmp(specific, "*") != 0)
			return head;
		lex(h);
	}else
		specific = nil;
	head = hmkcontent(h->c, generic, specific, head);

	for(;;){
		switch(h->tok){
		case ';':
			/*
			 * should make a list of these params
			 * for accept, they fall into two classes:
			 *	up to a q=..., they modify the media type.
			 *	afterwards, they acceptance criteria
			 */
			if(lex(h) == Word){
				s = hstrdup(h->c, h->wordval);
				if(lex(h) != '=' || lex(h) != Word && h->tok != QString)
					return head;
				v = strtod(h->wordval, nil);
				if(strcmp(s, "q") == 0)
					head->q = v;
				else if(strcmp(s, "mxb") == 0)
					head->mxb = v;
			}
			break;
		case ',':
			return  mimeok(h, name, multipart, head);
		default:
			return head;
		}
		lex(h);
	}
	return head;
}

/*
 * parse a list of entity tags
 * 1#entity-tag
 * entity-tag = [weak] opaque-tag
 * weak = "W/"
 * opaque-tag = quoted-string
 */
static HETag*
mimeetag(Hlex *h, HETag *head)
{
	HETag *e;
	int weak;

	for(;;){
		while(lex(h) != Word && h->tok != QString)
			if(h->tok != ',')
				return head;

		weak = 0;
		if(h->tok == Word && strcmp(h->wordval, "*") != 0){
			if(strcmp(h->wordval, "W") != 0)
				return head;
			if(lex(h) != '/' || lex(h) != QString)
				return head;
			weak = 1;
		}

		e = halloc(h->c, sizeof(HETag));
		e->etag = hstrdup(h->c, h->wordval);
		e->weak = weak;
		e->next = head;
		head = e;

		if(lex(h) != ',')
			return head;
	}
	return head;
}

/*
 * ranges-specifier = byte-ranges-specifier
 * byte-ranges-specifier = "bytes" "=" byte-range-set
 * byte-range-set = 1#(byte-range-spec|suffix-byte-range-spec)
 * byte-range-spec = byte-pos "-" [byte-pos]
 * byte-pos = 1*DIGIT
 * suffix-byte-range-spec = "-" suffix-length
 * suffix-length = 1*DIGIT
 *
 * syntactically invalid range specifiers cause the
 * entire header field to be ignored.
 * it is syntactically incorrect for the second byte pos
 * to be smaller than the first byte pos
 */
static HRange*
mimeranges(Hlex *h, HRange *head)
{
	HRange *r, *rh, *tail;
	char *w;
	ulong start, stop;
	int suf;

	if(lex(h) != Word || strcmp(h->wordval, "bytes") != 0 || lex(h) != '=')
		return head;

	rh = nil;
	tail = nil;
	for(;;){
		while(lex(h) != Word){
			if(h->tok != ','){
				if(h->tok == '\n')
					goto breakout;
				return head;
			}
		}

		w = h->wordval;
		start = 0;
		suf = 1;
		if(w[0] != '-'){
			suf = 0;
			start = digtoul(w, &w);
			if(w[0] != '-')
				return head;
		}
		w++;
		stop = ~0UL;
		if(w[0] != '\0'){
			stop = digtoul(w, &w);
			if(w[0] != '\0')
				return head;
			if(!suf && stop < start)
				return head;
		}

		r = halloc(h->c, sizeof(HRange));
		r->suffix = suf;
		r->start = start;
		r->stop = stop;
		r->next = nil;
		if(rh == nil)
			rh = r;
		else
			tail->next = r;
		tail = r;

		if(lex(h) != ','){
			if(h->tok == '\n')
				break;
			return head;
		}
	}
breakout:;

	if(head == nil)
		return rh;

	for(tail = head; tail->next != nil; tail = tail->next)
		;
	tail->next = rh;
	return head;
}

static void
mimeaccept(Hlex *h, char *name)
{
	h->c->head.oktype = mimeok(h, name, 1, h->c->head.oktype);
}

static void
mimeacceptchar(Hlex *h, char *name)
{
	h->c->head.okchar = mimeok(h, name, 0, h->c->head.okchar);
}

static void
mimeacceptenc(Hlex *h, char *name)
{
	h->c->head.okencode = mimeok(h, name, 0, h->c->head.okencode);
}

static void
mimeacceptlang(Hlex *h, char *name)
{
	h->c->head.oklang = mimeok(h, name, 0, h->c->head.oklang);
}

static void
mimemodified(Hlex *h, char *)
{
	lexhead(h);
	h->c->head.ifmodsince = hdate2sec(h->wordval);
}

static void
mimeunmodified(Hlex *h, char *)
{
	lexhead(h);
	h->c->head.ifunmodsince = hdate2sec(h->wordval);
}

static void
mimematch(Hlex *h, char *)
{
	h->c->head.ifmatch = mimeetag(h, h->c->head.ifmatch);
}

static void
mimenomatch(Hlex *h, char *)
{
	h->c->head.ifnomatch = mimeetag(h, h->c->head.ifnomatch);
}

/*
 * argument is either etag or date
 */
static void
mimeifrange(Hlex *h, char *)
{
	int c, d, et;

	et = 0;
	c = getc(h);
	while(c == ' ' || c == '\t')
		c = getc(h);
	if(c == '"')
		et = 1;
	else if(c == 'W'){
		d = getc(h);
		if(d == '/')
			et = 1;
		ungetc(h);
	}
	ungetc(h);
	if(et){
		h->c->head.ifrangeetag = mimeetag(h, h->c->head.ifrangeetag);
	}else{
		lexhead(h);
		h->c->head.ifrangedate = hdate2sec(h->wordval);
	}
}

static void
mimerange(Hlex *h, char *)
{
	h->c->head.range = mimeranges(h, h->c->head.range);
}

/*
 * note: netscape and ie through versions 4.7 and 4
 * support only basic authorization, so that is all that is supported here
 *
 * "Authorization" ":" "Basic" base64-user-pass
 * where base64-user-pass is the base64 encoding of
 * username ":" password
 */
static void
mimeauthorization(Hlex *h, char *)
{
	char *up, *p;
	int n;

	if(lex(h) != Word || cistrcmp(h->wordval, "basic") != 0)
		return;

	n = lexbase64(h);
	if(!n)
		return;

	/*
	 * wipe out source for password, so it won't be logged.
	 * it is replaced by a single =,
	 * which is valid base64, but not ok for an auth reponse.
	 * therefore future parses of the header field will not overwrite
	 * authuser and authpass.
	 */
	memmove(h->c->hpos - (n - 1), h->c->hpos, h->c->hstop - h->c->hpos);
	h->c->hstop -= n - 1;
	*h->c->hstop = '\0';
	h->c->hpos -= n - 1;
	h->c->hpos[-1] = '=';

	up = halloc(h->c, n + 1);
	n = dec64((uchar*)up, n, h->wordval, n);
	up[n] = '\0';
	p = strchr(up, ':');
	if(p != nil){
		*p++ = '\0';
		h->c->head.authuser = hstrdup(h->c, up);
		h->c->head.authpass = hstrdup(h->c, p);
	}
}

static void
mimeagent(Hlex *h, char *)
{
	lexhead(h);
	h->c->head.client = hstrdup(h->c, h->wordval);
}

static void
mimefrom(Hlex *h, char *)
{
	lexhead(h);
}

static void
mimehost(Hlex *h, char *)
{
	char *hd;

	lexhead(h);
	for(hd = h->wordval; *hd == ' ' || *hd == '\t'; hd++)
		;
	h->c->head.host = hlower(hstrdup(h->c, hd));
}

/*
 * if present, implies that a message body follows the headers
 * "content-length" ":" digits
 */
static void
mimecontlen(Hlex *h, char *)
{
	char *e;
	ulong v;

	if(lex(h) != Word)
		return;
	e = h->wordval;
	v = digtoul(e, &e);
	if(v == ~0UL || *e != '\0')
		return;
	h->c->head.contlen = v;
}

/*
 * mimexpect	: "expect" ":" expects
 * expects	: | expects "," expect
 * expect	: "100-continue" | token | token "=" token expectparams | token "=" qstring expectparams
 * expectparams	: ";" token | ";" token "=" token | token "=" qstring
 * for now, we merely parse "100-continue" or anything else.
 */
static void
mimeexpect(Hlex *h, char *)
{
	if(lex(h) != Word || cistrcmp(h->wordval, "100-continue") != 0 || lex(h) != '\n')
		h->c->head.expectother = 1;
	h->c->head.expectcont = 1;
}

static void
mimetransenc(Hlex *h, char *)
{
	h->c->head.transenc = mimehfields(h);
}

static void
mimefresh(Hlex *h, char *)
{
	char *s;

	lexhead(h);
	for(s = h->wordval; *s && (*s==' ' || *s=='\t'); s++)
		;
	if(strncmp(s, "pathstat/", 9) == 0)
		h->c->head.fresh_thresh = atoi(s+9);
	else if(strncmp(s, "have/", 5) == 0)
		h->c->head.fresh_have = atoi(s+5);
}

static void
mimeignore(Hlex *h, char *)
{
	lexhead(h);
}

static void
parsejump(Hlex *h, char *k)
{
	int l, r, m;

	l = 1;
	r = nelem(mimehead) - 1;
	while(l <= r){
		m = (r + l) >> 1;
		if(cistrcmp(mimehead[m].name, k) <= 0)
			l = m + 1;
		else
			r = m - 1;
	}
	m = l - 1;
	if(cistrcmp(mimehead[m].name, k) == 0 && !mimehead[m].ignore){
		mimehead[m].seen = 1;
		(*mimehead[m].parse)(h, k);
	}else
		mimeignore(h, k);
}

static int
lex(Hlex *h)
{
	return h->tok = lex1(h, 0);
}

static int
lexbase64(Hlex *h)
{
	int c, n;

	n = 0;
	lex1(h, 1);

	while((c = getc(h)) >= 0){
		if(!(c >= 'A' && c <= 'Z'
		|| c >= 'a' && c <= 'z'
		|| c >= '0' && c <= '9'
		|| c == '+' || c == '/')){
			ungetc(h);
			break;
		}

		if(n < HMaxWord-1)
			h->wordval[n++] = c;
	}
	h->wordval[n] = '\0';
	return n;
}

/*
 * rfc 822/rfc 1521 lexical analyzer
 */
static int
lex1(Hlex *h, int skipwhite)
{
	int level, c;

	if(h->eol)
		return '\n';

top:
	c = getc(h);
	switch(c){
	case '(':
		level = 1;
		while((c = getc(h)) >= 0){
			if(c == '\\'){
				c = getc(h);
				if(c < 0)
					return '\n';
				continue;
			}
			if(c == '(')
				level++;
			else if(c == ')' && --level == 0)
				break;
			else if(c == '\n'){
				c = getc(h);
				if(c < 0)
					return '\n';
				if(c == ')' && --level == 0)
					break;
				if(c != ' ' && c != '\t'){
					ungetc(h);
					return '\n';
				}
			}
		}
		goto top;

	case ' ': case '\t':
		goto top;

	case '\r':
		c = getc(h);
		if(c != '\n'){
			ungetc(h);
			goto top;
		}

	case '\n':
		if(h->tok == '\n'){
			h->eol = 1;
			h->eoh = 1;
			return '\n';
		}
		c = getc(h);
		if(c < 0){
			h->eol = 1;
			return '\n';
		}
		if(c != ' ' && c != '\t'){
			ungetc(h);
			h->eol = 1;
			return '\n';
		}
		goto top;

	case ')':
	case '<': case '>':
	case '[': case ']':
	case '@': case '/':
	case ',': case ';': case ':': case '?': case '=':
		if(skipwhite){
			ungetc(h);
			return c;
		}
		return c;

	case '"':
		if(skipwhite){
			ungetc(h);
			return c;
		}
		word(h, "\"");
		getc(h);		/* skip the closing quote */
		return QString;

	default:
		ungetc(h);
		if(skipwhite)
			return c;
		word(h, "\"(){}<>@,;:/[]?=\r\n \t");
		if(h->wordval[0] == '\0'){
			h->c->head.closeit = 1;
			hfail(h->c, HSyntax);
			longjmp(h->jmp, -1);
		}
		return Word;
	}
	goto top;
	return 0;
}

/*
 * return the rest of an rfc 822, including \n
 * do not map to lower case
 */
static void
lexhead(Hlex *h)
{
	int c, n;

	n = 0;
	while((c = getc(h)) >= 0){
		if(c == '\r')
			c = wordcr(h);
		else if(c == '\n')
			c = wordnl(h);
		if(c == '\n')
			break;
		if(c == '\\'){
			c = getc(h);
			if(c < 0)
				break;
		}

		if(n < HMaxWord-1)
			h->wordval[n++] = c;
	}
	h->tok = '\n';
	h->eol = 1;
	h->wordval[n] = '\0';
}

static void
word(Hlex *h, char *stop)
{
	int c, n;

	n = 0;
	while((c = getc(h)) >= 0){
		if(c == '\r')
			c = wordcr(h);
		else if(c == '\n')
			c = wordnl(h);
		if(c == '\\'){
			c = getc(h);
			if(c < 0)
				break;
		}else if(c < 32 || strchr(stop, c) != nil){
			ungetc(h);
			break;
		}

		if(n < HMaxWord-1)
			h->wordval[n++] = c;
	}
	h->wordval[n] = '\0';
}

static int
wordcr(Hlex *h)
{
	int c;

	c = getc(h);
	if(c == '\n')
		return wordnl(h);
	ungetc(h);
	return ' ';
}

static int
wordnl(Hlex *h)
{
	int c;

	c = getc(h);
	if(c == ' ' || c == '\t')
		return c;
	ungetc(h);

	return '\n';
}

static int
getc(Hlex *h)
{
	if(h->eoh)
		return -1;
	if(h->c->hpos < h->c->hstop)
		return *h->c->hpos++;
	h->eoh = 1;
	h->eol = 1;
	return -1;
}

static void
ungetc(Hlex *h)
{
	if(h->eoh)
		return;
	h->c->hpos--;
}

static ulong
digtoul(char *s, char **e)
{
	ulong v;
	int c, ovfl;

	v = 0;
	ovfl = 0;
	for(;;){
		c = *s;
		if(c < '0' || c > '9')
			break;
		s++;
		c -= '0';
		if(v > UlongMax/10 || v == UlongMax/10 && c >= UlongMax%10)
			ovfl = 1;
		v = v * 10 + c;
	}

	if(e)
		*e = s;
	if(ovfl)
		return UlongMax;
	return v;
}

/*
 * read in some header lines, either one or all of them.
 * copy results into header log buffer.
 */
int
hgethead(HConnect *c, int many)
{
	Hio *hin;
	char *s, *p, *pp;
	int n;

	hin = &c->hin;
	for(;;){
		s = (char*)hin->pos;
		pp = s;
		while(p = memchr(pp, '\n', (char*)hin->stop - pp)){
			if(!many || p == pp || p == pp + 1 && *pp == '\r'){
				pp = p + 1;
				break;
			}
			pp = p + 1;
		}
		hin->pos = (uchar*)pp;
		n = pp - s;
		if(c->hstop + n > &c->header[HBufSize])
			return 0;
		memmove(c->hstop, s, n);
		c->hstop += n;
		*c->hstop = '\0';
		if(p != nil)
			return 1;
		if(hreadbuf(hin, hin->pos) == nil)
			return 0;
	}
}

/* go from url with escaped utf to utf */
char *
hurlunesc(HConnect *cc, char *s)
{
	char *t, *v, *u;
	Rune r;
	int c, n;

	/* unescape */
	u = halloc(cc, strlen(s)+1);
	for(t = u; c = *s; s++){
		if(c == '%'){
			n = s[1];
			if(n >= '0' && n <= '9')
				n = n - '0';
			else if(n >= 'A' && n <= 'F')
				n = n - 'A' + 10;
			else if(n >= 'a' && n <= 'f')
				n = n - 'a' + 10;
			else
				break;
			r = n;
			n = s[2];
			if(n >= '0' && n <= '9')
				n = n - '0';
			else if(n >= 'A' && n <= 'F')
				n = n - 'A' + 10;
			else if(n >= 'a' && n <= 'f')
				n = n - 'a' + 10;
			else
				break;
			s += 2;
			c = (r<<4)+n;
		}
		*t++ = c;
	}
	*t = '\0';

	/* convert to valid utf */
	v = halloc(cc, UTFmax*strlen(u) + 1);
	s = u;
	t = v;
	while(*s){
		/* in decoding error, assume latin1 */
		if((n=chartorune(&r, s)) == 1 && r == Runeerror)
			r = (uchar)*s;
		s += n;
		t += runetochar(t, &r);
	}
	*t = '\0';

	return v;
}

/*
 *  go from http with latin1 escapes to utf,
 *  we assume that anything >= Runeself is already in utf
 */
char *
httpunesc(HConnect *cc, char *s)
{
	char *t, *v;
	int c;
	Htmlesc *e;

	v = halloc(cc, UTFmax*strlen(s) + 1);
	for(t = v; c = *s;){
		if(c == '&'){
			if(s[1] == '#' && s[2] && s[3] && s[4] && s[5] == ';'){
				c = atoi(s+2);
				if(c < Runeself){
					*t++ = c;
					s += 6;
					continue;
				}
				if(c < 256 && c >= 161){
					e = &htmlesc[c-161];
					t += runetochar(t, &e->value);
					s += 6;
					continue;
				}
			} else {
				for(e = htmlesc; e->name != nil; e++)
					if(strncmp(e->name, s, strlen(e->name)) == 0)
						break;
				if(e->name != nil){
					t += runetochar(t, &e->value);
					s += strlen(e->name);
					continue;
				}
			}
		}
		*t++ = c;
		s++;
	}
	*t = 0;
	return v;
}

/*
 * write initial part of successful header
 */
void
hokheaders(HConnect *c)
{
	Hio *hout;

	hout = &c->hout;
	hprint(hout, "%s 200 OK\r\n", hversion);
	hprint(hout, "Server: Plan9\r\n");
	hprint(hout, "Date: %D\r\n", time(nil));
	if(c->head.closeit)
		hprint(hout, "Connection: close\r\n");
	else if(!http11(c))
		hprint(hout, "Connection: Keep-Alive\r\n");
}

/*
 * write a failure message to the net and exit
 */
int
hfail(HConnect *c, int reason, ...)
{
	Hio *hout;
	char makeup[HBufSize];
	va_list arg;
	int n;

	hout = &c->hout;
	va_start(arg, reason);
	doprint(makeup, makeup+HBufSize, errormsg[reason].verbose, arg);
	va_end(arg);
	n = snprint(c->xferbuf, HBufSize, "<head><title>%s</title></head>\n<body><h1>%s</h1>\n%s</body>\n",
		errormsg[reason].concise, errormsg[reason].concise, makeup);

	hprint(hout, "%s %s\r\n", hversion, errormsg[reason].num);
	hprint(hout, "Date: %D\r\n", time(nil));
	hprint(hout, "Server: Plan9\r\n");
	hprint(hout, "Content-Type: text/html\r\n");
	hprint(hout, "Content-Length: %d\r\n", n);
	if(c->head.closeit)
		hprint(hout, "Connection: close\r\n");
	else if(!http11(c))
		hprint(hout, "Connection: Keep-Alive\r\n");
	hprint(hout, "\r\n");

	if(c->req.meth == nil || strcmp(c->req.meth, "HEAD") != 0)
		hwrite(hout, c->xferbuf, n);

	if(c->replog)
		c->replog(c, "Reply: %s\nReason: %s\n", errormsg[reason].num, errormsg[reason].concise);
	return hflush(hout);
}

int
hunallowed(HConnect *c, char *allowed)
{
	Hio *hout;
	int n;

	n = snprint(c->xferbuf, HBufSize, "<head><title>Method Not Allowed</title></head>\r\n"
		"<body><h1>Method Not Allowed</h1>\r\n"
		"You can't %s on <a href=\"%U\"> here</a>.<p></body>\r\n", c->req.meth, c->req.uri);

	hout = &c->hout;
	hprint(hout, "%s 405 Method Not Allowed\r\n", hversion);
	hprint(hout, "Date: %D\r\n", time(nil));
	hprint(hout, "Server: Plan9\r\n");
	hprint(hout, "Content-Type: text/html\r\n");
	hprint(hout, "Allow: %s\r\n", allowed);
	hprint(hout, "Content-Length: %d\r\n", n);
	if(c->head.closeit)
		hprint(hout, "Connection: close\r\n");
	else if(!http11(c))
		hprint(hout, "Connection: Keep-Alive\r\n");
	hprint(hout, "\r\n");

	if(strcmp(c->req.meth, "HEAD") != 0)
		hwrite(hout, c->xferbuf, n);

	if(c->replog)
		c->replog(c, "Reply: 405 Method Not Allowed\nReason: Method Not Allowed\n");
	return hflush(hout);
}

int
hredirected(HConnect *c, char *how, char *uri)
{
	Hio *hout;
	char *s, *ss, *host;
	int n;

	host = c->head.host;
	if(strchr(uri, ':')){
		host = "";
	}else if(uri[0] != '/'){
		s = strrchr(c->req.uri, '/');
		if(s != nil)
			*s = '\0';
		ss = halloc(c, strlen(c->req.uri) + strlen(uri) + 2 + UTFmax);
		sprint(ss, "%s/%s", c->req.uri, uri);
		uri = ss;
		if(s != nil)
			*s = '/';
	}

	n = snprint(c->xferbuf, HBufSize, 
			"<head><title>Redirection</title></head>\r\n"
			"<body><h1>Redirection</h1>\r\n"
			"Your selection can be found <a href=\"%U\"> here</a>.<p></body>\r\n", uri);

	hout = &c->hout;
	hprint(hout, "%s %s\r\n", hversion, how);
	hprint(hout, "Date: %D\r\n", time(nil));
	hprint(hout, "Server: Plan9\r\n");
	hprint(hout, "Content-type: text/html\r\n");
	hprint(hout, "Content-Length: %d\r\n", n);
	if(host == nil || host[0] == 0)
		hprint(hout, "Location: %U\r\n", uri);
	else
		hprint(hout, "Location: http://%U%U\r\n", host, uri);
	if(c->head.closeit)
		hprint(hout, "Connection: close\r\n");
	else if(!http11(c))
		hprint(hout, "Connection: Keep-Alive\r\n");
	hprint(hout, "\r\n");

	if(strcmp(c->req.meth, "HEAD") != 0)
		hwrite(hout, c->xferbuf, n);

	if(c->replog){
		if(host == nil || host[0] == 0)
			c->replog(c, "Reply: %s\nRedirect: %U\n", how, uri);
		else
			c->replog(c, "Reply: %s\nRedirect: http://%U%U\n", how, host, uri);
	}
	return hflush(hout);
}

int
hmoved(HConnect *c, char *uri)
{
	return hredirected(c, "301 Moved Permanently", uri);
}

int
http11(HConnect *c)
{
	return c->req.vermaj > 1 || c->req.vermaj == 1 && c->req.vermin > 0;
}

char*
hmkmimeboundary(HConnect *c)
{
	char buf[32];
	int i;

	srand((time(0)<<16)|getpid());
	strcpy(buf, "upas-");
	for(i = 5; i < sizeof(buf)-1; i++)
		buf[i] = 'a' + nrand(26);
	buf[i] = 0;
	return hstrdup(c, buf);
}

HSPairs*
hmkspairs(HConnect *c, char *s, char *t, HSPairs *next)
{
	HSPairs *sp;

	sp = halloc(c, sizeof *sp);
	sp->s = s;
	sp->t = t;
	sp->next = next;
	return sp;
}

HSPairs*
hrevspairs(HSPairs *sp)
{
	HSPairs *last, *next;

	last = nil;
	for(; sp != nil; sp = next){
		next = sp->next;
		sp->next = last;
		last = sp;
	}
	return last;
}

HFields*
hmkhfields(HConnect *c, char *s, HSPairs *p, HFields *next)
{
	HFields *hf;

	hf = halloc(c, sizeof *hf);
	hf->s = s;
	hf->params = p;
	hf->next = next;
	return hf;
}

HFields*
hrevhfields(HFields *hf)
{
	HFields *last, *next;

	last = nil;
	for(; hf != nil; hf = next){
		next = hf->next;
		hf->next = last;
		last = hf;
	}
	return last;
}

int
hcheckcontent(HContent *me, HContent *oks, char *list, int size)
{
	HContent *ok;

	if(oks == nil || me == nil)
		return 1;
	for(ok = oks; ok != nil; ok = ok->next){
		if((cistrcmp(ok->generic, me->generic) == 0 || strcmp(ok->generic, "*") == 0)
		&& (me->specific == nil || cistrcmp(ok->specific, me->specific) == 0 || strcmp(ok->specific, "*") == 0)){
			if(ok->mxb > 0 && size > ok->mxb)
				return 0;
			return 1;
		}
	}

	USED(list);
	if(0){
		fprint(2, "list: %s/%s not found\n", me->generic, me->specific);
		for(; oks != nil; oks = oks->next){
			if(oks->specific)
				fprint(2, "\t%s/%s\n", oks->generic, oks->specific);
			else
				fprint(2, "\t%s\n", oks->generic);
		}
	}
	return 0;
}

HContent*
hmkcontent(HConnect *c, char *generic, char *specific, HContent *next)
{
	HContent *ct;

	ct = halloc(c, sizeof(HContent));
	ct->generic = generic;
	ct->specific = specific;
	ct->next = next;
	ct->q = 1;
	ct->mxb = 0;
	return ct;
}

int
hurlconv(va_list *arg, Fconv *f)
{
	char buf[HMaxWord*2];
	Rune r;
	char *s;
	int t;

	s = va_arg(*arg, char*);
	for(t = 0; t < sizeof(buf) - 8; ){
		s += chartorune(&r, s);
		if(r == 0)
			break;
		if(r <= ' ' || r == '%' || r >= Runeself)
			t += snprint(&buf[t], sizeof(buf)-t, "%%%2.2x", r);
		else
			buf[t++] = r;
	}
	buf[t] = 0;
	strconv(buf, f);
	return 0;
}

int
httpconv(va_list *arg, Fconv *f)
{
	char buf[HMaxWord*2];
	Rune r;
	char *t, *s;
	Htmlesc *l;

	s = va_arg(*arg, char*);
	for(t = buf; t < buf + sizeof(buf) - 8; ){
		s += chartorune(&r, s);
		if(r == 0)
			break;
		for(l = htmlesc; l->name != nil; l++)
			if(l->value == r)
				break;
		if(l->name != nil){
			strcpy(t, l->name);
			t += strlen(t);
		}else
			*t++ = r;
	}
	*t = 0;
	strconv(buf, f);
	return 0;
}

/*
 * memory allocators:
 * e... routines call malloc and fatal error if out of memory
 * they should be used by initialization code only
 *
 * h routines call canalloc; they should be used by everything else
 * note this memory is wiped out at the start of each new request
 */
char*
hstrdup(HConnect *c, char *s)
{
	char *t;
	int n;

	n = strlen(s) + 1;
	t = binalloc(&c->bin, n, 0);
	if(t == nil)
		sysfatal("out of memory");
	memmove(t, s, n);
	return t;
}

void*
halloc(HConnect *c, ulong n)
{
	void *p;

	p = binalloc(&c->bin, n, 1);
	if(p == nil)
		sysfatal("out of memory");
	return p;
}

char*
hlower(char *p)
{
	char c;
	char *x;

	if(p == nil)
		return p;

	for(x = p; c = *x; x++)
		if(c >= 'A' && c <= 'Z')
			*x -= 'A' - 'a';
	return p;
}
