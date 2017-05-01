/* This is from the 9front realemu main.c
 * Released under the LICENSE file in this
 * directory, which is the MIT license.
 */
#include <u.h>
#include <libc.h>
#include "dat.h"
#include "fns.h"

/* for fs */
#include <auth.h>
#include <fcall.h>
#include <thread.h>
#include <9p.h>

static int portfd[5];
int porttrace;

#if 0
static unsigned long
gw1(unsigned char *p)
{
	print_func_entry();
	print_func_exit();
	return p[0];
}
static unsigned long
gw2(unsigned char *p)
{
	print_func_entry();
	print_func_exit();
	return (unsigned long)p[0] | (unsigned long)p[1]<<8;
}
static unsigned long
gw4(unsigned char *p)
{
	print_func_entry();
	print_func_exit();
	return (unsigned long)p[0] | (unsigned long)p[1]<<8 | (unsigned long)p[2]<<16 | (unsigned long)p[3]<<24;
}
static unsigned long (*gw[9])(unsigned char *p) = {
	[1] = gw1,
	[2] = gw2,
	[4] = gw4,
	[8] = gw4,
};

static void
pw1(unsigned char *p, unsigned long w)
{
	print_func_entry();
	p[0] = w & 0xFF;
	print_func_exit();
}
static void
pw2(unsigned char *p, unsigned long w)
{
	print_func_entry();
	p[0] = w & 0xFF;
	p[1] = (w>>8) & 0xFF;
	print_func_exit();
}
static void
pw4(unsigned char *p, unsigned long w)
{
	print_func_entry();
	p[0] = w & 0xFF;
	p[1] = (w>>8) & 0xFF;
	p[2] = (w>>16) & 0xFF;
	p[3] = (w>>24) & 0xFF;
	print_func_exit();
}
static void (*pw[5])(unsigned char *p, unsigned long w) = {
	[1] = pw1,
	[2] = pw2,
	[4] = pw4,
};

#endif

static char Ebusy[] = "device is busy";
static char Eintr[] = "interrupted";
static char Eperm[] = "permission denied";
//static char Eio[] = "i/o error";
static char Enonexist[] = "file does not exist";
static char Ebadspec[] = "bad attach specifier";
static char Ewalk[] = "walk in non directory";
//static char Ebadoff[] = "invalid offset";

static int flushed(void *);

enum {
	Qroot,
	Qctl,
	Nqid,
};

static struct Qtab {
	char *name;
	int mode;
	int type;
	int length;
} qtab[Nqid] = {
	"/",
		DMDIR|0555,
		QTDIR,
		0,

	"ctl",
		0666,
		0,
		0,
};

static void i2cinit(void)
{
}

static int
fillstat(unsigned long qid, Dir *d)
{
	print_func_entry();
	struct Qtab *t;

	memset(d, 0, sizeof(Dir));
	d->uid = "i2c";
	d->gid = "i2c";
	d->muid = "";
	d->qid = (Qid){qid, 0, 0};
	d->atime = time(0);
	t = qtab + qid;
	d->name = t->name;
	d->qid.type = t->type;
	d->mode = t->mode;
	d->length = t->length;
	print_func_exit();
	return 1;
}

static void
fsattach(Req *r)
{
	print_func_entry();
	char *spec;

	spec = r->ifcall.aname;
	if(spec && spec[0]){
		respond(r, Ebadspec);
		print_func_exit();
		return;
	}
	r->fid->qid = (Qid){Qroot, 0, QTDIR};
	r->ofcall.qid = r->fid->qid;
	respond(r, nil);
	print_func_exit();
}

static void
fsstat(Req *r)
{
	print_func_entry();
	fillstat((unsigned long)r->fid->qid.path, &r->d);
	r->d.name = estrdup9p(r->d.name);
	r->d.uid = estrdup9p(r->d.uid);
	r->d.gid = estrdup9p(r->d.gid);
	r->d.muid = estrdup9p(r->d.muid);
	respond(r, nil);
	print_func_exit();
}

static char*
fswalk1(Fid *fid, char *name, Qid *qid)
{
	print_func_entry();
	int i;
	unsigned long path;

	path = fid->qid.path;
	switch(path){
	case Qroot:
		if (strcmp(name, "..") == 0) {
			*qid = (Qid){Qroot, 0, QTDIR};
			fid->qid = *qid;
			print_func_exit();
			return nil;
		}
		for(i = fid->qid.path; i<Nqid; i++){
			if(strcmp(name, qtab[i].name) != 0)
				continue;
			*qid = (Qid){i, 0, 0};
			fid->qid = *qid;
			print_func_exit();
			return nil;
		}
		print_func_exit();
		return Enonexist;

	default:
		print_func_exit();
		return Ewalk;
	}
}

static void
fsopen(Req *r)
{
	print_func_entry();
	static int need[4] = { 4, 2, 6, 1 };
	struct Qtab *t;
	int n;

	t = qtab + r->fid->qid.path;
	n = need[r->ifcall.mode & 3];
	if((n & t->mode) != n)
		respond(r, Eperm);
	else
		respond(r, nil);
		print_func_exit();
}

static int
readtopdir(Fid *fid, unsigned char *buf, long off, int cnt, int blen)
{
	print_func_entry();
	int i, m, n;
	long pos;
	Dir d;

	n = 0;
	pos = 0;
	for (i = 1; i < Nqid; i++){
		fillstat(i, &d);
		m = convD2M(&d, &buf[n], blen-n);
		if(off <= pos){
			if(m <= BIT16SZ || m > cnt)
				break;
			n += m;
			cnt -= m;
		}
		pos += m;
	}
	print_func_exit();
	return n;
}

static Channel *reqchan;

static void
cpuproc(void *data)
{
	print_func_entry();
	unsigned long path;
	Req *r;
	long long o;
	unsigned long n;
	char *p;

	USED(o);
	USED(n);
	USED(p);


	threadsetname("cpuproc");

	while(r = recvp(reqchan)){
		if(flushed(r)){
			respond(r, Eintr);
			continue;
		}

		path = r->fid->qid.path;

		p = r->ifcall.data;
		n = r->ifcall.count;
		o = r->ifcall.offset;

		switch(((int)r->ifcall.type<<8)|path){

		case (Tread<<8) | Qctl:
			r->ofcall.count = 0;
			respond(r, nil);
			break;

		case (Twrite<<8) | Qctl:
/*
			if(n != sizeof rmu){
				fprint(2, "n is %d, sizeof(rmu) %d: %s\n", n, sizeof(rmu), Ebadureg);
				respond(r, Ebadureg);
				break;
			}
			memmove(&rmu, p, n);
			if(p = realmode(&cpu, &rmu, r)){
				respond(r, p);
				break;
			}
			r->ofcall.count = n;
*/
			r->ofcall.count = 0;
			respond(r, nil);
			break;
		}
	}
	print_func_exit();
}

static Channel *flushchan;

static int
flushed(void *r)
{
	print_func_entry();
	print_func_exit();
	return nbrecvp(flushchan) == r;
}

static void
fsflush(Req *r)
{
	print_func_entry();
	nbsendp(flushchan, r->oldreq);
	respond(r, nil);
	print_func_exit();
}

static void
dispatch(Req *r)
{
	print_func_entry();
	if(!nbsendp(reqchan, r))
		respond(r, Ebusy);
		print_func_exit();
}

static void
fsread(Req *r)
{
	print_func_entry();
	switch((unsigned long)r->fid->qid.path){
	case Qroot:
		r->ofcall.count = readtopdir(r->fid, (void*)r->ofcall.data, r->ifcall.offset,
			r->ifcall.count, r->ifcall.count);
		respond(r, nil);
		break;
	default:
		dispatch(r);
	}
	print_func_exit();
}

static void
fsend(Srv* srv)
{
	print_func_entry();
	threadexitsall(nil);
	print_func_exit();
}

static Srv fs = {
	.attach=		fsattach,
	.walk1=			fswalk1,
	.open=			fsopen,
	.read=			fsread,
	.write=			dispatch,
	.stat=			fsstat,
	.flush=			fsflush,
	.end=			fsend,
};

static void
usage(void)
{
	print_func_entry();
	fprint(2, "usgae:\t%s [-Dp] [-s srvname] [-m mountpoint]\n", argv0);
	exits("usage");
	print_func_exit();
}

void
threadmain(int argc, char *argv[])
{
	char *mnt = "/dev";
	char *srv = nil;

	portfd[0] = 0;

	ARGBEGIN{
	case 'D':
		chatty9p++;
		break;
	case 'p':
		porttrace = 1;
		break;
	case 'x':
		set_printx(1);
		break;
	case 's':
		srv = EARGF(usage());
		mnt = nil;
		break;
	case 'm':
		mnt = EARGF(usage());
		break;
	default:
		usage();
	}ARGEND

		 i2cinit();

	reqchan = chancreate(sizeof(Req*), 8);
	flushchan = chancreate(sizeof(Req*), 8);
	procrfork(cpuproc, nil, 16*1024, RFNAMEG|RFNOTEG);
	threadpostmountsrv(&fs, srv, mnt, MBEFORE);
}
