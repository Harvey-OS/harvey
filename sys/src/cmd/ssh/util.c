#include "ssh.h"
#include <bio.h>
#include <ndb.h>

char Edecode[] = "error decoding input packet";
char Eencode[] = "out of space encoding output packet (BUG)";
char Ehangup[] = "hungup connection";
char Ememory[] = "out of memory";

int debuglevel;
int doabort;

void
error(char *fmt, ...)
{
	va_list arg;
	char buf[2048];

	va_start(arg, fmt);
	vseprint(buf, buf+sizeof(buf), fmt, arg);
	va_end(arg);
	fprint(2, "%s: %s\n", argv0, buf);
	if(doabort)
		abort();
	exits(buf);
}

void
debug(int level, char *fmt, ...)
{
	va_list arg;

	if((level&debuglevel) == 0)
		return;
	va_start(arg, fmt);
	vfprint(2, fmt, arg);
	va_end(arg);
}

void*
emalloc(long n)
{
	void *a;

	a = mallocz(n, 1);
	if(a == nil)
		error(Ememory);
	setmalloctag(a, getcallerpc(&n));
	return a;
}

void*
erealloc(void *v, long n)
{
	v = realloc(v, n);
	if(v == nil)
		error(Ememory);
	setrealloctag(v, getcallerpc(&v));
	return v;
}


static int killpid[32];
static int nkillpid;
void
atexitkiller(void)
{
	int i, pid;

	pid = getpid();
	debug(DBG, "atexitkiller: nkillpid=%d mypid=%d\n", nkillpid, pid);
	for(i=0; i<nkillpid; i++)
		if(pid != killpid[i]){
			debug(DBG, "killing %d\n", killpid[i]);
			postnote(PNPROC, killpid[i], "kill");
		}
}
void
atexitkill(int pid)
{
	killpid[nkillpid++] = pid;
}

int
readstrnl(int fd, char *buf, int nbuf)
{
	int i;

	for(i=0; i<nbuf; i++){
		switch(read(fd, buf+i, 1)){
		case -1:
			return -1;
		case 0:
			werrstr("unexpected EOF");
			return -1;
		default:
			if(buf[i]=='\n'){
				buf[i] = '\0';
				return 0;
			}
			break;
		}
	}
	werrstr("line too long");
	return -1;
}

void
calcsessid(Conn *c)
{
	int n;
	uchar buf[1024];

	n = mptobe(c->hostkey->n, buf, sizeof buf, nil);
	n += mptobe(c->serverkey->n, buf+n, sizeof buf-n, nil);
	memmove(buf+n, c->cookie, COOKIELEN);
	n += COOKIELEN;
	md5(buf, n, c->sessid, nil);
}

void
sshlog(char *f, ...)
{
	char *s;
	va_list arg;
	Fmt fmt;
	static int pid;

	if(pid == 0)
		pid = getpid();

	va_start(arg, f);
	va_end(arg);

	if(fmtstrinit(&fmt) < 0)
		sysfatal("fmtstrinit: %r");

	fmtprint(&fmt, "[%d] ", pid);
	fmtvprint(&fmt, f, arg);

	s = fmtstrflush(&fmt);
	if(s == nil)
		sysfatal("fmtstrflush: %r");
	syslog(0, "ssh", "%s", s);
	free(s);
}

/*
 * this is far too smart.
 */
static int
pstrcmp(const void *a, const void *b)
{
	return strcmp(*(char**)a, *(char**)b);
}

static char*
trim(char *s)
{
	char *t;
	int i, last, n, nf;
	char **f;
	char *p;

	t = emalloc(strlen(s)+1);
	t[0] = '\0';
	n = 1;
	for(p=s; *p; p++)
		if(*p == ' ')
			n++;
	f = emalloc((n+1)*sizeof(f[0]));
	nf = tokenize(s, f, n+1);
	qsort(f, nf, sizeof(f[0]), pstrcmp);
	last=-1;
	for(i=0; i<nf; i++){
		if(last==-1 || strcmp(f[last], f[i])!=0){
			if(last >= 0)
				strcat(t, ",");
			strcat(t, f[i]);
			last = i;
		}
	}
	return t;	
}

static void
usetuple(Conn *c, Ndbtuple *t, int scanentries)
{
	int first;
	Ndbtuple *l, *e;
	char *s;

	first=1;
	s = c->host;
	for(l=t; first||l!=t; l=l->line, first=0){
		if(scanentries){
			for(e=l; e; e=e->entry){
				if(strcmp(e->val, c->host) != 0 &&
				  (strcmp(e->attr, "ip")==0 || strcmp(e->attr, "dom")==0 || strcmp(e->attr, "sys")==0)){
					s = smprint("%s %s", s, e->val);
					if(s == nil)
						error("out of memory");
				}
			}
		}
		if(strcmp(l->val, c->host) != 0 &&
		  (strcmp(l->attr, "ip")==0 || strcmp(l->attr, "dom")==0 || strcmp(l->attr, "sys")==0)){
			s = smprint("%s %s", s, l->val);
			if(s == nil)
				error("out of memory");
		}
	}
	s = trim(s);
	c->aliases = s;
}

void
setaliases(Conn *c, char *name)
{
	char *p, *net;
	char *attr[2];
	Ndbtuple *t;

	net = "/net";
	if(name[0]=='/'){
		p = strchr(name+1, '/');
		if(p){
			net = emalloc(p-name+1);
			memmove(net, name, p-name);
		}
	}
	if(p = strchr(name, '!'))
		name = p+1;

	c->host = emalloc(strlen(name)+1);
	strcpy(c->host, name);

	c->aliases = c->host;
	attr[0] = "dom";
	attr[1] = "ip";
	t = csipinfo(nil, ipattr(name), name, attr, 2);
	if(t != nil){
		usetuple(c, t, 0);
		ndbfree(t);
	}else{
		t = dnsquery(net, name, "ip");
		if(t != nil){
			usetuple(c, t, 1);
			ndbfree(t);
		}
	}
}

void
privatefactotum(void)
{
	char *user;
	Dir *d;

	if((user=getuser()) && (d=dirstat("/mnt/factotum/rpc")) && strcmp(user, d->uid)!=0){
		/* grab the terminal's factotum */
		rfork(RFNAMEG);	/* was RFNOTEG, which makes little sense */
		if(access("/mnt/term/mnt/factotum", AEXIST) >= 0){
//			fprint(2, "binding terminal's factotum\n");
			if(bind("/mnt/term/mnt/factotum", "/mnt/factotum", MREPL) < 0)
				sysfatal("cannot find factotum");
		}
	}
}
