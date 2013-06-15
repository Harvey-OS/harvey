/*
 * Remote debugging file system
 */

#include <u.h>
#include <libc.h>
#include <auth.h>
#include <fcall.h>
#include <bio.h>
#include <thread.h>
#include <9p.h>

int dbg = 0;
#define DBG	if(dbg)fprint

enum {
	NHASH = 4096,
	Readlen = 4,
	Pagequantum = 1024,
};

/* caching memory pages: a lot of space to avoid serial communications */
Lock pglock;
typedef struct	Page	Page;
struct Page {	/* cached memory contents */
	Page *link;
	ulong len;
	ulong addr;
	int count;
	uchar val[Readlen];
};

Page *pgtab[NHASH];

Page *freelist;

/* called with pglock locked */
Page*
newpg(void)
{
	int i;
	Page *p, *q;

	if(freelist == nil){
		p = malloc(sizeof(Page)*Pagequantum);
		if(p == nil)
			sysfatal("out of memory");

		for(i=0, q=p; i<Pagequantum-1; i++, q++)
			q->link = q+1;
		q->link = nil;

		freelist = p;
	}
	p = freelist;
	freelist = freelist->link;
	return p;
}

#define PHIINV 0.61803398874989484820
uint
ahash(ulong addr)
{
	return (uint)floor(NHASH*fmod(addr*PHIINV, 1.0));
}

int
lookup(ulong addr, uchar *val, ulong count)
{
	Page *p;

	lock(&pglock);
	for(p=pgtab[ahash(addr)]; p; p=p->link){
		if(p->addr == addr && p->count == count){
			memmove(val, p->val, count);
			unlock(&pglock);
			return 1;
		}
	}
	unlock(&pglock);
	return 0;
}

void
insert(ulong addr, uchar *val, int count)
{
	Page *p;
	uint h;

	lock(&pglock);
	p = newpg();
	p->addr = addr;
	p->count = count;
	memmove(p->val, val, count);
	h = ahash(addr);
	p->link = pgtab[h];
	p->len = pgtab[h] ? pgtab[h]->len+1 : 1;
	pgtab[h] = p;
	unlock(&pglock);
}

void
flushcache(void)
{
	int i;
	Page *p;

	lock(&pglock);
	for(i=0; i<NHASH; i++){
		if(p=pgtab[i]){
			for(;p->link; p=p->link)
				;
			p->link = freelist;
			freelist = p;
		}
		pgtab[i] = nil;
	}
	unlock(&pglock);
}

enum
{
	Xctl	= 1,
	Xfpregs,
	Xkregs,
	Xmem,
	Xproc,
	Xregs,
	Xtext,
	Xstatus,

};

int	textfd;
int	rfd;
Biobuf	rfb;
char*	portname = "/dev/eia0";
char*	textfile = "/386/9pc";
char*	procname = "1";
char*	srvname;
Channel* rchan;

void
usage(void)
{
	fprint(2, "usage: rdbfs [-p procnum] [-s srvname] [-t textfile] [serialport]\n");
	exits("usage");
}

void
noalarm(void*, char *msg)
{
	if(strstr(msg, "alarm"))
		noted(NCONT);
	noted(NDFLT);
}

/*
 * 	send and receive responses on the serial line
 */
void
eiaread(void*)
{
	Req *r;
	char *p;
	uchar *data;
	char err[ERRMAX];
	char buf[1000];
	int i, tries;

	notify(noalarm);
	while(r = recvp(rchan)){
		DBG(2, "got %F: here goes...", &r->ifcall);
		if(r->ifcall.count > Readlen)
			r->ifcall.count = Readlen;
		r->ofcall.count = r->ifcall.count;
		if(r->type == Tread && lookup(r->ifcall.offset, (uchar*)r->ofcall.data, r->ofcall.count)){
			respond(r, nil);
			continue;
		}
		for(tries=0; tries<5; tries++){
			if(r->type == Twrite){
				DBG(2, "w%.8lux %.8lux...", (ulong)r->ifcall.offset, *(ulong*)r->ifcall.data);
				fprint(rfd, "w%.8lux %.8lux\n", (ulong)r->ifcall.offset, *(ulong*)r->ifcall.data);
			}else if(r->type == Tread){
				DBG(2, "r%.8lux...", (ulong)r->ifcall.offset);
				fprint(rfd, "r%.8lux\n", (ulong)r->ifcall.offset);
			}else{
				respond(r, "oops");
				break;
			}
			for(;;){
				werrstr("");
				alarm(500);
				p=Brdline(&rfb, '\n');
				alarm(0);
				if(p == nil){
					rerrstr(err, sizeof err);
					DBG(2, "error %s\n", err);
					if(strstr(err, "alarm") || strstr(err, "interrupted"))
						break;
					if(Blinelen(&rfb) == 0) // true eof
						sysfatal("eof on serial line?");
					Bread(&rfb, buf, Blinelen(&rfb)<sizeof buf ? Blinelen(&rfb) : sizeof buf);
					continue;
				}
				p[Blinelen(&rfb)-1] = 0;
				if(p[0] == '\r')
					p++;
				DBG(2, "serial %s\n", p);
				if(p[0] == 'R'){
					if(strtoul(p+1, 0, 16) == (ulong)r->ifcall.offset){
						/* we know that data can handle Readlen bytes */
						data = (uchar*)r->ofcall.data;
						for(i=0; i<r->ifcall.count; i++)
							data[i] = strtol(p+1+8+1+3*i, 0, 16);
						insert(r->ifcall.offset, data, r->ifcall.count);
						respond(r, nil);
						goto Break2;
					}else
						DBG(2, "%.8lux ≠ %.8lux\n", strtoul(p+1, 0, 16), (ulong)r->ifcall.offset);
				}else if(p[0] == 'W'){
					respond(r, nil);
					goto Break2;
				}else{
					DBG(2, "unknown message\n");
				}
			}
		}
	Break2:;
	}
}

void
attachremote(char* name)
{
	int fd;
	char buf[128];

	print("attach %s\n", name);
	rfd = open(name, ORDWR);
	if(rfd < 0)
		sysfatal("can't open remote %s", name);

	sprint(buf, "%sctl", name);
	fd = open(buf, OWRITE);
	if(fd < 0)
		sysfatal("can't set baud rate on %s", buf);
	write(fd, "B9600", 6);
	close(fd);
	Binit(&rfb, rfd, OREAD);
}

void
fsopen(Req *r)
{
	char buf[ERRMAX];

	switch((uintptr)r->fid->file->aux){
	case Xtext:
		close(textfd);
		textfd = open(textfile, OREAD);
		if(textfd < 0) {
			snprint(buf, sizeof buf, "text: %r");
			respond(r, buf);
			return;
		}
		break;
	}		
	respond(r, nil);
}

void
fsread(Req *r)
{
	int i, n;
	char buf[512];

	switch((uintptr)r->fid->file->aux) {
	case Xfpregs:
	case Xproc:
	case Xregs:
		respond(r, "Egreg");
		break;
	case Xkregs:
	case Xmem:
		if(sendp(rchan, r) != 1){
			snprint(buf, sizeof buf, "rdbfs sendp: %r");
			respond(r, buf);
			return;
		}
		break;
	case Xtext:
		n = pread(textfd, r->ofcall.data, r->ifcall.count, r->ifcall.offset);
		if(n < 0) {
			rerrstr(buf, sizeof buf);
			respond(r, buf);
			break;
		}
		r->ofcall.count = n;
		respond(r, nil);
		break;
	case Xstatus:
		n = sprint(buf, "%-28s%-28s%-28s", "remote", "system", "New");
		for(i = 0; i < 9; i++)
			n += sprint(buf+n, "%-12d", 0);
		readstr(r, buf);
		respond(r, nil);
		break;
	default:
		respond(r, "unknown read");
	}
}

void
fswrite(Req *r)
{
	char buf[ERRMAX];

	switch((uintptr)r->fid->file->aux) {
	case Xctl:
		if(strncmp(r->ifcall.data, "kill", 4) == 0 ||
		   strncmp(r->ifcall.data, "exit", 4) == 0) {
			respond(r, nil);
			postnote(PNGROUP, getpid(), "umount");
			exits(nil);
		}else if(strncmp(r->ifcall.data, "refresh", 7) == 0){
			flushcache();
			respond(r, nil);
		}else if(strncmp(r->ifcall.data, "hashstats", 9) == 0){
			int i;
			lock(&pglock);
			for(i=0; i<NHASH; i++)
				if(pgtab[i])
					print("%lud ", pgtab[i]->len);
			print("\n");
			unlock(&pglock);
			respond(r, nil);
		}else
			respond(r, "permission denied");
		break;
	case Xkregs:
	case Xmem:
		if(sendp(rchan, r) != 1) {
			snprint(buf, sizeof buf, "rdbfs sendp: %r");
			respond(r, buf);
			return;
		}
		break;
	default:
		respond(r, "Egreg");
		break;
	}
}

struct {
	char *s;
	int id;
	int mode;
} tab[] = {
	"ctl",		Xctl,		0222,
	"fpregs",	Xfpregs,	0666,
	"kregs",	Xkregs,		0666,
	"mem",		Xmem,		0666,
	"proc",		Xproc,		0444,
	"regs",		Xregs,		0666,
	"text",		Xtext,		0444,
	"status",	Xstatus,	0444,
};

void
killall(Srv*)
{
	postnote(PNGROUP, getpid(), "kill");
}

Srv fs = {
.open=	fsopen,
.read=	fsread,
.write=	fswrite,
.end=	killall,
};

void
threadmain(int argc, char **argv)
{
	int i, p[2];
	File *dir;

	rfork(RFNOTEG);
	ARGBEGIN{
	case 'D':
		chatty9p++;
		break;
	case 'd':
		dbg = 1;
		break;
	case 'p':
		procname = EARGF(usage());
		break;
	case 's':
		srvname = EARGF(usage());
		break;
	case 't':
		textfile = EARGF(usage());
		break;
	default:
		usage();
	}ARGEND;

	switch(argc){
	case 0:
		break;
	case 1:
		portname = argv[0];
		break;
	default:
		usage();
	}

	rchan = chancreate(sizeof(Req*), 10);
	attachremote(portname);
	if(pipe(p) < 0)
		sysfatal("pipe: %r");

	fmtinstall('F', fcallfmt);
	proccreate(eiaread, nil, 8192);

	fs.tree = alloctree("rdbfs", "rdbfs", DMDIR|0555, nil);
	dir = createfile(fs.tree->root, procname, "rdbfs", DMDIR|0555, 0);
	for(i=0; i<nelem(tab); i++)
		closefile(createfile(dir, tab[i].s, "rdbfs", tab[i].mode, (void*)tab[i].id));
	closefile(dir);
	threadpostmountsrv(&fs, srvname, "/proc", MBEFORE);
	exits(0);
}

