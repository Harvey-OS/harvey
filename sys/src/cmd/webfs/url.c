/*
 * This is a URL parser, written to parse "Common Internet Scheme" URL
 * syntax as described in RFC1738 and updated by RFC2396.  Only absolute URLs 
 * are supported, using "server-based" naming authorities in the schemes.
 * Support for literal IPv6 addresses is included, per RFC2732.
 *
 * Current "known" schemes: http, ftp, file.
 *
 * We can do all the parsing operations without Runes since URLs are
 * defined to be composed of US-ASCII printable characters.
 * See RFC1738, RFC2396.
 */

#include <u.h>
#include <libc.h>
#include <ctype.h>
#include <regexp.h>
#include <plumb.h>
#include <thread.h>
#include <fcall.h>
#include <9p.h>
#include "dat.h"
#include "fns.h"

int urldebug;

/* If set, relative paths with leading ".." segments will have them trimmed */
#define RemoveExtraRelDotDots	0
#define ExpandCurrentDocUrls	1

static char*
schemestrtab[] =
{
	nil,
	"http",
	"https",
	"ftp",
	"file",
};

static int
ischeme(char *s)
{
	int i;

	for(i=0; i<nelem(schemestrtab); i++)
		if(schemestrtab[i] && strcmp(s, schemestrtab[i])==0)
			return i;
	return USunknown;
}

/*
 * URI splitting regexp is from RFC2396, Appendix B: 
 *		^(([^:/?#]+):)?(//([^/?#]*))?([^?#]*)(\?([^#]*))?(#(.*))?
 *		 12            3  4          5       6  7        8 9
 *
 * Example: "http://www.ics.uci.edu/pub/ietf/uri/#Related"
 * $2 = scheme			"http"
 * $4 = authority		"www.ics.uci.edu"
 * $5 = path			"/pub/ietf/uri/"
 * $7 = query			<undefined>
 * $9 = fragment		"Related"
 */

/*
 * RFC2396, Sec 3.1, contains:
 *
 * Scheme names consist of a sequence of characters beginning with a
 * lower case letter and followed by any combination of lower case
 * letters, digits, plus ("+"), period ("."), or hyphen ("-").  For
 * resiliency, programs interpreting URI should treat upper case letters
 * as equivalent to lower case in scheme names (e.g., allow "HTTP" as
 * well as "http").
 */

/*
 * For server-based naming authorities (RFC2396 Sec 3.2.2):
 *    server        = [ [ userinfo "@" ] hostport ]
 *    userinfo      = *( unreserved | escaped |
 *                      ";" | ":" | "&" | "=" | "+" | "$" | "," )
 *    hostport      = host [ ":" port ]
 *    host          = hostname | IPv4address
 *    hostname      = *( domainlabel "." ) toplabel [ "." ]
 *    domainlabel   = alphanum | alphanum *( alphanum | "-" ) alphanum
 *    toplabel      = alpha | alpha *( alphanum | "-" ) alphanum
 *    IPv4address   = 1*digit "." 1*digit "." 1*digit "." 1*digit
 *    port          = *digit
 *
 *  The host is a domain name of a network host, or its IPv4 address as a
 *  set of four decimal digit groups separated by ".".  Literal IPv6
 *  addresses are not supported.
 *
 * Note that literal IPv6 address support is outlined in RFC2732:
 *    host          = hostname | IPv4address | IPv6reference
 *    ipv6reference = "[" IPv6address "]"		(RFC2373)
 *
 * Since hostnames and numbers will have to be resolved by the OS anyway,
 * we don't have to parse them too pedantically (counting '.'s, checking 
 * for well-formed literal IP addresses, etc.).
 *
 * In FTP/file paths, we reject most ";param"s and querys.  In HTTP paths,
 * we just pass them through.
 *
 * Instead of letting a "path" be 0-or-more characters as RFC2396 suggests, 
 * we'll say it's 1-or-more characters, 0-or-1 times.  This way, an absent
 * path yields a nil substring match, instead of an empty one.
 *
 * We're more restrictive than RFC2396 indicates with "userinfo" strings,
 * insisting they have the form "[user[:password]]".  This may need to
 * change at some point, however.
 */

/* RE character-class components -- these go in brackets */
#define PUNCT			"\\-_.!~*'()"
#define RES			";/?:@&=+$,"
#define ALNUM		"a-zA-Z0-9"
#define HEX			"0-9a-fA-F"
#define UNRES			ALNUM PUNCT

/* RE components; _N => has N parenthesized subexpressions when expanded */
#define ESCAPED_1			"(%[" HEX "][" HEX "])"
#define URIC_2			"([" RES UNRES "]|" ESCAPED_1 ")"
#define URICNOSLASH_2		"([" UNRES ";?:@&=+$,]|" ESCAPED_1 ")"
#define USERINFO_2		"([" UNRES ";:&=+$,]|" ESCAPED_1 ")"
#define PCHAR_2			"([" UNRES ":@&=+$,]|" ESCAPED_1 ")"
#define PSEGCHAR_3		"([/;]|" PCHAR_2 ")"

typedef struct Retab Retab;
struct Retab
{
	char	*str;
	Reprog	*prog;
	int		size;
	int		ind[5];
};

enum
{
	REsplit = 0,
	REscheme,
	REunknowndata,
	REauthority,
	REhost,
	REuserinfo,
	REabspath,
	REquery,
	REfragment,
	REhttppath,
	REftppath,
	REfilepath,

	MaxResub=	20,
};

Retab retab[] =	/* view in constant width Font */
{
[REsplit]
	"^(([^:/?#]+):)?(//([^/?#]*))?([^?#]+)?(\\?([^#]*))?(#(.*))?$", nil, 0,
	/* |-scheme-|      |-auth.-|  |path--|    |query|     |--|frag */
	{  2,              4,         5,          7,          9},

[REscheme]
	"^[a-z][a-z0-9+-.]*$", nil, 0,
	{ 0, },

[REunknowndata]
	"^" URICNOSLASH_2 URIC_2 "*$", nil, 0,
	{ 0, },

[REauthority]
	"^(((" USERINFO_2 "*)@)?(((\\[[^\\]@]+\\])|([^:\\[@]+))(:([0-9]*))?)?)?$", nil, 0,
	/* |----user info-----|  |--------host----------------|  |-port-| */
	{  3,                    7,                              11, },

[REhost]
	"^(([a-zA-Z0-9\\-.]+)|(\\[([a-fA-F0-9.:]+)\\]))$", nil, 0,
	/* |--regular host--|     |-IPv6 literal-| */
	{  2,                     4, },

[REuserinfo]
	"^(([^:]*)(:([^:]*))?)$", nil, 0,
	/* |user-|  |pass-| */
	{  2,       4, },

[REabspath]
	"^/" PSEGCHAR_3 "*$", nil, 0,
	{ 0, },

[REquery]
	"^" URIC_2 "*$", nil, 0,
	{ 0, },

[REfragment]
	"^" URIC_2 "*$", nil, 0,
	{ 0, },

[REhttppath]
	"^.*$", nil, 0,
	{ 0, },

[REftppath]
	"^(.+)(;[tT][yY][pP][eE]=([aAiIdD]))?$", nil, 0,
	/*|--|-path              |ftptype-| */
	{ 1,                     3, }, 

[REfilepath]
	"^.*$", nil, 0,
	{ 0, },
};

static int
countleftparen(char *s)
{
	int n;

	n = 0;
	for(; *s; s++)
		if(*s == '(')
			n++;
	return n;
}

void
initurl(void)
{
	int i, j;

	for(i=0; i<nelem(retab); i++){
		retab[i].prog = regcomp(retab[i].str);
		if(retab[i].prog == nil)
			sysfatal("recomp(%s): %r", retab[i].str);
		retab[i].size = countleftparen(retab[i].str)+1;
		for(j=0; j<nelem(retab[i].ind); j++)
			if(retab[i].ind[j] >= retab[i].size)
				sysfatal("bad index in regexp table: retab[%d].ind[%d] = %d >= %d",
					i, j, retab[i].ind[j], retab[i].size);
		if(MaxResub < retab[i].size)
			sysfatal("MaxResub too small: %d < %d", MaxResub, retab[i].size);
	}
}

typedef struct SplitUrl SplitUrl;
struct SplitUrl
{
	struct {
		char *s;
		char *e;
	} url, scheme, authority, path, query, fragment;
};

/*
 * Implements the algorithm in RFC2396 sec 5.2 step 6.
 * Returns number of chars written, excluding NUL terminator.
 * dest is known to be >= strlen(base)+rel_len.
 */
static void
merge_relative_path(char *base, char *rel_st, int rel_len, char *dest)
{
	char *s, *p, *e, *pdest;

	pdest = dest;

	/* 6a: start with base, discard last segment */
	if(base && base[0]){
		/* Empty paths don't match in our scheme; 'base' should be nil */
		assert(base[0] == '/');
		e = strrchr(base, '/');
		e++;
		memmove(pdest, base, e-base);
		pdest += e-base;
	}else{
		/* Artistic license on my part */
		*pdest++ = '/';
	}

	/* 6b: append relative component */
	if(rel_st){
		memmove(pdest, rel_st, rel_len);
		pdest += rel_len;
	}

	/* 6c: remove any occurrences of "./" as a complete segment */
	s = dest;
	*pdest = '\0';
	while(e = strstr(s, "./")){
		if((e == dest) || (*(e-1) == '/')){
 			memmove(e, e+2, pdest+1-(e+2));	/* +1 for NUL */
			pdest -= 2;
		}else
			s = e+1;
	}

	/* 6d: remove a trailing "." as a complete segment */
	if(pdest>dest && *(pdest-1)=='.' && 
	  (pdest==dest+1 || *(pdest-2)=='/'))
		*--pdest = '\0';

	/* 6e: remove occurences of "seg/../", where seg != "..", left->right */
	s = dest+1;
	while(e = strstr(s, "/../")){
		p = e - 1;
		while(p >= dest && *p != '/')
			p--;
		if(memcmp(p, "/../", 4) != 0){
			memmove(p+1, e+4, pdest+1-(e+4));
			pdest -= (e+4) - (p+1);
		}else
			s = e+1;
	}

	/* 6f: remove a trailing "seg/..", where seg isn't ".."  */
	if(pdest-3 > dest && memcmp(pdest-3, "/..", 3)==0){
		p = pdest-3 - 1;
		while(p >= dest && *p != '/')
			p--;
		if(memcmp(p, "/../", 4) != 0){
			pdest = p+1;
			*pdest = '\0';
		}
	}

	/* 6g: leading ".." segments are errors -- we'll just blat them out. */
	if(RemoveExtraRelDotDots){
		p = dest;
		if (p[0] == '/')
			p++;
		s = p;
		while(s[0]=='.' && s[1]=='.' && (s[2]==0 || s[2]=='/'))
			s += 3;
		if(s > p){
			memmove(p, s, pdest+1-s);
			pdest -= s-p;
		}
	}
	USED(pdest);

	if(urldebug)
		fprint(2, "merge_relative_path: '%s' + '%.*s' -> '%s'\n", base, rel_len, 
			rel_st, dest);
}

/*
 * See RFC2396 sec 5.2 for info on resolving relative URIs to absolute form.
 *
 * If successful, this just ends up freeing and replacing "u->url".
 */
static int
resolve_relative(SplitUrl *su, Url *base, Url *u)
{
	char *url, *path;
	char *purl, *ppath;
	int currentdoc, ulen, plen;

	if(base == nil){
		werrstr("relative URI given without base");
		return -1;
	}
	if(base->scheme == nil){
		werrstr("relative URI given with no scheme");
		return -1;
	}
	if(base->ischeme == USunknown){
		werrstr("relative URI given with unknown scheme");
		return -1;
	}
	if(base->ischeme == UScurrent){
		werrstr("relative URI given with incomplete base");
		return -1;
	}
	assert(su->scheme.s == nil);

	/* Sec 5.2 step 2 */
	currentdoc = 0;
	if(su->path.s==nil && su->scheme.s==nil && su->authority.s==nil && su->query.s==nil){
		/* Reference is to current document */
		if(urldebug)
			fprint(2, "url %s is relative to current document\n", u->url);
		u->ischeme = UScurrent;
		if(!ExpandCurrentDocUrls)
			return 0;
		currentdoc = 1;
	}
	
	/* Over-estimate the maximum lengths, for allocation purposes */
	/* (constants are for separators) */
	plen = 1;
	if(base->path)
		plen += strlen(base->path);
	if(su->path.s)
		plen += 1 + (su->path.e - su->path.s);

	ulen = 0;
	ulen += strlen(base->scheme) + 1;
	if(su->authority.s)
		ulen += 2 + (su->authority.e - su->authority.s);
	else
		ulen += 2 + ((base->authority) ? strlen(base->authority) : 0);
	ulen += plen;
	if(su->query.s)
		ulen += 1 + (su->query.e - su->query.s);
	else if(currentdoc && base->query)
		ulen += 1 + strlen(base->query);
	if(su->fragment.s)
		ulen += 1 + (su->fragment.e - su->fragment.s);
	else if(currentdoc && base->fragment)
		ulen += 1 + strlen(base->fragment);
	url = emalloc(ulen+1);
	path = emalloc(plen+1);

	url[0] = '\0';
	purl = url;
	path[0] = '\0';
	ppath = path;

	if(su->authority.s || (su->path.s && (su->path.s[0] == '/'))){
		/* Is a "network-path" or "absolute-path"; don't merge with base path */
		/* Sec 5.2 steps 4,5 */
		if(su->path.s){
			memmove(ppath, su->path.s, su->path.e - su->path.s);
			ppath += su->path.e - su->path.s;
			*ppath = '\0';
		}
	}else if(currentdoc){
		/* Is a current-doc reference; just copy the path from the base URL */
		if(base->path){
			strcpy(ppath, base->path);
			ppath += strlen(ppath);
		}
		USED(ppath);
	}else{
		/* Is a relative-path reference; we have to merge it */
		/* Sec 5.2 step 6 */
		merge_relative_path(base->path,
			su->path.s, su->path.e - su->path.s, ppath);
	}

	/* Build new URL from pieces, inheriting from base where needed */
	strcpy(purl, base->scheme);
	purl += strlen(purl);
	*purl++ = ':';
	if(su->authority.s){
		strcpy(purl, "//");
		purl += strlen(purl);
		memmove(purl, su->authority.s, su->authority.e - su->authority.s);
		purl += su->authority.e - su->authority.s;
	}else if(base->authority){
		strcpy(purl, "//");
		purl += strlen(purl);
		strcpy(purl, base->authority);
		purl += strlen(purl);
	}
	assert((path[0] == '\0') || (path[0] == '/'));
	strcpy(purl, path);
	purl += strlen(purl);

	/*
	 * The query and fragment are not inherited from the base,
	 * except in case of "current document" URLs, which inherit any query
	 * and may inherit the fragment.
	 */
	if(su->query.s){
		*purl++ = '?';
		memmove(purl, su->query.s, su->query.e - su->query.s);
		purl += su->query.e - su->query.s;
	}else if(currentdoc && base->query){
		*purl++ = '?';
		strcpy(purl, base->query);
		purl += strlen(purl);
	}

	if(su->fragment.s){
		*purl++ = '#';
		memmove(purl, su->query.s, su->query.e - su->query.s);
		purl += su->fragment.e - su->fragment.s;
	}else if(currentdoc && base->fragment){
		*purl++ = '#';
		strcpy(purl, base->fragment);
		purl += strlen(purl);
	}
	USED(purl);

	if(urldebug)
		fprint(2, "resolve_relative: '%s' + '%s' -> '%s'\n", base->url, u->url, url);
	free(u->url);
	u->url = url;
	free(path);
	return 0;
}

int
regx(Reprog *prog, char *s, Resub *m, int nm)
{
	int i;

	if(s == nil)
		s = m[0].sp;	/* why is this necessary? */

	i = regexec(prog, s, m, nm);
/*
	if(i >= 0)
		for(j=0; j<nm; j++)
			fprint(2, "match%d: %.*s\n", j, utfnlen(m[j].sp, m[j].ep-m[j].sp), m[j].sp);
*/
	return i;
}

static int
ismatch(int i, char *s, char *desc)
{
	Resub m[1];

	m[0].sp = m[0].ep = nil;
	if(!regx(retab[i].prog, s, m, 1)){
		werrstr("malformed %s: %q", desc, s);
		return 0;
	}
	return 1;
}

static int
spliturl(char *url, SplitUrl *su)
{
	Resub m[MaxResub];
	Retab *t;

	/*
	 * Newlines are not valid in a URI, but regexp(2) treats them specially 
	 * so it's best to make sure there are none before proceeding.
	 */
	if(strchr(url, '\n')){
		werrstr("newline in URI");
		return -1;
	}

	/*
	 * Because we use NUL-terminated strings, as do many client and server
	 * implementations, an escaped NUL ("%00") will quite likely cause problems
	 * when unescaped.  We can check for such a sequence once before examining
 	 * the components because, per RFC2396 sec. 2.4.1 - 2.4.2, '%' is reserved
	 * in URIs to _always_ indicate escape sequences.  Something like "%2500"
	 * will still get by, but that's legitimate, and if it ends up causing
	 * a NUL then someone is unescaping too many times.
	 */
	if(strstr(url, "%00")){
		werrstr("escaped NUL in URI");
		return -1;
	}

	m[0].sp = m[0].ep = nil;
	t = &retab[REsplit];
	if(!regx(t->prog, url, m, t->size)){
		werrstr("malformed URI: %q", url);
		return -1;
	}

	su->url.s = m[0].sp;
	su->url.e = m[0].ep;
	su->scheme.s = m[t->ind[0]].sp;
	su->scheme.e = m[t->ind[0]].ep;
	su->authority.s = m[t->ind[1]].sp;
	su->authority.e = m[t->ind[1]].ep;
	su->path.s = m[t->ind[2]].sp;
	su->path.e = m[t->ind[2]].ep;
	su->query.s = m[t->ind[3]].sp;
	su->query.e = m[t->ind[3]].ep;
	su->fragment.s = m[t->ind[4]].sp;
	su->fragment.e = m[t->ind[4]].ep;

	if(urldebug)
		fprint(2, "split url %s into %.*q %.*q %.*q %.*q %.*q %.*q\n",
			url,
			su->url.s ? utfnlen(su->url.s, su->url.e-su->url.s) : 10, su->url.s ? su->url.s : "",
			su->scheme.s ? utfnlen(su->scheme.s, su->scheme.e-su->scheme.s) : 10, su->scheme.s ? su->scheme.s : "",
			su->authority.s ? utfnlen(su->authority.s, su->authority.e-su->authority.s) : 10, su->authority.s ? su->authority.s : "",
			su->path.s ? utfnlen(su->path.s, su->path.e-su->path.s) : 10, su->path.s ? su->path.s : "",
			su->query.s ? utfnlen(su->query.s, su->query.e-su->query.s) : 10, su->query.s ? su->query.s : "",
			su->fragment.s ? utfnlen(su->fragment.s, su->fragment.e-su->fragment.s) : 10, su->fragment.s ? su->fragment.s : "");

	return 0;
}

static int
parse_scheme(SplitUrl *su, Url *u)
{
	if(su->scheme.s == nil){
		werrstr("missing scheme");
		return -1;
	}
	u->scheme = estredup(su->scheme.s, su->scheme.e);
	strlower(u->scheme);

	if(!ismatch(REscheme, u->scheme, "scheme"))
		return -1;

	u->ischeme = ischeme(u->scheme);
	if(urldebug)
		fprint(2, "parse_scheme %s => %d\n", u->scheme, u->ischeme);
	return 0;
}

static int
parse_unknown_part(SplitUrl *su, Url *u)
{
	char *s, *e;

	assert(u->ischeme == USunknown);
	assert(su->scheme.e[0] == ':');

	s = su->scheme.e+1;
	if(su->fragment.s){
		e = su->fragment.s-1;
		assert(*e == '#');
	}else
		e = s+strlen(s);

	u->schemedata = estredup(s, e);
	if(!ismatch(REunknowndata, u->schemedata, "unknown scheme data"))
		return -1;
	return 0;
}

static int
parse_userinfo(char *s, char *e, Url *u)
{
	Resub m[MaxResub];
	Retab *t;

	m[0].sp = s;
	m[0].ep = e;
	t = &retab[REuserinfo];
	if(!regx(t->prog, nil, m, t->size)){
		werrstr("malformed userinfo: %.*q", utfnlen(s, e-s), s);
		return -1;
	}
	if(m[t->ind[0]].sp)
		u->user = estredup(m[t->ind[0]].sp, m[t->ind[0]].ep);
	if(m[t->ind[1]].sp)
		u->user = estredup(m[t->ind[1]].sp, m[t->ind[1]].ep);
	return 0;
}

static int
parse_host(char *s, char *e, Url *u)
{
	Resub m[MaxResub];
	Retab *t;

	m[0].sp = s;
	m[0].ep = e;
	t = &retab[REhost];
	if(!regx(t->prog, nil, m, t->size)){
		werrstr("malformed host: %.*q", utfnlen(s, e-s), s);
		return -1;
	}

	assert(m[t->ind[0]].sp || m[t->ind[1]].sp);

	if(m[t->ind[0]].sp)	/* regular */
		u->host = estredup(m[t->ind[0]].sp, m[t->ind[0]].ep);
	else
		u->host = estredup(m[t->ind[1]].sp, m[t->ind[1]].ep);
	return 0;
}

static int
parse_authority(SplitUrl *su, Url *u)
{
	Resub m[MaxResub];
	Retab *t;
	char *host;
	char *userinfo;

	if(su->authority.s == nil)
		return 0;

	u->authority = estredup(su->authority.s, su->authority.e);
	m[0].sp = m[0].ep = nil;
	t = &retab[REauthority];
	if(!regx(t->prog, u->authority, m, t->size)){
		werrstr("malformed authority: %q", u->authority);
		return -1;
	}

	if(m[t->ind[0]].sp)
		if(parse_userinfo(m[t->ind[0]].sp, m[t->ind[0]].ep, u) < 0)
			return -1;
	if(m[t->ind[1]].sp)
		if(parse_host(m[t->ind[1]].sp, m[t->ind[1]].ep, u) < 0)
			return -1;
	if(m[t->ind[2]].sp)
		u->port = estredup(m[t->ind[2]].sp, m[t->ind[2]].ep);


	if(urldebug > 0){
		userinfo = estredup(m[t->ind[0]].sp, m[t->ind[0]].ep); 
		host = estredup(m[t->ind[1]].sp, m[t->ind[1]].ep);
		fprint(2, "port: %q, authority %q\n", u->port, u->authority);
		fprint(2, "host %q, userinfo %q\n", host, userinfo);
		free(host);
		free(userinfo);
	}
	return 0;
}

static int
parse_abspath(SplitUrl *su, Url *u)
{
	if(su->path.s == nil)
		return 0;
	u->path = estredup(su->path.s, su->path.e);
	if(!ismatch(REabspath, u->path, "absolute path"))
		return -1;
	return 0;
}

static int
parse_query(SplitUrl *su, Url *u)
{
	if(su->query.s == nil)
		return 0;
	u->query = estredup(su->query.s, su->query.e);
	if(!ismatch(REquery, u->query, "query"))
		return -1;
	return 0;
}

static int
parse_fragment(SplitUrl *su, Url *u)
{
	if(su->fragment.s == nil)
		return 0;
	u->fragment = estredup(su->fragment.s, su->fragment.e);
	if(!ismatch(REfragment, u->fragment, "fragment"))
		return -1;
	return 0;
}

static int
postparse_http(Url *u)
{
	u->open = httpopen;
	u->read = httpread;
	u->close = httpclose;

	if(u->authority==nil){
		werrstr("missing authority (hostname, port, etc.)");
		return -1;
	}
	if(u->host == nil){
		werrstr("missing host specification");
		return -1;
	}

	if(u->path == nil){
		u->http.page_spec = estrdup("/");
		return 0;
	}

	if(!ismatch(REhttppath, u->path, "http path"))
		return -1;
	if(u->query){
		u->http.page_spec = emalloc(strlen(u->path)+1+strlen(u->query)+1);
		strcpy(u->http.page_spec, u->path);
		strcat(u->http.page_spec, "?");
		strcat(u->http.page_spec, u->query);
	}else
		u->http.page_spec = estrdup(u->path);

	return 0;
}

static int
postparse_ftp(Url *u)
{
	Resub m[MaxResub];
	Retab *t;

	if(u->authority==nil){
		werrstr("missing authority (hostname, port, etc.)");
		return -1;
	}
	if(u->query){
		werrstr("unexpected \"?query\" in ftp path");
		return -1;
	}
	if(u->host == nil){
		werrstr("missing host specification");
		return -1;
	}

	if(u->path == nil){
		u->ftp.path_spec = estrdup("/");
		return 0;
	}

	m[0].sp = m[0].ep = nil;
	t = &retab[REftppath];
	if(!regx(t->prog, u->path, m, t->size)){
		werrstr("malformed ftp path: %q", u->path);
		return -1;
	}

	if(m[t->ind[0]].sp){
		u->ftp.path_spec = estredup(m[t->ind[0]].sp, m[t->ind[0]].ep);
		if(strchr(u->ftp.path_spec, ';')){
			werrstr("unexpected \";param\" in ftp path");
			return -1;
		}
	}else
		u->ftp.path_spec = estrdup("/");

	if(m[t->ind[1]].sp){
		u->ftp.type = estredup(m[t->ind[1]].sp, m[t->ind[1]].ep);
		strlower(u->ftp.type);
	}
	return 0;
}

static int
postparse_file(Url *u)
{
	if(u->user || u->passwd){
		werrstr("user information not valid with file scheme");
		return -1;
	}
	if(u->query){
		werrstr("unexpected \"?query\" in file path");
		return -1;
	}
	if(u->port){
		werrstr("port not valid with file scheme");
		return -1;
	}
	if(u->path == nil){
		werrstr("missing path in file scheme");
		return -1;
	}
	if(strchr(u->path, ';')){
		werrstr("unexpected \";param\" in file path");
		return -1;
	}

	if(!ismatch(REfilepath, u->path, "file path"))
		return -1;

	/* "localhost" is equivalent to no host spec, we'll chose the latter */
	if(u->host && cistrcmp(u->host, "localhost") == 0){
		free(u->host);
		u->host = nil;
	}
	return 0;
}

static int (*postparse[])(Url*) = {
	nil,
	postparse_http,
	postparse_http,
	postparse_ftp,
	postparse_file,
};

Url*
parseurl(char *url, Url *base)
{
	Url *u;
	SplitUrl su;

	if(urldebug)
		fprint(2, "parseurl %s with base %s\n", url, base ? base->url : "<none>");

	u = emalloc(sizeof(Url));
	u->url = estrdup(url);
	if(spliturl(u->url, &su) < 0){
	Fail:
		freeurl(u);
		return nil;
	}

	/* RFC2396 sec 3.1 says relative URIs are distinguished by absent scheme */ 
	if(su.scheme.s==nil){
		if(urldebug)
			fprint(2, "parseurl has nil scheme\n");
		if(resolve_relative(&su, base, u) < 0 || spliturl(u->url, &su) < 0)
			goto Fail;
		if(u->ischeme == UScurrent){
			/* 'u.url' refers to current document; set fragment and return */
			if(parse_fragment(&su, u) < 0)
				goto Fail;
			return u;
		}
	}

	if(parse_scheme(&su, u) < 0
	|| parse_fragment(&su, u) < 0)
		goto Fail;

	if(u->ischeme == USunknown){
		if(parse_unknown_part(&su, u) < 0)
			goto Fail;
		return u;
	}

	if(parse_query(&su, u) < 0
	|| parse_authority(&su, u) < 0
	|| parse_abspath(&su, u) < 0)
		goto Fail;

	if(u->ischeme < nelem(postparse) && postparse[u->ischeme])
		if((*postparse[u->ischeme])(u) < 0)
			goto Fail;

	setmalloctag(u, getcallerpc(&url));
	return u;
}

void
freeurl(Url *u)
{
	if(u == nil)
		return;
	free(u->url);
	free(u->scheme);
	free(u->schemedata);
	free(u->authority);
	free(u->user);
	free(u->passwd);
	free(u->host);
	free(u->port);
	free(u->path);
	free(u->query);
	free(u->fragment);
	switch(u->ischeme){
	case UShttp:
		free(u->http.page_spec);
		break;
	case USftp:
		free(u->ftp.path_spec);
		free(u->ftp.type);
		break;
	}
	free(u);
}

void
rewriteurl(Url *u)
{
	char *s;

	if(u->schemedata)
		s = estrmanydup(u->scheme, ":", u->schemedata, nil);
	else
		s = estrmanydup(u->scheme, "://", 
			u->user ? u->user : "",
			u->passwd ? ":" : "", u->passwd ? u->passwd : "",
			u->user ? "@" : "", u->host ? u->host : "", 
			u->port ? ":" : "", u->port ? u->port : "",
			u->path,
			u->query ? "?" : "", u->query ? u->query : "",
			u->fragment ? "#" : "", u->fragment ? u->fragment : "",
			nil);
	free(u->url);
	u->url = s;
}

int
seturlquery(Url *u, char *query)
{
	if(query == nil){
		free(u->query);
		u->query = nil;
		return 0;
	}

	if(!ismatch(REquery, query, "query"))
		return -1;

	free(u->query);
	u->query = estrdup(query);
	return 0;
}

static void
dupp(char **p)
{
	if(*p)
		*p = estrdup(*p);
}

Url*
copyurl(Url *u)
{
	Url *v;

	v = emalloc(sizeof(Url));
	*v = *u;
	dupp(&v->url);
	dupp(&v->scheme);
	dupp(&v->schemedata);
	dupp(&v->authority);
	dupp(&v->user);
	dupp(&v->passwd);
	dupp(&v->host);
	dupp(&v->port);
	dupp(&v->path);
	dupp(&v->query);
	dupp(&v->fragment);

	switch(v->ischeme){
	case UShttp:
		dupp(&v->http.page_spec);
		break;
	case USftp:
		dupp(&v->ftp.path_spec);
		dupp(&v->ftp.type);
		break;
	}
	return v;
}

static int
dhex(char c)
{
	if('0' <= c && c <= '9')
		return c-'0';
	if('a' <= c && c <= 'f')
		return c-'a'+10;
	if('A' <= c && c <= 'F')
		return c-'A'+10;
	return 0;
}

char*
escapeurl(char *s, int (*needesc)(int))
{
	int n;
	char *t, *u;
	Rune r;
	static char *hex = "0123456789abcdef";

	n = 0;
	for(t=s; *t; t++)
		if((*needesc)(*t))
			n++;

	u = emalloc(strlen(s)+2*n+1);
	t = u;
	for(; *s; s++){
		s += chartorune(&r, s);
		if(r >= 0xFF){
			werrstr("URLs cannot contain Runes > 0xFF");
			free(t);
			return nil;
		}
		if((*needesc)(r)){
			*u++ = '%';
			*u++ = hex[(r>>4)&0xF];
			*u++ = hex[r&0xF];
		}else
			*u++ = r;
	}
	*u = '\0';
	return t;
}

char*
unescapeurl(char *s)
{
	char *r, *w;
	Rune rune;

	s = estrdup(s);
	for(r=w=s; *r; r++){
		if(*r=='%'){
			r++;
			if(!isxdigit(r[0]) || !isxdigit(r[1])){
				werrstr("bad escape sequence '%.3s' in URL", r);
				return nil;
			}
			if(r[0]=='0' && r[2]=='0'){
				werrstr("escaped NUL in URL");
				return nil;
			}
			rune = (dhex(r[0])<<4)|dhex(r[1]);	/* latin1 */
			w += runetochar(w, &rune);
			r += 2;
		}else
			*w++ = *r;
	}
	*w = '\0';
	return s;
}

