#include "ratfs.h"
#include <ip.h>

enum {
	Maxdoms	=	10,		/* max domains in a path */
	Timeout =	2*60*60,	/* seconds until temporarily trusted addr times out */
};

static	int	accountmatch(char*, char**, int, char*);
static	Node*	acctwalk(char*,  Node*);
static	int	dommatch(char*, char*);
static	Address* ipsearch(ulong, Address*, int);
static	Node*	ipwalk(char*,  Node*);
static	Node*	trwalk(char*, Node*);
static	int	usermatch(char*, char*);

/*
 *	Do a walk
 */
char*
walk(char *name, Fid *fidp)
{
	Node *np;

	if((fidp->node->d.mode & DMDIR) == 0)
		return "not a directory";

	if(strcmp(name, ".") == 0)
		return 0;
	if(strcmp(name, "..") == 0){
		fidp->node = fidp->node->parent;
		fidp->name = 0;
		return 0;
	}

	switch(fidp->node->d.type){
	case Directory:
	case Addrdir:
		np = dirwalk(name, fidp->node);
		break;
	case Trusted:
		np = trwalk(name, fidp->node);
		break;
	case IPaddr:
		np = ipwalk(name, fidp->node);
		break;
	case Acctaddr:
		np = acctwalk(name, fidp->node);
		break;
	default:
		return "directory botch in walk";
	}
	if(np) {
		fidp->node = np;
		fidp->name = np->d.name;
		return 0;
	}
	return "file does not exist";
}

/*
 *	Walk to a subdirectory
 */
Node*
dirwalk(char *name, Node *np)
{
	Node *p;

	for(p = np->children; p; p = p->sibs)
		if(strcmp(name, p->d.name) == 0)
			break;
	return p;
}

/*
 *	Walk the directory of trusted files
 */
static Node*
trwalk(char *name, Node *np)
{
	Node *p;
	ulong peerip;
	uchar addr[IPv4addrlen];

	v4parseip(addr, name);
	peerip = nhgetl(addr);

	for(p = np->children; p; p = p->sibs)
		if((peerip&p->ip.mask) == p->ip.ipaddr)
			break;
	return p;
}

/*
 *	Walk a directory of IP addresses
 */
static Node*
ipwalk(char *name,  Node *np)
{
	Address *ap;
	ulong peerip;
	uchar addr[IPv4addrlen];

	v4parseip(addr, name);
	peerip = nhgetl(addr);

	if(debugfd >= 0)
		fprint(debugfd, "%d.%d.%d.%d - ", addr[0]&0xff, addr[1]&0xff,
				addr[2]&0xff, addr[3]&0xff);
	ap = ipsearch(peerip, np->addrs, np->count);
	if(ap == 0)
		return 0;

	dummy.d.name = ap->name;
	return &dummy;
}

/*
 *	Walk a directory of account names
 */
static Node*
acctwalk(char *name, Node *np)
{
	int i, n;
	Address *ap;
	char *p, *cp, *user;
	char buf[512];
	char *doms[Maxdoms];

	strecpy(buf, buf+sizeof buf, name);
	subslash(buf);

	p = buf;
	for(n = 0; n < Maxdoms; n++) {
		cp = strchr(p, '!');
		if(cp == 0)
			break;
		*cp = 0;
		doms[n] = p;
		p = cp+1;
	}
	user = p;

	for(i = 0; i < np->count; i++){
		ap = &np->addrs[i];
		if (accountmatch(ap->name, doms, n, user)) {
			dummy.d.name = ap->name;
			return &dummy;
		}
	}
	return 0;
}

/*
 * binary search sorted IP address list
 */

static Address*
ipsearch(ulong addr, Address *base, int n)
{
	ulong top, bot, mid;
	Address *ap;

	bot = 0;
	top = n;
	for (mid = (bot+top)/2; mid < top; mid = (bot+top)/2) {
		ap = &base[mid];
		if((addr&ap->ip.mask) == ap->ip.ipaddr)
			return ap;
		if(addr < ap->ip.ipaddr)
			top = mid;
		else if(mid != n-1 && addr >= base[mid+1].ip.ipaddr)
			bot = mid;
		else
			break;
	}
	return 0;
}

/*
 *	Read a directory
 */
int
dread(Fid *fidp, int cnt)
{
	uchar *q, *eq, *oq;
	int n, skip;
	Node *np;

	if(debugfd >= 0)
		fprint(debugfd, "dread %d\n", cnt);

	np = fidp->node;
	oq = q = rbuf+IOHDRSZ;
	eq = q+cnt;
	if(fidp->dirindex >= np->count)
		return 0;

	skip = fidp->dirindex;
	for(np = np->children; skip > 0 && np; np = np->sibs)
		skip--;
	if(np == 0)
		return 0;

	for(; q < eq && np; np = np->sibs){
		if(debugfd >= 0)
			printnode(np);
		if((n=convD2M(&np->d, q, eq-q)) <= BIT16SZ)
			break;
		q += n;
		fidp->dirindex++;
	}
	return q - oq;
}

/*
 *	Read a directory of IP addresses or account names
 */
int
hread(Fid *fidp, int cnt)
{
	uchar *q, *eq, *oq;
	int i, n, path;
	Address *p;
	Node *np;

	if(debugfd >= 0)
		fprint(debugfd, "hread %d\n", cnt);

	np = fidp->node;
	oq = q = rbuf+IOHDRSZ;
	eq = q+cnt;
	if(fidp->dirindex >= np->count)
		return 0;

	path = np->baseqid;
	for(i = fidp->dirindex; q < eq && i < np->count; i++){
		p = &np->addrs[i];
		dummy.d.name = p->name;
		dummy.d.qid.path = path++;
		if((n=convD2M(&dummy.d, q, eq-q)) <= BIT16SZ)
			break;
		q += n;
	}
	fidp->dirindex = i;
	return q - oq;
}

/*
 *	Find a directory node by type
 */
Node*
finddir(int type)
{
	Node *np;

	for(np = root->children; np; np = np->sibs)
		if (np->d.type == type)
			return np;
	return 0;
}

/*
 *	Remove temporary pseudo-files that have timed-out
 *	from the trusted directory
 */
void
cleantrusted(void)
{
	Node *np, **l;
	ulong t;

	np = finddir(Trusted);
	if (np == 0)
		return;

	t = time(0)-Timeout;
	l = &np->children;
	for (np = np->children; np; np = *l) {
		if(np->d.type == Trustedtemp && t >= np->d.mtime) {
			*l = np->sibs;
			if(debugfd >= 0)
				fprint(debugfd, "Deleting %s\n", np->d.name);
			np->parent->count--;
			free(np);
		} else
			l = &np->sibs;
	}
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
accountmatch(char *spec, char **doms, int ndoms, char *user)
{
	char *cp, *userp;
	int i, ret;

	userp = 0;
	ret = 0;
	cp = strchr(spec, '!');
	if(cp){
		*cp++ = 0;		/* restored below */
		if(*cp)
		if(strcmp(cp, "*"))	/* "!*" is the same as no user field */
			userp = cp;	/* there is a user name */
	}

	if(userp == 0){			/* no user field - domain match only */
		for(i = 0; i < ndoms && doms[i]; i++)
			if(dommatch(doms[i], spec) == 0)
				ret = 1;
	} else {
		/* check for "!user", "*!user" or "domain!user" */
		if(usermatch(user, userp) == 0){
			if(*spec == 0 || strcmp(spec, "*") == 0)
				ret = 1;
			else if(ndoms > 0  && dommatch(doms[ndoms-1], spec) == 0)
				ret = 1;
		}
	}
	if(cp)
		cp[-1] = '!';
	return ret;
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

/*
 *	Match a domain specification
 */
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
 *	Custom allocators to avoid malloc overheads on small objects.
 * 	We never free these.  (See below.)
 */
typedef struct Stringtab	Stringtab;
struct Stringtab {
	Stringtab *link;
	char *str;
};
static Stringtab*
taballoc(void)
{
	static Stringtab *t;
	static uint nt;

	if(nt == 0){
		t = malloc(64*sizeof(Stringtab));
		if(t == 0)
			fatal("out of memory");
		nt = 64;
	}
	nt--;
	return t++;
}

static char*
xstrdup(char *s)
{
	char *r;
	int len;
	static char *t;
	static int nt;

	len = strlen(s)+1;
	if(len >= 8192)
		fatal("strdup big string");

	if(nt < len){
		t = malloc(8192);
		if(t == 0)
			fatal("out of memory");
		nt = 8192;
	}
	r = t;
	t += len;
	nt -= len;
	strcpy(r, s);
	return r;
}

/*
 *	Return a uniquely allocated copy of a string.
 *	Don't free these -- they stay in the table for the 
 *	next caller who wants that particular string.
 *	String comparison can be done with pointer comparison 
 *	if you know both strings are atoms.
 */
static Stringtab *stab[1024];

static uint
hash(char *s)
{
	uint h;
	uchar *p;

	h = 0;
	for(p=(uchar*)s; *p; p++)
		h = h*37 + *p;
	return h;
}

char*
atom(char *str)
{
	uint h;
	Stringtab *tab;
	
	h = hash(str) % nelem(stab);
	for(tab=stab[h]; tab; tab=tab->link)
		if(strcmp(str, tab->str) == 0)
			return tab->str;

	tab = taballoc();
	tab->str = xstrdup(str);
	tab->link = stab[h];
	stab[h] = tab;
	return tab->str;
}
