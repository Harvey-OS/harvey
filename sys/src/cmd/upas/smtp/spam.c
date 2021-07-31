#include "common.h"
#include "smtpd.h"
#include "ip.h"

enum {
	NORELAY = 0,
	DNSVERIFY,
	SAVEBLOCK,
	DOMNAME,
	OURNETS,
	OURDOMS,

	IP = 0,
	STRING,
};


typedef struct Keyword Keyword;

struct Keyword {
	char	*name;
	int	code;
};

static Keyword options[] = {
	"norelay",		NORELAY,
	"verifysenderdom",	DNSVERIFY,
	"saveblockedmsg",	SAVEBLOCK,
	"defaultdomain",	DOMNAME,	
	"ournets",		OURNETS,
	"ourdomains",		OURDOMS,
	0,			NONE,
};

static Keyword actions[] = {
	"allow",		ACCEPT,
	"accept",		ACCEPT,
	"block",		BLOCKED,
	"deny",			DENIED,
	"dial",			DIALUP,
	"relay",		DELAY,
	"delay",		DELAY,
	0,			NONE,
};

static	char*	getline(Biobuf*);

static int
findkey(char *val, Keyword *p)
{

	for(; p->name; p++)
		if(strcmp(val, p->name) == 0)
				break;
	return p->code;
}

void
getconf(void)
{
	Biobuf *bp;
	char *cp, *p;
	String *s;
	char buf[512];
/*
**		let it fail on unix
*/
	if(hisaddr && *hisaddr){
		sprint(buf, "/mail/ratify/trusted/%s#32", hisaddr);
		if(access(buf,0) >= 0)
			trusted++;
	}


	snprint(buf, sizeof(buf), "%s/smtpd.conf", UPASLIB);
	bp = sysopen(buf, "r", 0);
	if(bp == 0)
		return;

	for(;;){
		cp = getline(bp);
		if(cp == 0)
			break;
		p = cp+strlen(cp)+1;
		switch(findkey(cp, options)){
		case NORELAY:
			if(fflag == 0 && strcmp(p, "on") == 0)
				fflag++;
			break;
		case DNSVERIFY:
			if(rflag == 0 && strcmp(p, "on") == 0)
				rflag++;
			break;
		case SAVEBLOCK:
			if(sflag == 0 && strcmp(p, "on") == 0)
				sflag++;
			break;
		case DOMNAME:
			if(dom == 0)
				dom = strdup(p);
			break;
		case OURNETS:
			if (trusted == 0)
				trusted = cidrcheck(p);
			break;
		case OURDOMS:
			while(*p){
				s = s_new();
				s_append(s, p);
				listadd(&ourdoms, s);
				p += strlen(p)+1;
			}
			break;
		default:
			break;
		}
	}
	sysclose(bp);
}

/*
 *	match a user name.  the only meta-char is '*' which matches all
 *	characters.  we only allow it as "*", which matches anything or
 *	an * at the end of the name (e.g., "username*") which matches
 *	trailing characters.
 */
static int
usermatch(char *pathuser, char *specuser)
{
	int n;

	n = strlen(specuser)-1;
	if(specuser[n] == '*'){
		if(n == 0)		/* match everything */
			return 0;
		return strncmp(pathuser, specuser, n);
	}
	return strcmp(pathuser, specuser);
}

static int
dommatch(char *pathdom, char *specdom)
{
	int n;

	if (*specdom == '*'){
		if (specdom[1] == '.' && specdom[2]){
			specdom += 2;
			n = strlen(pathdom)-strlen(specdom);
			if(n == 0 || (n > 0 && pathdom[n-1] == '.'))
				return strcmp(pathdom+n, specdom);
			return n;
		}
	}
	return strcmp(pathdom, specdom);
}

/*
 * match path components to prohibited domain & user specifications.  patterns include:
 *	domain, domain! or domain!*	  - all users in domain
 *	*.domain, *.domain! or *.domain!* - all users in domain and its subdomains
 *	!user or *!user			  - user in all domains
 *	domain!user			  - user in domain
 *	*.domain!user			  - user in domain and its subdomains
 *
 *	if "user" has a trailing '*', it matches all user names beginning with "user"
 *
 * there are special semantics for the "domain, domain! or domain!*" specifications:
 * the first two forms match when the domain is anywhere in at list of source-routed
 * domains while the latter matches only when the domain is the last hop.  the same is
 * true for the *.domain!* form of the pattern.
 */
static int
accountmatch(char *spec, List *doms, char *user)
{
	char *cp, *p;
	int i, n;
	Link *l;

	static char *dangerous[] = { "*", "!", "*!", "!*", "*!*", 0 };

	for(; *spec; spec += n){
		n = strlen(spec)+1;

		/* rule out dangerous patterns */
		for (i = 0; dangerous[i]; i++)
			if(strcmp(spec, dangerous[i])== 0)
				break;
		if(dangerous[i])
			continue;

		p = 0;
		cp = strchr(spec, '!');
		if(cp){
			*cp++ = 0;
			if(*cp)
			if(strcmp(cp, "*"))	/* rule out "!*" */
				p = cp;
		}

		if(p == 0){			/* no user field - domain match only */
			for(l = doms->first; l; l = l->next)
				if(dommatch(s_to_c(l->p), spec) == 0)
					return 1;
		} else {
			/* check for "!user", "*!user" or "domain!user" */
			if(usermatch(user, p) == 0){
				if(*spec == 0 || strcmp(spec, "*") == 0)
					return 1;
				if(doms->last && dommatch(s_to_c(doms->last->p), spec) == 0)
					return 1;
			}
		}
	}
	return 0;
}

/*
 * we risk reparsing the file of blocked addresses if there are
 * multiple senders or if the peer does a transaction and then a RSET
 * followed by another transaction.  we believe this happens rarely,
 * but we cache the last sender to try to minimize the overhead.  we
 * also cache whether we rejected a previous transaction based on
 * ip address, since this never changes.
 */
int
blocked(String *path)
{
	char buf[512], *cp, *p, *user;
	Biobuf *bp;
	int action, type;
	List doms;
	String *s, *lpath;

	static String *lastsender;
	static int lastret, blockedbyip;

	if(debug)
		fprint(2, "blocked(%s)\n", s_to_c(path));

	if(blockedbyip)
		return lastret;

	if(lastsender){
		if(strcmp(s_to_c(lastsender), s_to_c(path)) == 0)
			return lastret;
		s_free(lastsender);
		lastsender = 0;
	}

	snprint(buf, sizeof(buf), "%s/blocked", UPASLIB);
	bp = sysopen(buf, "r", 0);
	if(bp == 0)
		return ACCEPT;

	lpath = s_copy(s_to_c(s_restart(path)));	/* convert to LC */
	for(cp = s_to_c(lpath); *cp; cp++)
		*cp = tolower(*cp);

	/* parse the path into a list of domains and a user */
	doms.first = doms.last = 0;
	p = s_to_c(lpath);
	while (cp = strchr(p, '!')){
		*cp = 0;
		s = s_new();
		s_append(s, p);
		*cp = '!';
		listadd(&doms, s);
		p = cp+1;
	}
	user = p;

	/*
	 * line format: [*]VERB param 1, ... param n,  where '*' indicates
	 * params are path names; if not present, params are ip addresses in
	 * CIDR format.
	 */
	for(;;){
		action = ACCEPT;
		cp = getline(bp);
		if(cp == 0)
			break;
		type = *cp;
		if(type == '*') {
			cp++;
			if(*cp == 0)
				cp++;
		}
		action = findkey(cp, actions);
		if (action == NONE)
			continue;

		cp += strlen(cp)+1;
		if(type == '*' && accountmatch(cp, &doms, user))
			break;
		else if(type != '*'&& cidrcheck(cp)) {
			blockedbyip = 1;
			break;
		}
	}
	sysclose(bp);
	listfree(&doms);
	lastsender = s_copy(s_to_c(s_restart(path)));
	s_free(lpath);
	lastret = action;
	return action;
}

/*
 * get a canonicalized line: a string of null-terminated lower-case
 * tokens with a two null bytes at the end.
 */
static char*
getline(Biobuf *bp)
{
	char c, *cp, *p, *q;
	int n;

	static char *buf;
	static int bufsize;

	for(;;){
		cp = Brdline(bp, '\n');
		if(cp == 0)
			return 0;
		n = Blinelen(bp);
		cp[n-1] = 0;
		if(buf == 0 || bufsize < n+1){
			bufsize += 512;
			if(bufsize < n+1)
				bufsize = n+1;
			buf = realloc(buf, bufsize);
			if(buf == 0)
				break;
		}
		q = buf;
		for (p = cp; *p; p++){
			c = *p;
			if(c == '\\' && p[1])	/* we don't allow \<newline> */
				c = *++p;
			else
			if(c == '#')
				break;
			else
			if(c == ' ' || c == '\t' || c == ',')
				if(q == buf || q[-1] == 0)
					continue;
				else
					c = 0;
			*q++ = tolower(c);
		}
		if(q != buf){
			if(q[-1])
				*q++ = 0;
			*q = 0;
			break;
		}
	}
	return buf;
}

int
forwarding(String *path)
{
	char *cp, *s;
	String *lpath;
	Link *l;

	if(debug)
		fprint(2, "forwarding(%s)\n", s_to_c(path));

		/* first check if they want loopback */

	lpath = s_copy(s_to_c(s_restart(path)));
	if(hisaddr && *hisaddr){
		cp = s_to_c(lpath);
		if(strncmp(cp, "[]!", 3) == 0){
found:
			s_append(path, "[");
			s_append(path, hisaddr);
			s_append(path, "]!");
			s_append(path, cp+3);
			s_terminate(path);
			s_free(lpath);
			return 0;
		}
		cp = strchr(cp,'!');			/* skip our domain and check next */
		if(cp++ && strncmp(cp, "[]!", 3) == 0)
			goto found;
	}
		/* if mail is from a trusted subnet, allow it to forward*/
	if(trusted) {
		s_free(lpath);
		return 0;
	}

		/* sender is untrusted; ensure receiver is in one of our domains */
	for(cp = s_to_c(lpath); *cp; cp++)		/* convert receiver lc */
		*cp = tolower(*cp);

	for(s = s_to_c(lpath); cp = strchr(s, '!'); s = cp+1){
		*cp = 0;
		if(strchr(s, '.')){
			for(l = ourdoms.first; l; l = l->next){
				if(dommatch(s, s_to_c(l->p)) == 0)
					break;
			}
			if(l == 0){
				*cp = '!';
				s_free(lpath);
				return 1;
			}
		}
		*cp = '!';
	}
	s_free(lpath);
	return 0;
}

int
cidrcheck(char *cp)
{
	char *p;
	ulong a, m;
	uchar addr[IPv4addrlen];
	uchar mask[IPv4addrlen];

	if(peerip == 0)
		return 0;

		/* parse a list of CIDR addresses comparing each to the peer IP addr */
	while(cp && *cp){
		v4parsecidr(addr, mask, cp);
		a = nhgetl(addr);
		m = nhgetl(mask);
		/*
		 * if a mask isn't specified, we build a minimal mask
		 * instead of using the default mask for that net.  in this
		 * case we never allow a class A mask (0xff000000).
		 */
		if(strchr(cp, '/') == 0){
			m = 0xff000000;
			p = cp;
			for(p = strchr(p, '.'); p && p[1]; p = strchr(p+1, '.'))
					m = (m>>8)|0xff000000;

			/* force at least a class B */
			m |= 0xffff0000;
		}
		if((peerip&m) == a)
			return 1;
		cp += strlen(cp)+1;
	}		
	return 0;
}

char*
dumpfile(char *sender)
{
	int i, fd;
	ulong h;
	static char buf[512];
	char *cp;

	if (sflag == 1){
		cp = ctime(time(0));
		cp[7] = 0;
		if(cp[8] == ' ')
			sprint(buf, "%s/queue.dump/%s%c", SPOOL, cp+4, cp[9]);
		else
			sprint(buf, "%s/queue.dump/%s%c%c", SPOOL, cp+4, cp[8], cp[9]);
		cp = buf+strlen(buf);
		if(access(buf, 0) < 0 && sysmkdir(buf, 0777) < 0)
			return "/dev/null";
		h = 0;
		while(*sender)
			h = h*257 + *sender++;
		for(i = 0; i < 50; i++){
			h += lrand();
			sprint(cp, "/%lud", h);
			if(access(buf, 0) >= 0)
				continue;
			fd = syscreate(buf, ORDWR, 0666);
			if(fd >= 0){
				if(debug)
					fprint(2, "saving in %s\n", buf);
				close(fd);
				return buf;
			}
		}
	}
	return "/dev/null";
}

int
recipok(char *user)
{
	char *cp, *p, c;
	char buf[512];
	int n;
	Biobuf *bp;

	snprint(buf, sizeof(buf), "%s/names.blocked", UPASLIB);
	bp = sysopen(buf, "r", 0);
	if(bp == 0)
		return 1;
	for(;;){
		cp = Brdline(bp, '\n');
		if(cp == 0)
			break;
		n = Blinelen(bp);
		cp[n-1] = 0;

		while(*cp == ' ' || *cp == '\t')
			cp++;
		for(p = cp; c = *p; p++){
			if(c == '#')
				break;
			if(c == ' ' || c == '\t')
				break;
		}
		if(p > cp){
			*p = 0;
			if(cistrcmp(user, cp) == 0){
				Bterm(bp);
				return 0;
			}
		}
	}
	Bterm(bp);
	return 1;
}
