#include "ratfs.h"
#include <ip.h>

enum {
	ACCEPT = 0,		/* verbs in control file */
	REFUSED,
	DENIED,
	DIALUP,
	BLOCKED,
	DELAY,
	NONE,

	Subchar	=	'#',	/* character substituted for '/' in file names */
};

static	Keyword actions[] = {
	"allow",		ACCEPT,
	"accept",		ACCEPT,
	"block",		BLOCKED,
	"deny",			DENIED,
	"dial",			DIALUP,
	"relay",		DELAY,
	"delay",		DELAY,
	0,			NONE,
};

static void	acctinsert(Node*, char*);
static char*	getline(Biobuf*);
static void	ipinsert(Node*, char*);
static void	ipsort(void);

/*
 *	Input the configuration file
 *	Currently we only process the "ournets"
 *	specification.
 */
void
getconf(void)
{
	Biobuf *bp;
	char *cp;
	Node *np, *dir, **l;

	if(debugfd >= 0)
		fprint(debugfd, "loading %s\n", conffile);

	bp = Bopen(conffile, OREAD);
	if(bp == 0)
		return;

	dir = finddir(Trusted);
	if(dir == 0)
		return;

	/*
	 * if this isn't the first time, purge permanent entries
	 */
	trustedqid = Qtrustedfile;
	if(lastconftime){
		l = &dir->children;
		for(np = dir->children; np; np = *l){
			if(np->d.type == Trustedperm){
				*l = np->sibs;
				free(np);
			} else {
				np->d.qid.path = trustedqid++;
				l = &np->sibs;
			}
		}
		dir->count = 0;
	}

	for(;;){
		cp = getline(bp);
		if(cp == 0)
			break;
		if (strcmp(cp, "ournets") == 0){
			for(cp += strlen(cp)+1; cp && *cp; cp += strlen(cp)+1){
				np = newnode(dir, cp, Trustedperm, 0111, trustedqid++);
				cidrparse(&np->ip, cp);
				subslash(cp);
				np->d.name = atom(cp);
			}
		}
	}
	Bterm(bp);
	lastconftime = time(0);
}

/*
 *	Reload the control file, if necessary
 */
void
reload(void)
{
	int type, action;
	Biobuf *bp;
	char *cp;
	Node *np, *dir;

	if(debugfd >= 0)
		fprint(debugfd,"loading %s\n", ctlfile);

	bp = Bopen(ctlfile, OREAD);
	if(bp == 0)
		return;
	
	if(lastctltime){
		for(dir = root->children; dir; dir = dir->sibs){
			if (dir->d.type != Addrdir)
				continue;
			for(np = dir->children; np; np = np->sibs)
				np->count = 0;
		}
	}

	for(;;){
		cp = getline(bp);
		if(cp == 0)
			break;
		type = *cp;
		if(type == '*'){
			cp++;
			if(*cp == 0)		/* space before keyword */
				cp++;
		}
		action = findkey(cp, actions);
		if (action == NONE)
			continue;
		if (action == ACCEPT)
			dir = dirwalk("allow", root);
		else
		if (action == DELAY)
			dir = dirwalk("delay", root);
		else
			dir = dirwalk(cp, root);
		if(dir == 0)
			continue;
		
		for(cp += strlen(cp)+1; cp && *cp; cp += strlen(cp)+1){
			if(type == '*')
				acctinsert(dir, cp);
			else
				ipinsert(dir, cp);
		}
	}
	Bterm(bp);
	ipsort();
	dummy.d.mtime = dummy.d.atime = lastctltime = time(0);
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

/*
 *	Match a keyword
 */
int
findkey(char *val, Keyword *p)
{

	for(; p->name; p++)
		if(strcmp(val, p->name) == 0)
				break;
	return p->code;
}

/*
 *	parse a cidr specification in either IP/mask or IP#mask format
 */
void
cidrparse(Cidraddr *cidr, char *cp)
{

	char *p, *slash;
	int c;
	ulong a, m;
	uchar addr[IPv4addrlen];
	uchar mask[IPv4addrlen];
	char buf[64];

	/*
	 * find '/' or '#' character in the cidr specification
	 */
	slash = 0;
	for(p = buf; p < buf+sizeof(buf)-1 && *cp; p++) {
		c = *cp++;
		switch(c) {
		case Subchar:
			c = '/';
			slash = p;
			break;
		case '/':
			slash = p;
			break;
		default:
			break;
		}
		*p = c;
	}
	*p = 0;

	v4parsecidr(addr, mask, buf);
	a = nhgetl(addr);
	m = nhgetl(mask);
	/*
	 * if a mask isn't specified, we build a minimal mask
	 * instead of using the default mask for that net.  in this
	 * case we never allow a class A mask (0xff000000).
	 */
	if(slash == 0){
		m = 0xff000000;
		p = buf;
		for(p = strchr(p, '.'); p && p[1]; p = strchr(p+1, '.'))
				m = (m>>8)|0xff000000;

		/* force at least a class B */
		m |= 0xffff0000;
	}
	cidr->ipaddr = a;
	cidr->mask = m;
}

/*
 *	Substitute Subchar ('#') for '/'
 */
char*
subslash(char *os)
{
	char *s;

	for(s=os; *s; s++)
		if(*s == '/')
			*s = Subchar;
	return os;
}

/*
 *	Insert an account pseudo-file in a directory
 */
static void
acctinsert(Node *np, char *cp)
{
	int i;
	char *tmp;
	Address *ap;

	static char *dangerous[] = { "*", "!", "*!", "!*", "*!*", 0 };

	if(cp == 0 || *cp == 0)
		return;

	/* rule out dangerous patterns */
	for (i = 0; dangerous[i]; i++)
		if(strcmp(cp, dangerous[i])== 0)
			return;

	np = dirwalk("account", np);
	if(np == 0)
		return;

	i = np->count++;
	if(i >= np->allocated){
		np->allocated = np->count;
		np->addrs = realloc(np->addrs, np->allocated*sizeof(Address));
		if(np->addrs == 0)
			fatal("out of memory");
	}

	ap = &np->addrs[i];			/* new entry on end */
	tmp = strdup(cp);
	if(tmp == nil)
		fatal("out of memory");
	subslash(tmp);
	ap->name = atom(tmp);
	free(tmp);
}

/*
 *	Insert an IP address pseudo-file in a directory
 */
static void
ipinsert(Node *np, char *cp)
{
	char *tmp;
	int i;
	Address *ap;
	if(cp == 0 || *cp == 0)
		return;

	np = dirwalk("ip", np);
	if(np == 0)
		return;

	i = np->count++;
	if(i >= np->allocated){
		np->allocated = np->count;
		np->addrs = realloc(np->addrs, np->allocated*sizeof(Address));
		if(np->addrs == 0)
			fatal("out of memory");
	}

	ap = &np->addrs[i];				/* new entry on end */
	tmp = strdup(cp);
	if(tmp == nil)
		fatal("out of memory");
	subslash(tmp);
	ap->name = atom(tmp);
	free(tmp);
	cidrparse(&ap->ip, cp);
}

int
ipcomp(void *a, void *b)
{
	ulong aip, bip;

	aip = ((Address*)a)->ip.ipaddr;
	bip = ((Address*)b)->ip.ipaddr;
	if(aip > bip)
		return 1;
	if(aip < bip)
		return -1;
	return 0;
}

/*
 *	Sort a directory of IP addresses
 */
static void
ipsort(void)
{
	int base;
	Node *dir, *np;

	base = Qaddrfile;
	for(dir = root->children; dir; dir = dir->sibs){
		if (dir->d.type != Addrdir)
			continue;
		for(np = dir->children; np; np = np->sibs){
			if(np->d.type == IPaddr && np->count && np->addrs)
				qsort(np->addrs, np->count, sizeof(Address), ipcomp);
			np->baseqid = base;
			base += np->count;
		}
	}
}
