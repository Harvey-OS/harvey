/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include "common.h"
#include "smtpd.h"
#include <ip.h>

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
	int8_t	*name;
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
	"block",		BLOCKED,
	"deny",			DENIED,
	"dial",			DIALUP,
	"delay",		DELAY,
	0,			NONE,
};

static	int	hisaction;
static	List	ourdoms;
static	List 	badguys;
static	uint32_t	v4peerip;

static	int8_t*	getline(Biobuf*);
static	int	cidrcheck(int8_t*);

static int
findkey(int8_t *val, Keyword *p)
{

	for(; p->name; p++)
		if(strcmp(val, p->name) == 0)
				break;
	return p->code;
}

int8_t*
actstr(int a)
{
	static int8_t buf[32];
	Keyword *p;

	for(p=actions; p->name; p++)
		if(p->code == a)
			return p->name;
	if(a==NONE)
		return "none";
	sprint(buf, "%d", a);
	return buf;
}

int
getaction(int8_t *s, int8_t *type)
{
	int8_t buf[1024];
	Keyword *k;

	if(s == nil || *s == 0)
		return ACCEPT;

	for(k = actions; k->name != 0; k++){
		snprint(buf, sizeof buf, "/mail/ratify/%s/%s/%s", k->name, type, s);
		if(access(buf,0) >= 0)
			return k->code;
	}
	return ACCEPT;
}

int
istrusted(int8_t *s)
{
	int8_t buf[1024];

	if(s == nil || *s == 0)
		return 0;

	snprint(buf, sizeof buf, "/mail/ratify/trusted/%s", s);
	return access(buf,0) >= 0;
}

void
getconf(void)
{
	Biobuf *bp;
	int8_t *cp, *p;
	String *s;
	int8_t buf[512];
	uint8_t addr[4];

	v4parseip(addr, nci->rsys);
	v4peerip = nhgetl(addr);

	trusted = istrusted(nci->rsys);
	hisaction = getaction(nci->rsys, "ip");
	if(debug){
		fprint(2, "istrusted(%s)=%d\n", nci->rsys, trusted);
		fprint(2, "getaction(%s, ip)=%s\n", nci->rsys, actstr(hisaction));
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
usermatch(int8_t *pathuser, int8_t *specuser)
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
dommatch(int8_t *pathdom, int8_t *specdom)
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
 *  figure out action for this sender
 */
int
blocked(String *path)
{
	String *lpath;
	int action;

	if(debug)
		fprint(2, "blocked(%s)\n", s_to_c(path));

	/* if the sender's IP address is blessed, ignore sender email address */
	if(trusted){
		if(debug)
			fprint(2, "\ttrusted => trusted\n");
		return TRUSTED;
	}

	/* if sender's IP address is blocked, ignore sender email address */
	if(hisaction != ACCEPT){
		if(debug)
			fprint(2, "\thisaction=%s => %s\n", actstr(hisaction), actstr(hisaction));
		return hisaction;
	}

	/* convert to lower case */
	lpath = s_copy(s_to_c(path));
	s_tolower(lpath);

	/* classify */
	action = getaction(s_to_c(lpath), "account");
	if(debug)
		fprint(2, "\tgetaction account %s => %s\n", s_to_c(lpath), actstr(action));
	s_free(lpath);
	return action;
}

/*
 * get a canonicalized line: a string of null-terminated lower-case
 * tokens with a two null bytes at the end.
 */
static int8_t*
getline(Biobuf *bp)
{
	int8_t c, *cp, *p, *q;
	int n;

	static int8_t *buf;
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

static int
isourdom(int8_t *s)
{
	Link *l;

	if(strchr(s, '.') == nil)
		return 1;

	for(l = ourdoms.first; l; l = l->next){
		if(dommatch(s, s_to_c(l->p)) == 0)
			return 1;
	}
	return 0;
}

int
forwarding(String *path)
{
	int8_t *cp, *s;
	String *lpath;

	if(debug)
		fprint(2, "forwarding(%s)\n", s_to_c(path));

	/* first check if they want loopback */
	lpath = s_copy(s_to_c(s_restart(path)));
	if(nci->rsys && *nci->rsys){
		cp = s_to_c(lpath);
		if(strncmp(cp, "[]!", 3) == 0){
found:
			s_append(path, "[");
			s_append(path, nci->rsys);
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

	/* if mail is from a trusted IP addr, allow it to forward */
	if(trusted) {
		s_free(lpath);
		return 0;
	}

	/* sender is untrusted; ensure receiver is in one of our domains */
	for(cp = s_to_c(lpath); *cp; cp++)		/* convert receiver lc */
		*cp = tolower(*cp);

	for(s = s_to_c(lpath); cp = strchr(s, '!'); s = cp+1){
		*cp = 0;
		if(!isourdom(s)){
			s_free(lpath);
			return 1;
		}
	}
	s_free(lpath);
	return 0;
}

int
masquerade(String *path, int8_t *him)
{
	int8_t *cp, *s;
	String *lpath;
	int rv = 0;

	if(debug)
		fprint(2, "masquerade(%s) ", s_to_c(path));

	if(trusted || path == nil) {
		if(debug)
			fprint(2, "0\n");
		return 0;
	}

	lpath = s_copy(s_to_c(path));

	/* sender is untrusted; ensure receiver is in one of our domains */
	for(cp = s_to_c(lpath); *cp; cp++)		/* convert receiver lc */
		*cp = tolower(*cp);
	s = s_to_c(lpath);

	/* scan first element of ! or last element of @ paths */
	if((cp = strchr(s, '!')) != nil){
		*cp = 0;
		if(isourdom(s))
			rv = 1;
	} else if((cp = strrchr(s, '@')) != nil){
		if(isourdom(cp+1))
			rv = 1;
	} else {
		if(isourdom(him))
			rv = 1;
	}

	s_free(lpath);
	if (debug)
		fprint(2, "%d\n", rv);
	return rv;
}

/* this is a v4 only check */
static int
cidrcheck(int8_t *cp)
{
	int8_t *p;
	uint32_t a, m;
	uint8_t addr[IPv4addrlen];
	uint8_t mask[IPv4addrlen];

	if(v4peerip == 0)
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
		if((v4peerip&m) == a)
			return 1;
		cp += strlen(cp)+1;
	}		
	return 0;
}

int
isbadguy(void)
{
	Link *l;

	/* check if this IP address is banned */
	for(l = badguys.first; l; l = l->next)
		if(cidrcheck(s_to_c(l->p)))
			return 1;

	return 0;
}

void
addbadguy(int8_t *p)
{
	listadd(&badguys, s_copy(p));
};

int8_t*
dumpfile(int8_t *sender)
{
	int i, fd;
	uint32_t h;
	static int8_t buf[512];
	int8_t *cp;

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

int8_t *validator = "/mail/lib/validateaddress";

int
recipok(int8_t *user)
{
	int8_t *cp, *p, c;
	int8_t buf[512];
	int n;
	Biobuf *bp;
	int pid;
	Waitmsg *w;

	if(shellchars(user)){
		syslog(0, "smtpd", "shellchars in user name");
		return 0;
	}

	if(access(validator, AEXEC) == 0)
	switch(pid = fork()) {
	case -1:
		break;
	case 0:
		execl(validator, "validateaddress", user, nil);
		exits(0);
	default:
		while(w = wait()) {
			if(w->pid != pid)
				continue;
			if(w->msg[0] != 0){
				/*
				syslog(0, "smtpd", "validateaddress %s: %s", user, w->msg);
				*/
				return 0;
			}
			break;
		}
	}

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
				syslog(0, "smtpd", "names.blocked blocks %s", user);
				Bterm(bp);
				return 0;
			}
		}
	}
	Bterm(bp);
	return 1;
}

/*
 *  a user can opt out of spam filtering by creating
 *  a file in his mail directory named 'nospamfiltering'.
 */
int
optoutofspamfilter(int8_t *addr)
{
	int8_t *p, *f;
	int rv;

	p = strchr(addr, '!');
	if(p)
		p++;
	else
		p = addr;


	rv = 0;
	f = smprint("/mail/box/%s/nospamfiltering", p);
	if(f != nil){
		rv = access(f, 0)==0;
		free(f);
	}

	return rv;
}
