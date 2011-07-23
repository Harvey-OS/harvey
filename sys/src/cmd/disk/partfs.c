/*
 * partfs - serve an underlying file, with devsd-style partitions
 */
#include <u.h>
#include <libc.h>
#include <auth.h>
#include <fcall.h>
#include <thread.h>
#include <9p.h>

typedef struct Part Part;
struct Part
{
	int	inuse;
	int	vers;
	ulong	mode;
	char	*name;
	vlong	offset;		/* in sectors */
	vlong	length;		/* in sectors */
};

enum
{
	Qroot = 0,
	Qdir,
	Qctl,
	Qpart,
};

int fd = -1, ctlfd = -1;
int rdonly;
ulong ctlmode = 0666;
ulong time0;
vlong nsect, sectsize;

char *inquiry = "partfs hard drive";
char *sdname = "sdXX";
Part tab[64];

char*
ctlstring(void)
{
	Part *p;
	Fmt fmt;

	fmtstrinit(&fmt);
	fmtprint(&fmt, "inquiry %s\n", inquiry);
	fmtprint(&fmt, "geometry %lld %lld\n", nsect, sectsize);
	for (p = tab; p < tab + nelem(tab); p++)
		if (p->inuse)
			fmtprint(&fmt, "part %s %lld %lld\n",
				p->name, p->offset, p->length);
	return fmtstrflush(&fmt);
}

int
addpart(char *name, vlong start, vlong end)
{
	Part *p;

	if(start < 0 || start > end || end > nsect){
		werrstr("bad partition boundaries");
		return -1;
	}

	if (strcmp(name, "ctl") == 0 || strcmp(name, "data") == 0) {
		werrstr("partition name already in use");
		return -1;
	}
	for (p = tab; p < tab + nelem(tab) && p->inuse; p++)
		if (strcmp(p->name, name) == 0) {
			werrstr("partition name already in use");
			return -1;
		}
	if(p == tab + nelem(tab)){
		werrstr("no free partition slots");
		return -1;
	}

	p->inuse = 1;
	free(p->name);
	p->name = estrdup9p(name);
	p->offset = start;
	p->length = end - start;
	p->mode = ctlmode;
	p->vers++;
	return 0;
}

int
delpart(char *s)
{
	Part *p;

	for (p = tab; p < tab + nelem(tab); p++)
		if(p->inuse && strcmp(p->name, s) == 0)
			break;
	if(p == tab + nelem(tab)){
		werrstr("partition not found");
		return -1;
	}

	p->inuse = 0;
	free(p->name);
	p->name = nil;
	return 0;
}

static void
addparts(char *buf)
{
	char *f[4], *p, *q;

	/*
	 * Use partitions passed from boot program,
	 * e.g.
	 *	sdC0part=dos 63 123123/plan9 123123 456456
	 * This happens before /boot sets hostname so the
	 * partitions will have the null-string for user.
	 */
	for(p = buf; p != nil; p = q){
		if(q = strchr(p, '/'))
			*q++ = '\0';
		if(tokenize(p, f, nelem(f)) >= 3 &&
		    addpart(f[0], strtoull(f[1], 0, 0), strtoull(f[2], 0, 0)) < 0)
			fprint(2, "%s: addpart %s: %r\n", argv0, f[0]);
	}
}

static void
ctlwrite0(Req *r, char *msg, Cmdbuf *cb)
{
	vlong start, end;
	Part *p;

	r->ofcall.count = r->ifcall.count;

	if(cb->nf < 1){
		respond(r, "empty control message");
		return;
	}

	if(strcmp(cb->f[0], "part") == 0){
		if(cb->nf != 4){
			respondcmderror(r, cb, "part takes 3 args");
			return;
		}
		start = strtoll(cb->f[2], 0, 0);
		end = strtoll(cb->f[3], 0, 0);
		if(addpart(cb->f[1], start, end) < 0){
			respondcmderror(r, cb, "%r");
			return;
		}
	}
	else if(strcmp(cb->f[0], "delpart") == 0){
		if(cb->nf != 2){
			respondcmderror(r, cb, "delpart takes 1 arg");
			return;
		}
		if(delpart(cb->f[1]) < 0){
			respondcmderror(r, cb, "%r");
			return;
		}
	}
	else if(strcmp(cb->f[0], "inquiry") == 0){
		if(cb->nf != 2){
			respondcmderror(r, cb, "inquiry takes 1 arg");
			return;
		}
		free(inquiry);
		inquiry = estrdup9p(cb->f[1]);
	}
	else if(strcmp(cb->f[0], "geometry") == 0){
		if(cb->nf != 3){
			respondcmderror(r, cb, "geometry takes 2 args");
			return;
		}
		nsect = strtoll(cb->f[1], 0, 0);
		sectsize = strtoll(cb->f[2], 0, 0);
		if(tab[0].inuse && strcmp(tab[0].name, "data") == 0 &&
		    tab[0].vers == 0){
			tab[0].offset = 0;
			tab[0].length = nsect;
		}
		for(p = tab; p < tab + nelem(tab); p++)
			if(p->inuse && p->offset + p->length > nsect){
				p->inuse = 0;
				free(p->name);
				p->name = nil;
			}
	} else
		/* pass through to underlying ctl file, if any */
		if (write(ctlfd, msg, r->ifcall.count) != r->ifcall.count) {
			respondcmderror(r, cb, "%r");
			return;
		}
	respond(r, nil);
}

void
ctlwrite(Req *r)
{
	char *msg;
	Cmdbuf *cb;

	r->ofcall.count = r->ifcall.count;

	msg = emalloc9p(r->ifcall.count+1);
	memmove(msg, r->ifcall.data, r->ifcall.count);
	msg[r->ifcall.count] = '\0';

	cb = parsecmd(r->ifcall.data, r->ifcall.count);
	ctlwrite0(r, msg, cb);

	free(cb);
	free(msg);
}

int
rootgen(int off, Dir *d, void*)
{
	memset(d, 0, sizeof *d);
	d->atime = time0;
	d->mtime = time0;
	if(off == 0){
		d->name = estrdup9p(sdname);
		d->mode = DMDIR|0777;
		d->qid.path = Qdir;
		d->qid.type = QTDIR;
		d->uid = estrdup9p("partfs");
		d->gid = estrdup9p("partfs");
		d->muid = estrdup9p("");
		return 0;
	}
	return -1;
}

int
dirgen(int off, Dir *d, void*)
{
	int n;
	Part *p;

	memset(d, 0, sizeof *d);
	d->atime = time0;
	d->mtime = time0;
	if(off == 0){
		d->name = estrdup9p("ctl");
		d->mode = ctlmode;
		d->qid.path = Qctl;
		goto Have;
	}

	off--;
	n = 0;
	for(p = tab; p < tab + nelem(tab); p++, n++){
		if(!p->inuse)
			continue;
		if(n == off){
			d->name = estrdup9p(p->name);
			d->length = p->length*sectsize;
			d->mode = p->mode;
			d->qid.path = Qpart + p - tab;
			d->qid.vers = p->vers;
			goto Have;
		}
	}
	return -1;

Have:
	d->uid = estrdup9p("partfs");
	d->gid = estrdup9p("partfs");
	d->muid = estrdup9p("");
	return 0;
}

int
rdwrpart(Req *r)
{
	int q;
	long count, tot;
	vlong offset;
	uchar *dat;
	Part *p;

	q = r->fid->qid.path - Qpart;
	if(q < 0 || q > nelem(tab) || !tab[q].inuse ||
	    tab[q].vers != r->fid->qid.vers){
		respond(r, "unknown partition");
		return -1;
	}
	p = &tab[q];

	offset = r->ifcall.offset;
	count = r->ifcall.count;
	if(offset < 0){
		respond(r, "negative offset");
		return -1;
	}
	if(count < 0){
		respond(r, "negative count");
		return -1;
	}
	if(offset > p->length*sectsize){
		respond(r, "offset past end of partition");
		return -1;
	}
	if(offset+count > p->length*sectsize)
		count = p->length*sectsize - offset;
	offset += p->offset*sectsize;

	if(r->ifcall.type == Tread)
		dat = (uchar*)r->ofcall.data;
	else
		dat = (uchar*)r->ifcall.data;

	/* pass i/o through to underlying file */
	seek(fd, offset, 0);
	if (r->ifcall.type == Twrite) {
		tot = write(fd, dat, count);
		if (tot != count) {
			respond(r, "%r");
			return -1;
		}
	} else {
		tot = read(fd, dat, count);
		if (tot < 0) {
			respond(r, "%r");
			return -1;
		}
	}
	r->ofcall.count = tot;
	respond(r, nil);
	return 0;
}

void
fsread(Req *r)
{
	char *s;

	switch((int)r->fid->qid.path){
	case Qroot:
		dirread9p(r, rootgen, nil);
		break;
	case Qdir:
		dirread9p(r, dirgen, nil);
		break;
	case Qctl:
		s = ctlstring();
		readstr(r, s);
		free(s);
		break;
	default:
		rdwrpart(r);
		return;
	}
	respond(r, nil);
}

void
fswrite(Req *r)
{
	switch((int)r->fid->qid.path){
	case Qroot:
	case Qdir:
		respond(r, "write to a directory?");
		break;
	case Qctl:
		ctlwrite(r);
		break;
	default:
		rdwrpart(r);
		break;
	}
}

void
fsopen(Req *r)
{
	if(r->ifcall.mode&ORCLOSE)
		respond(r, "cannot open ORCLOSE");

	switch((int)r->fid->qid.path){
	case Qroot:
	case Qdir:
		if(r->ifcall.mode != OREAD){
			respond(r, "bad mode for directory open");
			return;
		}
	}

	respond(r, nil);
}

void
fsstat(Req *r)
{
	int q;
	Dir *d;
	Part *p;

	d = &r->d;
	memset(d, 0, sizeof *d);
	d->qid = r->fid->qid;
	d->atime = d->mtime = time0;
	q = r->fid->qid.path;
	switch(q){
	case Qroot:
		d->name = estrdup9p("/");
		d->mode = DMDIR|0777;
		break;

	case Qdir:
		d->name = estrdup9p(sdname);
		d->mode = DMDIR|0777;
		break;

	case Qctl:
		d->name = estrdup9p("ctl");
		d->mode = 0666;
		break;

	default:
		q -= Qpart;
		if(q < 0 || q > nelem(tab) || tab[q].inuse == 0 ||
		    r->fid->qid.vers != tab[q].vers){
			respond(r, "partition no longer exists");
			return;
		}
		p = &tab[q];
		d->name = estrdup9p(p->name);
		d->length = p->length * sectsize;
		d->mode = p->mode;
		break;
	}

	d->uid = estrdup9p("partfs");
	d->gid = estrdup9p("partfs");
	d->muid = estrdup9p("");
	respond(r, nil);
}

void
fsattach(Req *r)
{
	char *spec;

	spec = r->ifcall.aname;
	if(spec && spec[0]){
		respond(r, "invalid attach specifier");
		return;
	}
	r->ofcall.qid = (Qid){Qroot, 0, QTDIR};
	r->fid->qid = r->ofcall.qid;
	respond(r, nil);
}

char*
fswalk1(Fid *fid, char *name, Qid *qid)
{
	Part *p;

	switch((int)fid->qid.path){
	case Qroot:
		if(strcmp(name, sdname) == 0){
			fid->qid.path = Qdir;
			fid->qid.type = QTDIR;
			*qid = fid->qid;
			return nil;
		}
		break;
	case Qdir:
		if(strcmp(name, "ctl") == 0){
			fid->qid.path = Qctl;
			fid->qid.vers = 0;
			fid->qid.type = 0;
			*qid = fid->qid;
			return nil;
		}
		for(p = tab; p < tab + nelem(tab); p++)
			if(p->inuse && strcmp(p->name, name) == 0){
				fid->qid.path = p - tab + Qpart;
				fid->qid.vers = p->vers;
				fid->qid.type = 0;
				*qid = fid->qid;
				return nil;
			}
		break;
	}
	return "file not found";
}

Srv fs = {
	.attach=fsattach,
	.open=	fsopen,
	.read=	fsread,
	.write=	fswrite,
	.stat=	fsstat,
	.walk1=	fswalk1,
};

char *mtpt = "/dev";
char *srvname;

void
usage(void)
{
	fprint(2, "usage: %s [-Dr] [-d sdname] [-m mtpt] [-p 9parts] "
		"[-s srvname] diskimage\n", argv0);
	fprint(2, "\tdefault mtpt is /dev\n");
	exits("usage");
}

void
main(int argc, char **argv)
{
	int isdir;
	char *file, *cname, *parts;
	Dir *dir;

	quotefmtinstall();
	time0 = time(0);
	parts = nil;

	ARGBEGIN{
	case 'D':
		chatty9p++;
		break;
	case 'd':
		sdname = EARGF(usage());
		break;
	case 'm':
		mtpt = EARGF(usage());
		break;
	case 'p':
		parts = EARGF(usage());
		break;
	case 'r':
		rdonly = 1;
		break;
	case 's':
		srvname = EARGF(usage());
		break;
	default:
		usage();
	}ARGEND

	if(argc != 1)
		usage();
	file = argv[0];
	dir = dirstat(file);
	if(!dir)
		sysfatal("%s: %r", file);
	isdir = (dir->mode & DMDIR) != 0;
	free(dir);

	if (isdir) {
		cname = smprint("%s/ctl", file);
		if ((ctlfd = open(cname, ORDWR)) < 0)
			sysfatal("open %s: %r", cname);
		file = smprint("%s/data", file);
	}
	if((fd = open(file, rdonly? OREAD: ORDWR)) < 0)
		sysfatal("open %s: %r", file);

	sectsize = 512;			/* conventional */
	dir = dirfstat(fd);
	if (dir)
		nsect = dir->length / sectsize;
	free(dir);

	inquiry = estrdup9p(inquiry);
	tab[0].inuse = 1;
	tab[0].name = estrdup9p("data");
	tab[0].mode = 0666;
	tab[0].length = nsect;

	/*
	 * hack for booting from usb: add 9load partitions.
	 */
	if(parts != nil)
		addparts(parts);

	postmountsrv(&fs, srvname, mtpt, MBEFORE);
	exits(nil);
}
