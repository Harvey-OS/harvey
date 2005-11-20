/*
 * Cookie file system.  Allows hget and multiple webfs's to collaborate.
 * Conventionally mounted on /mnt/webcookies.
 */

#include <u.h>
#include <libc.h>
#include <bio.h>
#include <ndb.h>
#include <fcall.h>
#include <thread.h>
#include <9p.h>
#include <ctype.h>

int debug = 0;

typedef struct Cookie Cookie;
typedef struct Jar Jar;

struct Cookie
{
	/* external info */
	char*	name;
	char*	value;
	char*	dom;		/* starts with . */
	char*	path;
	char*	version;
	char*	comment;	/* optional, may be nil */

	uint	expire;		/* time of expiration: ~0 means when webcookies dies */
	int	secure;
	int	explicitdom;	/* dom was explicitly set */
	int	explicitpath;	/* path was explicitly set */
	int	netscapestyle;

	/* internal info */
	int	deleted;
	int	mark;
	int	ondisk;
};

struct Jar
{
	Cookie	*c;
	int	nc;
	int	mc;

	Qid	qid;
	int	dirty;
	char	*file;
	char	*lockfile;
};

struct {
	char	*s;
	int	offset;
	int	ishttp;
} stab[] = {
	"domain",		offsetof(Cookie, dom),		1,
	"path",			offsetof(Cookie, path),		1,
	"name",			offsetof(Cookie, name),		0,
	"value",		offsetof(Cookie, value),	0,
	"comment",		offsetof(Cookie, comment),	1,
	"version",		offsetof(Cookie, version),	1,
};

struct {
	char *s;
	int	offset;
} itab[] = {
	"expire",		offsetof(Cookie, expire),
	"secure",		offsetof(Cookie, secure),
	"explicitdomain",	offsetof(Cookie, explicitdom),
	"explicitpath",		offsetof(Cookie, explicitpath),
	"netscapestyle",	offsetof(Cookie, netscapestyle),
};

#pragma varargck type "J"	Jar*
#pragma varargck type "K"	Cookie*

/* HTTP format */
int
jarfmt(Fmt *fmt)
{
	int i;
	Jar *jar;

	jar = va_arg(fmt->args, Jar*);
	if(jar == nil || jar->nc == 0)
		return fmtstrcpy(fmt, "");

	fmtprint(fmt, "Cookie: ");
	if(jar->c[0].version)
		fmtprint(fmt, "$Version=%s; ", jar->c[0].version);
	for(i=0; i<jar->nc; i++)
		fmtprint(fmt, "%s%s=%s", i ? "; ":"", jar->c[i].name, jar->c[i].value);
	fmtprint(fmt, "\r\n");
	return 0;
}

/* individual cookie */
int
cookiefmt(Fmt *fmt)
{
	int j, k, first;
	char *t;
	Cookie *c;

	c = va_arg(fmt->args, Cookie*);

	first = 1;
	for(j=0; j<nelem(stab); j++){
		t = *(char**)((char*)c+stab[j].offset);
		if(t == nil)
			continue;
		if(first)
			first = 0;
		else
			fmtprint(fmt, " ");
		fmtprint(fmt, "%s=%q", stab[j].s, t);
	}
	for(j=0; j<nelem(itab); j++){
		k = *(int*)((char*)c+itab[j].offset);
		if(k == 0)
			continue;
		if(first)
			first = 0;
		else
			fmtprint(fmt, " ");
		fmtprint(fmt, "%s=%ud", itab[j].s, k);
	}
	return 0;
}

/*
 * sort cookies:
 *	- alpha by name
 *	- alpha by domain
 *	- longer paths first, then alpha by path (RFC2109 4.3.4)
 */
int
cookiecmp(Cookie *a, Cookie *b)
{
	int i;

	if((i = strcmp(a->name, b->name)) != 0)
		return i;
	if((i = cistrcmp(a->dom, b->dom)) != 0)
		return i;
	if((i = strlen(b->path) - strlen(a->path)) != 0)
		return i;
	if((i = strcmp(a->path, b->path)) != 0)
		return i;
	return 0;
}

int
exactcookiecmp(Cookie *a, Cookie *b)
{
	int i;

	if((i = cookiecmp(a, b)) != 0)
		return i;
	if((i = strcmp(a->value, b->value)) != 0)
		return i;
	if(a->version || b->version){
		if(!a->version)
			return -1;
		if(!b->version)
			return 1;
		if((i = strcmp(a->version, b->version)) != 0)
			return i;
	}
	if(a->comment || b->comment){
		if(!a->comment)
			return -1;
		if(!b->comment)
			return 1;
		if((i = strcmp(a->comment, b->comment)) != 0)
			return i;
	}
	if((i = b->expire - a->expire) != 0)
		return i;
	if((i = b->secure - a->secure) != 0)
		return i;
	if((i = b->explicitdom - a->explicitdom) != 0)
		return i;
	if((i = b->explicitpath - a->explicitpath) != 0)
		return i;
	if((i = b->netscapestyle - a->netscapestyle) != 0)
		return i;

	return 0;
}

void
freecookie(Cookie *c)
{
	int i;

	for(i=0; i<nelem(stab); i++)
		free(*(char**)((char*)c+stab[i].offset));
}

void
copycookie(Cookie *c)
{
	int i;
	char **ps;

	for(i=0; i<nelem(stab); i++){
		ps = (char**)((char*)c+stab[i].offset);
		if(*ps)
			*ps = estrdup9p(*ps);
	}
}

void
delcookie(Jar *j, Cookie *c)
{
	int i;

	j->dirty = 1;
	i = c - j->c;
	if(i < 0 || i >= j->nc)
		abort();
	c->deleted = 1;
}

void
addcookie(Jar *j, Cookie *c)
{
	int i;

	if(!c->name || !c->value || !c->path || !c->dom){
		fprint(2, "not adding incomplete cookie\n");
		return;
	}

	if(debug)
		fprint(2, "add %K\n", c);

	for(i=0; i<j->nc; i++)
		if(cookiecmp(&j->c[i], c) == 0){
			if(debug)
				fprint(2, "cookie %K matches %K\n", &j->c[i], c);
			if(exactcookiecmp(&j->c[i], c) == 0){
				if(debug)
					fprint(2, "\texactly\n");
				j->c[i].mark = 0;
				return;
			}
			delcookie(j, &j->c[i]);
		}

	j->dirty = 1;
	if(j->nc == j->mc){
		j->mc += 16;
		j->c = erealloc9p(j->c, j->mc*sizeof(Cookie));
	}
	j->c[j->nc] = *c;
	copycookie(&j->c[j->nc]);
	j->nc++;
}

void
purgejar(Jar *j)
{
	int i;

	for(i=j->nc-1; i>=0; i--){
		if(!j->c[i].deleted)
			continue;
		freecookie(&j->c[i]);
		--j->nc;
		j->c[i] = j->c[j->nc];
	}
}

void
addtojar(Jar *jar, char *line, int ondisk)
{
	Cookie c;
	int i, j, nf, *pint;
	char *f[20], *attr, *val, **pstr;
	
	memset(&c, 0, sizeof c);
	c.expire = ~0;
	c.ondisk = ondisk;
	nf = tokenize(line, f, nelem(f));
	for(i=0; i<nf; i++){
		attr = f[i];
		if((val = strchr(attr, '=')) != nil)
			*val++ = '\0';
		else
			val = "";
		/* string attributes */
		for(j=0; j<nelem(stab); j++){
			if(strcmp(stab[j].s, attr) == 0){
				pstr = (char**)((char*)&c+stab[j].offset);
				*pstr = val;
			}
		}
		/* integer attributes */
		for(j=0; j<nelem(itab); j++){
			if(strcmp(itab[j].s, attr) == 0){
				pint = (int*)((char*)&c+itab[j].offset);
				if(val[0]=='\0')
					*pint = 1;
				else
					*pint = strtoul(val, 0, 0);
			}
		}
	}
	if(c.name==nil || c.value==nil || c.dom==nil || c.path==nil){
		if(debug)
			fprint(2, "ignoring fractional cookie %K\n", &c);
		return;
	}
	addcookie(jar, &c);
}

Jar*
newjar(void)
{
	Jar *jar;

	jar = emalloc9p(sizeof(Jar));
	return jar;
}

int
expirejar(Jar *jar, int exiting)
{
	int i, n;
	uint now;

	now = time(0);
	n = 0;
	for(i=0; i<jar->nc; i++){
		if(jar->c[i].expire < now || (exiting && jar->c[i].expire==~0)){
			delcookie(jar, &jar->c[i]);
			n++;
		}
	}
	return n;
}

int
syncjar(Jar *jar)
{
	int i, fd;
	char *line;
	Dir *d;
	Biobuf *b;
	Qid q;

	if(jar->file==nil)
		return 0;

	memset(&q, 0, sizeof q);
	if((d = dirstat(jar->file)) != nil){
		q = d->qid;
		if(d->qid.path != jar->qid.path || d->qid.vers != jar->qid.vers)
			jar->dirty = 1;
		free(d);
	}

	if(jar->dirty == 0)
		return 0;

	fd = -1;
	for(i=0; i<50; i++){
		if((fd = create(jar->lockfile, OWRITE, DMEXCL|0666)) < 0){
			sleep(100);
			continue;
		}
		break;
	}
	if(fd < 0){
		if(debug)
			fprint(2, "open %s: %r", jar->lockfile);
		werrstr("cannot acquire jar lock: %r");
		return -1;
	}

	for(i=0; i<jar->nc; i++)	/* mark is cleared by addcookie */
		jar->c[i].mark = jar->c[i].ondisk;

	if((b = Bopen(jar->file, OREAD)) == nil){
		if(debug)
			fprint(2, "Bopen %s: %r", jar->file);
		werrstr("cannot read cookie file %s: %r", jar->file);
		close(fd);
		return -1;
	}
	for(; (line = Brdstr(b, '\n', 1)) != nil; free(line)){
		if(*line == '#')
			continue;
		addtojar(jar, line, 1);
	}
	Bterm(b);

	for(i=0; i<jar->nc; i++)
		if(jar->c[i].mark)
			delcookie(jar, &jar->c[i]);

	purgejar(jar);

	b = Bopen(jar->file, OWRITE);
	if(b == nil){
		if(debug)
			fprint(2, "Bopen write %s: %r", jar->file);
		close(fd);
		return -1;
	}
	Bprint(b, "# webcookies cookie jar\n");
	Bprint(b, "# comments and non-standard fields will be lost\n");
	for(i=0; i<jar->nc; i++){
		if(jar->c[i].expire == ~0)
			continue;
		Bprint(b, "%K\n", &jar->c[i]);
		jar->c[i].ondisk = 1;
	}
	Bterm(b);

	jar->dirty = 0;
	close(fd);
	if((d = dirstat(jar->file)) != nil){
		jar->qid = d->qid;
		free(d);
	}
	return 0;
}

Jar*
readjar(char *file)
{
	char *lock, *p;
	Jar *jar;

	jar = newjar();
	lock = emalloc9p(strlen(file)+10);
	strcpy(lock, file);
	if((p = strrchr(lock, '/')) != nil)
		p++;
	else
		p = lock;
	memmove(p+2, p, strlen(p)+1);
	p[0] = 'L';
	p[1] = '.';
	jar->lockfile = lock;
	jar->file = file;
	jar->dirty = 1;

	if(syncjar(jar) < 0){
		free(jar->file);
		free(jar->lockfile);
		free(jar);
		return nil;
	}
	return jar;
}

void
closejar(Jar *jar)
{
	int i;

	expirejar(jar, 0);
	if(syncjar(jar) < 0)
		fprint(2, "warning: cannot rewrite cookie jar: %r\n");

	for(i=0; i<jar->nc; i++)
		freecookie(&jar->c[i]);

	free(jar->file);
	free(jar);	
}

/*
 * Domain name matching is per RFC2109, section 2:
 *
 * Hosts names can be specified either as an IP address or a FQHN
 * string.  Sometimes we compare one host name with another.  Host A's
 * name domain-matches host B's if
 *
 * * both host names are IP addresses and their host name strings match
 *   exactly; or
 *
 * * both host names are FQDN strings and their host name strings match
 *   exactly; or
 *
 * * A is a FQDN string and has the form NB, where N is a non-empty name
 *   string, B has the form .B', and B' is a FQDN string.  (So, x.y.com
 *   domain-matches .y.com but not y.com.)
 *
 * Note that domain-match is not a commutative operation: a.b.c.com
 * domain-matches .c.com, but not the reverse.
 *
 * (This does not verify that IP addresses and FQDN's are well-formed.)
 */
int
isdomainmatch(char *name, char *pattern)
{
	int lname, lpattern;

	if(cistrcmp(name, pattern)==0)
		return 1;

	if(strcmp(ipattr(name), "dom")==0 && pattern[0]=='.'){
		lname = strlen(name);
		lpattern = strlen(pattern);
		if(lname >= lpattern && cistrcmp(name+lname-lpattern, pattern)==0)
			return 1;
	}

	return 0;
}

/*
 * RFC2109 4.3.4:
 *	- domain must match
 *	- path in cookie must be a prefix of request path
 *	- cookie must not have expired
 */
int
iscookiematch(Cookie *c, char *dom, char *path, uint now)
{
	return isdomainmatch(dom, c->dom)
		&& strncmp(c->path, path, strlen(c->path))==0
		&& c->expire >= now;
}

/* 
 * Produce a subjar of matching cookies.
 * Secure cookies are only included if secure is set.
 */
Jar*
cookiesearch(Jar *jar, char *dom, char *path, int issecure)
{
	int i;
	Jar *j;
	uint now;

	now = time(0);
	j = newjar();
	for(i=0; i<jar->nc; i++)
		if((issecure || !jar->c[i].secure) && iscookiematch(&jar->c[i], dom, path, now))
			addcookie(j, &jar->c[i]);
	if(j->nc == 0){
		closejar(j);
		werrstr("no cookies found");
		return nil;
	}
	qsort(j->c, j->nc, sizeof(j->c[0]), (int(*)(const void*, const void*))cookiecmp);
	return j;
}

/*
 * RFC2109 4.3.2 security checks
 */
char*
isbadcookie(Cookie *c, char *dom, char *path)
{
	if(strncmp(c->path, path, strlen(c->path)) != 0)
		return "cookie path is not a prefix of the request path";

	if(c->explicitdom && c->dom[0] != '.')
		return "cookie domain doesn't start with dot";

	if(memchr(c->dom+1, '.', strlen(c->dom)-1-1) == nil)
		return "cookie domain doesn't have embedded dots";

	if(!isdomainmatch(dom, c->dom))
		return "request host does not match cookie domain";

	if(strcmp(ipattr(dom), "dom")==0
	&& memchr(dom, '.', strlen(dom)-strlen(c->dom)) != nil)
		return "request host contains dots before cookie domain";

	return 0;
}

/*
 * Sunday, 25-Jan-2002 12:24:36 GMT
 * Sunday, 25 Jan 2002 12:24:36 GMT
 * Sun, 25 Jan 02 12:24:36 GMT
 */
int
isleap(int year)
{
	return year%4==0 && (year%100!=0 || year%400==0);
}

uint
strtotime(char *s)
{
	char *os;
	int i;
	Tm tm;

	static int mday[2][12] = {
		31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31,
		31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31,
	};
	static char *wday[] = {
		"Sunday", "Monday", "Tuesday", "Wednesday",
		"Thursday", "Friday", "Saturday",
	};
	static char *mon[] = {
		"Jan", "Feb", "Mar", "Apr", "May", "Jun",
		"Jul", "Aug", "Sep", "Oct", "Nov", "Dec",
	};

	os = s;
	/* Sunday, */
	for(i=0; i<nelem(wday); i++){
		if(cistrncmp(s, wday[i], strlen(wday[i])) == 0){
			s += strlen(wday[i]);
			break;
		}
		if(cistrncmp(s, wday[i], 3) == 0){
			s += 3;
			break;
		}
	}
	if(i==nelem(wday)){
		if(debug)
			fprint(2, "bad wday (%s)\n", os);
		return -1;
	}
	if(*s++ != ',' || *s++ != ' '){
		if(debug)
			fprint(2, "bad wday separator (%s)\n", os);
		return -1;
	}

	/* 25- */
	if(!isdigit(s[0]) || !isdigit(s[1]) || (s[2]!='-' && s[2]!=' ')){
		if(debug)
			fprint(2, "bad day of month (%s)\n", os);
		return -1;
	}
	tm.mday = strtol(s, 0, 10);
	s += 3;

	/* Jan- */
	for(i=0; i<nelem(mon); i++)
		if(cistrncmp(s, mon[i], 3) == 0){
			tm.mon = i;
			s += 3;
			break;
		}
	if(i==nelem(mon)){
		if(debug)
			fprint(2, "bad month (%s)\n", os);
		return -1;
	}
	if(s[0] != '-' && s[0] != ' '){
		if(debug)
			fprint(2, "bad month separator (%s)\n", os);
		return -1;
	}
	s++;

	/* 2002 */
	if(!isdigit(s[0]) || !isdigit(s[1])){
		if(debug)
			fprint(2, "bad year (%s)\n", os);
		return -1;
	}
	tm.year = strtol(s, 0, 10);
	s += 2;
	if(isdigit(s[0]) && isdigit(s[1]))
		s += 2;
	else{
		if(tm.year <= 68)
			tm.year += 2000;
		else
			tm.year += 1900;
	}
	if(tm.mday==0 || tm.mday > mday[isleap(tm.year)][tm.mon]){
		if(debug)
			fprint(2, "invalid day of month (%s)\n", os);
		return -1;
	}
	tm.year -= 1900;
	if(*s++ != ' '){
		if(debug)
			fprint(2, "bad year separator (%s)\n", os);
		return -1;
	}

	if(!isdigit(s[0]) || !isdigit(s[1]) || s[2]!=':'
	|| !isdigit(s[3]) || !isdigit(s[4]) || s[5]!=':'
	|| !isdigit(s[6]) || !isdigit(s[7]) || s[8]!=' '){
		if(debug)
			fprint(2, "bad time (%s)\n", os);
		return -1;
	}

	tm.hour = atoi(s);
	tm.min = atoi(s+3);
	tm.sec = atoi(s+6);
	if(tm.hour >= 24 || tm.min >= 60 || tm.sec >= 60){
		if(debug)
			fprint(2, "invalid time (%s)\n", os);
		return -1;
	}
	s += 9;

	if(cistrcmp(s, "GMT") != 0){
		if(debug)
			fprint(2, "time zone not GMT (%s)\n", os);
		return -1;
	}
	strcpy(tm.zone, "GMT");
	tm.yday = 0;
	return tm2sec(&tm);
}

/*
 * skip linear whitespace.  we're a bit more lenient than RFC2616 2.2.
 */
char*
skipspace(char *s)
{
	while(*s=='\r' || *s=='\n' || *s==' ' || *s=='\t')
		s++;
	return s;
}

/*
 * Try to identify old netscape headers.
 * The old headers:
 *	- didn't allow spaces around the '='
 *	- used an 'Expires' attribute
 *	- had no 'Version' attribute
 *	- had no quotes
 *	- allowed whitespace in values
 *	- apparently separated attr/value pairs with ';' exclusively
 */
int
isnetscape(char *hdr)
{
	char *s;

	for(s=hdr; (s=strchr(s, '=')) != nil; s++){
		if(isspace(s[1]) || (s > hdr && isspace(s[-1])))
			return 0;
		if(s[1]=='"')
			return 0;
	}
	if(cistrstr(hdr, "version="))
		return 0;
	return 1;
}

/*
 * Parse HTTP response headers, adding cookies to jar.
 * Overwrites the headers.  May overwrite path.
 */
char* parsecookie(Cookie*, char*, char**, int, char*, char*);
int
parsehttp(Jar *jar, char *hdr, char *dom, char *path)
{
	static char setcookie[] = "Set-Cookie:";
	char *e, *p, *nextp;
	Cookie c;
	int isns, n;

	isns = isnetscape(hdr);
	n = 0;
	for(p=hdr; p; p=nextp){
		p = skipspace(p);
		if(*p == '\0')
			break;
		nextp = strchr(p, '\n');
		if(nextp != nil)
			*nextp++ = '\0';
		if(debug)
			fprint(2, "?%s\n", p);
		if(cistrncmp(p, setcookie, strlen(setcookie)) != 0)
			continue;
		if(debug)
			fprint(2, "%s\n", p);
		p = skipspace(p+strlen(setcookie));
		for(; *p; p=skipspace(p)){
			if((e = parsecookie(&c, p, &p, isns, dom, path)) != nil){
				if(debug)
					fprint(2, "parse cookie: %s\n", e);
				break;
			}
			if((e = isbadcookie(&c, dom, path)) != nil){
				if(debug)
					fprint(2, "reject cookie; %s\n", e);
				continue;
			}
			addcookie(jar, &c);
			n++;
		}
	}
	return n;
}

static char*
skipquoted(char *s)
{
	/*
	 * Sec 2.2 of RFC2616 defines a "quoted-string" as:
	 *
	 *  quoted-string  = ( <"> *(qdtext | quoted-pair ) <"> )
	 *  qdtext         = <any TEXT except <">>
	 *  quoted-pair    = "\" CHAR
	 *
	 * TEXT is any octet except CTLs, but including LWS;
	 * LWS is [CR LF] 1*(SP | HT);
	 * CHARs are ASCII octets 0-127;  (NOTE: we reject 0's)
	 * CTLs are octets 0-31 and 127;
	 */
	if(*s != '"')
		return s;

	for(s++; 32 <= *s && *s < 127 && *s != '"'; s++)
		if(*s == '\\' && *(s+1) != '\0')
			s++;
	return s;
}

static char*
skiptoken(char *s)
{
	/*
	 * Sec 2.2 of RFC2616 defines a "token" as
 	 *  1*<any CHAR except CTLs or separators>;
	 * CHARs are ASCII octets 0-127;
	 * CTLs are octets 0-31 and 127;
	 * separators are "()<>@,;:\/[]?={}", double-quote, SP (32), and HT (9)
	 */
	while(32 <= *s && *s < 127 && strchr("()<>@,;:[]?={}\" \t\\", *s)==nil)
		s++;

	return s;
}

static char*
skipvalue(char *s, int isns)
{
	char *t;

	/*
	 * An RFC2109 value is an HTTP token or an HTTP quoted string.
	 * Netscape servers ignore the spec and rely on semicolons, apparently.
	 */
	if(isns){
		if((t = strchr(s, ';')) == nil)
			t = s+strlen(s);
		return t;
	}
	if(*s == '"')
		return skipquoted(s);
	return skiptoken(s);
}

/*
 * RMID=80b186bb64c03c65fab767f8; expires=Monday, 10-Feb-2003 04:44:39 GMT; 
 *	path=/; domain=.nytimes.com
 */
char*
parsecookie(Cookie *c, char *p, char **e, int isns, char *dom, char *path)
{
	int i, done;
	char *t, *u, *attr, *val;

	memset(c, 0, sizeof *c);
	c->expire = ~0;

	/* NAME=VALUE */
	t = skiptoken(p);
	c->name = p;
	p = skipspace(t);
	if(*p != '='){
	Badname:
		return "malformed cookie: no NAME=VALUE";
	}
	*t = '\0';
	p = skipspace(p+1);
	t = skipvalue(p, isns);
	if(*t)
		*t++ = '\0';
	c->value = p;
	p = skipspace(t);
	if(c->name[0]=='\0' || c->value[0]=='\0')
		goto Badname;

	done = 0;
	for(; *p && !done; p=skipspace(p)){
		attr = p;
		t = skiptoken(p);
		u = skipspace(t);
		switch(*u){
		case '\0':
			*t = '\0';
			p = val = u;
			break;
		case ';':
			*t = '\0';
			val = "";
			p = u+1;
			break;
		case '=':
			*t = '\0';
			val = skipspace(u+1);
			p = skipvalue(val, isns);
			if(*p==',')
				done = 1;
			if(*p)
				*p++ = '\0';
			break;
		case ',':
			if(!isns){
				val = "";
				p = u;
				*p++ = '\0';
				done = 1;
				break;
			}
		default:
			if(debug)
				fprint(2, "syntax: %s\n", p);
			return "syntax error";
		}
		for(i=0; i<nelem(stab); i++)
			if(stab[i].ishttp && cistrcmp(stab[i].s, attr)==0)
				*(char**)((char*)c+stab[i].offset) = val;
		if(cistrcmp(attr, "expires") == 0){
			if(!isns)
				return "non-netscape cookie has Expires tag";
			if(!val[0])
				return "bad expires tag";
			c->expire = strtotime(val);
			if(c->expire == ~0)
				return "cannot parse netscape expires tag";
		}
		if(cistrcmp(attr, "max-age") == 0)
			c->expire = time(0)+atoi(val);
		if(cistrcmp(attr, "secure") == 0)
			c->secure = 1;
	}

	if(c->dom)
		c->explicitdom = 1;
	else
		c->dom = dom;
	if(c->path)
		c->explicitpath = 1;
	else{
		c->path = path;
		if((t = strchr(c->path, '?')) != 0)
			*t = '\0';
		if((t = strrchr(c->path, '/')) != 0)
			*t = '\0';
	}
	c->netscapestyle = isns;
	*e = p;

	return nil;
}

Jar *jar;

enum
{
	Xhttp = 1,
	Xcookies,

	NeedUrl = 0,
	HaveUrl,
};

typedef struct Aux Aux;
struct Aux
{
	int state;
	char *dom;
	char *path;
	char *inhttp;
	char *outhttp;
	char *ctext;
	int rdoff;
};
enum
{
	AuxBuf = 4096,
	MaxCtext = 16*1024*1024,
};

void
fsopen(Req *r)
{
	char *s, *es;
	int i, sz;
	Aux *a;

	switch((uintptr)r->fid->file->aux){
	case Xhttp:
		syncjar(jar);
		a = emalloc9p(sizeof(Aux));
		r->fid->aux = a;
		a->inhttp = emalloc9p(AuxBuf);
		a->outhttp = emalloc9p(AuxBuf);
		break;

	case Xcookies:
		syncjar(jar);
		a = emalloc9p(sizeof(Aux));
		r->fid->aux = a;
		if(r->ifcall.mode&OTRUNC){
			a->ctext = emalloc9p(1);
			a->ctext[0] = '\0';
		}else{
			sz = 256*jar->nc+1024;	/* BUG should do better */
			a->ctext = emalloc9p(sz);
			a->ctext[0] = '\0';
			s = a->ctext;
			es = s+sz;
			for(i=0; i<jar->nc; i++)
				s = seprint(s, es, "%K\n", &jar->c[i]);
		}
		break;
	}
	respond(r, nil);
}

void
fsread(Req *r)
{
	Aux *a;

	a = r->fid->aux;
	switch((uintptr)r->fid->file->aux){
	case Xhttp:
		if(a->state == NeedUrl){
			respond(r, "must write url before read");
			return;
		}
		r->ifcall.offset = a->rdoff;
		readstr(r, a->outhttp);
		a->rdoff += r->ofcall.count;
		respond(r, nil);
		return;

	case Xcookies:
		readstr(r, a->ctext);
		respond(r, nil);
		return;

	default:
		respond(r, "bug in webcookies");
		return;
	}
}

void
fswrite(Req *r)
{
	Aux *a;
	int i, sz, hlen, issecure;
	char buf[1024], *p;
	Jar *j;

	a = r->fid->aux;
	switch((uintptr)r->fid->file->aux){
	case Xhttp:
		if(a->state == NeedUrl){
			if(r->ifcall.count >= sizeof buf){
				respond(r, "url too long");
				return;
			}
			memmove(buf, r->ifcall.data, r->ifcall.count);
			buf[r->ifcall.count] = '\0';
			issecure = 0;
			if(cistrncmp(buf, "http://", 7) == 0)
				hlen = 7;
			else if(cistrncmp(buf, "https://", 8) == 0){
				hlen = 8;
				issecure = 1;
			}else{
				respond(r, "url must begin http:// or https://");
				return;
			}
			if(buf[hlen]=='/'){
				respond(r, "url without host name");
				return;
			}
			p = strchr(buf+hlen, '/');
			if(p == nil)
				a->path = estrdup9p("/");
			else{
				a->path = estrdup9p(p);
				*p = '\0';
			}
			a->dom = estrdup9p(buf+hlen);
			a->state = HaveUrl;
			j = cookiesearch(jar, a->dom, a->path, issecure);
			if(debug){
				fprint(2, "search %s %s got %p\n", a->dom, a->path, j);
				if(j){
					fprint(2, "%d cookies\n", j->nc);
					for(i=0; i<j->nc; i++)
						fprint(2, "%K\n", &j->c[i]);
				}
			}
			snprint(a->outhttp, AuxBuf, "%J", j);
			if(j)
				closejar(j);
		}else{
			if(strlen(a->inhttp)+r->ifcall.count >= AuxBuf){
				respond(r, "http headers too large");
				return;
			}
			memmove(a->inhttp+strlen(a->inhttp), r->ifcall.data, r->ifcall.count);
		}
		r->ofcall.count = r->ifcall.count;
		respond(r, nil);
		return;

	case Xcookies:
		sz = r->ifcall.count+r->ifcall.offset;
		if(sz > strlen(a->ctext)){
			if(sz >= MaxCtext){
				respond(r, "cookie file too large");
				return;
			}
			a->ctext = erealloc9p(a->ctext, sz+1);
			a->ctext[sz] = '\0';
		}
		memmove(a->ctext+r->ifcall.offset, r->ifcall.data, r->ifcall.count);
		r->ofcall.count = r->ifcall.count;
		respond(r, nil);
		return;

	default:
		respond(r, "bug in webcookies");
		return;
	}
}

void
fsdestroyfid(Fid *fid)
{
	char *p, *nextp;
	Aux *a;
	int i;

	a = fid->aux;
	if(a == nil)
		return;
	switch((uintptr)fid->file->aux){
	case Xhttp:
		parsehttp(jar, a->inhttp, a->dom, a->path);
		break;
	case Xcookies:
		for(i=0; i<jar->nc; i++)
			jar->c[i].mark = 1;
		for(p=a->ctext; *p; p=nextp){
			if((nextp = strchr(p, '\n')) != nil)
				*nextp++ = '\0';
			else
				nextp = "";
			addtojar(jar, p, 0);
		}
		for(i=0; i<jar->nc; i++)
			if(jar->c[i].mark)
				delcookie(jar, &jar->c[i]);
		break;
	}
	syncjar(jar);
	free(a->dom);
	free(a->path);
	free(a->inhttp);
	free(a->outhttp);
	free(a->ctext);
	free(a);
}

void
fsend(Srv*)
{
	closejar(jar);
}

Srv fs = 
{
.open=		fsopen,
.read=		fsread,
.write=		fswrite,
.destroyfid=	fsdestroyfid,
.end=		fsend,
};

void
usage(void)
{
	fprint(2, "usage: webcookies [-f file] [-m mtpt] [-s service]\n");
	exits("usage");
}
	
void
main(int argc, char **argv)
{
	char *file, *mtpt, *home, *srv;

	file = nil;
	srv = nil;
	mtpt = "/mnt/webcookies";
	ARGBEGIN{
	case 'D':
		chatty9p++;
		break;
	case 'd':
		debug = 1;
		break;
	case 'f':
		file = EARGF(usage());
		break;
	case 's':
		srv = EARGF(usage());
		break;
	case 'm':
		mtpt = EARGF(usage());
		break;
	default:
		usage();
	}ARGEND

	if(argc != 0)
		usage();

	quotefmtinstall();
	fmtinstall('J', jarfmt);
	fmtinstall('K', cookiefmt);

	if(file == nil){
		home = getenv("home");
		if(home == nil)
			sysfatal("no cookie file specified and no $home");
		file = emalloc9p(strlen(home)+30);
		strcpy(file, home);
		strcat(file, "/lib/webcookies");
	}
	if(access(file, AEXIST) < 0)
		close(create(file, OWRITE, 0666));

	jar = readjar(file);
	if(jar == nil)
		sysfatal("readjar: %r");

	fs.tree = alloctree("cookie", "cookie", DMDIR|0555, nil);
	closefile(createfile(fs.tree->root, "http", "cookie", 0666, (void*)Xhttp));
	closefile(createfile(fs.tree->root, "cookies", "cookie", 0666, (void*)Xcookies));

	postmountsrv(&fs, srv, mtpt, MREPL);
	exits(nil);
}
