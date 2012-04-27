#include "std.h"
#include "dat.h"

static int
unhex(char c)
{
	if('0' <= c && c <= '9')
		return c-'0';
	if('a' <= c && c <= 'f')
		return c-'a'+10;
	if('A' <= c && c <= 'F')
		return c-'A'+10;
	abort();
	return -1;
}

int
hexparse(char *hex, uchar *dat, int ndat)
{
	int i, n;

	n = strlen(hex);
	if(n%2)
		return -1;
	n /= 2;
	if(n > ndat)
		return -1;
	if(hex[strspn(hex, "0123456789abcdefABCDEF")] != '\0')
		return -1;
	for(i=0; i<n; i++)
		dat[i] = (unhex(hex[2*i])<<4)|unhex(hex[2*i+1]);
	return n;
}

char*
estrappend(char *s, char *fmt, ...)
{
	char *t;
	int l;
	va_list arg;

	va_start(arg, fmt);
	t = vsmprint(fmt, arg);
	if(t == nil)
		sysfatal("out of memory");
	va_end(arg);
	l = s ? strlen(s) : 0;
	s = erealloc(s, l+strlen(t)+1);
	strcpy(s+l, t);
	free(t);
	return s;
}

static char secstore[100];   /* server name */

/* bind in the default network and cs */
static int
bindnetcs(void)
{
	int srvfd;

	if(access("/net/tcp", AEXIST) < 0)
		bind("#I", "/net", MBEFORE);

	if(access("/net/cs", AEXIST) < 0){
		if((srvfd = open("#s/cs", ORDWR)) >= 0){
			if(mount(srvfd, -1, "/net", MBEFORE, "") >= 0)
				return 0;
			close(srvfd);
		}
		return -1;
	}
	return 0;
}

char*
safecpy(char *to, char *from, int n)
{
	memset(to, 0, n);
	if(n == 1)
		return to;
	if(from==nil)
		sysfatal("safecpy called with from==nil, pc=%#p",
			getcallerpc(&to));
	strncpy(to, from, n-1);
	return to;
}

int
secdial(void)
{
	char *p, buf[80], *f[3];
	int fd, nf;

	p = secstore; /* take it from writehostowner, if set there */
	if(*p == 0)	  /* else use the authserver */
		p = "$auth";

	if(bindnetcs() >= 0)
		return dial(netmkaddr(p, "net", "secstore"), 0, 0, 0);

	/* translate $auth ourselves.
	 * authaddr is something like il!host!566 or tcp!host!567.
	 * extract host, accounting for a change of format to something
	 * like il!host or tcp!host or host.
	 */
	if(strcmp(p, "$auth")==0){
		if(authaddr == nil)
			return -1;
		safecpy(buf, authaddr, sizeof buf);
		nf = getfields(buf, f, nelem(f), 0, "!");
		switch(nf){
		default:
			return -1;
		case 1:
			p = f[0];
			break;
		case 2:
		case 3:
			p = f[1];
			break;
		}
	}
	fd = dial(netmkaddr(p, "tcp", "5356"), 0, 0, 0);
	if(fd >= 0)
		return fd;
	return -1;
}

int
_authdial(char *net, char *authdom)
{
	int fd, vanilla;

	vanilla = net==nil || strcmp(net, "/net")==0;

	if(!vanilla || bindnetcs()>=0)
		return authdial(net, authdom);

	/*
	 * If we failed to mount /srv/cs, assume that
	 * we're still bootstrapping the system and dial
	 * the one auth server passed to us on the command line.
	 * In normal operation, it is important *not* to do this,
	 * because the bootstrap auth server is only good for
	 * a single auth domain.
	 *
	 * The ticket request code should really check the
	 * remote authentication domain too.
	 */

	/* use the auth server passed to us as an arg */
	if(authaddr == nil)
		return -1;
	fd = dial(netmkaddr(authaddr, "il", "566"), 0, 0, 0);
	if(fd >= 0)
		return fd;
	return dial(netmkaddr(authaddr, "tcp", "567"), 0, 0, 0);
}

/*
 *  keep caphash fd open since opens of it could be disabled
 */
static int caphashfd;

void
initcap(void)
{
	caphashfd = open("#¤/caphash", OWRITE);
}

/*
 *  create a change uid capability 
 */
char*
mkcap(char *from, char *to)
{
	uchar rand[20];
	char *cap;
	char *key;
	int nfrom, nto;
	uchar hash[SHA1dlen];

	if(caphashfd < 0)
		return nil;

	/* create the capability */
	nto = strlen(to);
	nfrom = strlen(from);
	cap = emalloc(nfrom+1+nto+1+sizeof(rand)*3+1);
	sprint(cap, "%s@%s", from, to);
	memrandom(rand, sizeof(rand));
	key = cap+nfrom+1+nto+1;
	enc64(key, sizeof(rand)*3, rand, sizeof(rand));

	/* hash the capability */
	hmac_sha1((uchar*)cap, strlen(cap), (uchar*)key, strlen(key), hash, nil);

	/* give the kernel the hash */
	key[-1] = '@';
	if(write(caphashfd, hash, SHA1dlen) < 0){
		free(cap);
		return nil;
	}

	return cap;
}

/*
 *  prompt for a string with a possible default response
 */
char*
readcons(char *prompt, char *def, int raw)
{
	int fdin, fdout, ctl, i, n;
	char line[10];
	char *s;

	if(raw){
		ctl = open("/dev/consctl", OWRITE);
		if(ctl >= 0)
			write(ctl, "rawon", 5);
	} else
		ctl = -1;
	fdin = open("/dev/cons", OREAD);
	if(fdin < 0)
		fdin = 0;
	fdout = open("/dev/cons", OWRITE);
	if(fdout < 0)
		fdout = 1;
	if(def != nil)
		fprint(fdout, "%s[%s]: ", prompt, def);
	else
		fprint(fdout, "%s: ", prompt);
	s = estrdup("");
	for(;;){
		n = read(fdin, line, 1);
		if(n == 0){
		Error:
			close(fdin);
			close(fdout);
			if(ctl >= 0)
				close(ctl);
			free(s);
			return nil;
		}
		if(n < 0)
			goto Error;
		if(line[0] == 0x7f)
			goto Error;
		if(n == 0 || line[0] == '\n' || line[0] == '\r'){
			if(raw){
				write(ctl, "rawoff", 6);
				write(fdout, "\n", 1);
			}
			close(ctl);
			close(fdin);
			close(fdout);
			if(*s == 0 && def != nil)
				s = estrappend(s, "%s", def);
			return s;
		}
		if(line[0] == '\b'){
			for(i = strlen(s); i>0 && (s[i]&0xc0) == 0x80; i--)
				s[i] = 0;
		} else if(line[0] == 0x15) {	/* ^U: line kill */
			if(def != nil)
				fprint(fdout, "\n%s[%s]: ", prompt, def);
			else
				fprint(fdout, "\n%s: ", prompt);
			
			s[0] = 0;
		} else {
			s = estrappend(s, "%c", line[0]);
		}
	}
}

int
memrandom(void *p, int n)
{
	uchar *cp;

	for(cp = (uchar*)p; n > 0; n--)
		*cp++ = fastrand();
	return 0;
}

Key*
plan9authkey(Attr *a)
{
	char *dom;
	Key *k;

	/*
	 * The only important part of a is dom.
	 * We don't care, for example, about user name.
	 */
	dom = strfindattr(a, "dom");
	if(dom)
		k = keylookup("proto=p9sk1 role=server user? dom=%q", dom);
	else
		k = keylookup("proto=p9sk1 role=server user? dom?");
	if(k == nil)
		werrstr("could not find plan 9 auth key dom %q", dom);
	return k;
}

Attr*
addcap(Attr *a, char *sysuser, Ticket *t)
{
	Attr *newa;
	char *c;

//	c = mkcap(t->cuid, t->suid);
	c = mkcap(sysuser, t->suid);
	newa = addattr(a, "cuid=%q suid=%q cap=%s", t->cuid, t->suid, c);
	free(c);
	return newa;
}

char*
getnvramkey(int flag, char **secstorepw)
{
	char *s;
	Nvrsafe safe;
	char spw[CONFIGLEN+1];
	int i;

	memset(&safe, 0, sizeof safe);
	/*
	 * readnvram can return -1 meaning nvram wasn't written,
	 * but safe still holds good data.
	 */
	if(readnvram(&safe, flag)<0 && safe.authid[0]==0) 
		return nil;

	/*
	 *  we're using the config area to hold the secstore
	 *  password.  if there's anything there, return it.
	 */
	memmove(spw, safe.config, CONFIGLEN);
	spw[CONFIGLEN] = 0;
	if(spw[0] != 0)
		*secstorepw = estrdup(spw);

	/*
	 *  only use nvram key if it is non-zero
	 */
	for(i = 0; i < DESKEYLEN; i++)
		if(safe.machkey[i] != 0)
			break;
	if(i == DESKEYLEN)
		return nil;

	s = emalloc(512);
	sprint(s, "key proto=p9sk1 user=%q dom=%q !hex=%.*H !password=______", 
		safe.authid, safe.authdom, DESKEYLEN, safe.machkey);
	writehostowner(safe.authid);

	return s;
}

static int
outin(char *prompt, char *def, int len)
{
	char *s;

	s = readcons(prompt, def, 0);
	if(s == nil)
		return -1;
	if(s == nil)
		sysfatal("s==nil???");
	strncpy(def, s, len);
	def[len-1] = 0;
	free(s);
	return strlen(def);
}

/*
 *  get host owner and set it
 */
void
promptforhostowner(void)
{
	char owner[64], *p;

	/* hack for bitsy; can't prompt during boot */
	if(p = getenv("user")){
		writehostowner(p);
		free(p);
		return;
	}
	free(p);

	strcpy(owner, "none");
	do{
		outin("user", owner, sizeof(owner));
	} while(*owner == 0);
	writehostowner(owner);
}

void
writehostowner(char *owner)
{
	int fd;
	char *s;

	if((s = strchr(owner,'@')) != nil){
		*s++ = 0;
		strncpy(secstore, s, (sizeof secstore)-1);
	}
	fd = open("#c/hostowner", OWRITE);
	if(fd >= 0){
		if(fprint(fd, "%s", owner) < 0)
			fprint(2, "factotum: setting #c/hostowner to %q: %r\n",
				owner);
		close(fd);
	}
}

