#include <u.h>
#include <libc.h>
#include <fcall.h>
#include <thread.h>
#include <9p.h>

uint time0;

enum
{
	Qroot = 0,
	Qkid,
};

char *kidname;
uint kidmode;

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
	switch((int)fid->qid.path){
	default:
		return "path not found";
	case Qroot:
		if(strcmp(name, "..") == 0)
			break;
		if(strcmp(name, kidname) == 0){
			fid->qid = (Qid){Qkid, 0, kidmode>>24};
			break;
		}
		return "path not found";
	case Qkid:
		if(strcmp(name, "..") == 0){
			fid->qid = (Qid){Qroot, 0, QTDIR};
			break;
		}
		return "path not found";
	}
	*qid = fid->qid;
	return nil;
}

void
fsstat(Req *r)
{
	int q;
	Dir *d;

	d = &r->d;
	memset(d, 0, sizeof *d);
	q = r->fid->qid.path;
	d->qid = r->fid->qid;
	switch(q){
	case Qroot:
		d->name = estrdup9p("/");
		d->mode = DMDIR|0777;
		break;

	case Qkid:
		d->name = estrdup9p(kidname);
		d->mode = kidmode;
		break;
	}

	d->atime = d->mtime = time0;
	d->uid = estrdup9p("stub");
	d->gid = estrdup9p("stub");
	d->muid = estrdup9p("");
	respond(r, nil);
}

int
dirgen(int off, Dir *d, void*)
{
	if(off != 0)
		return -1;

	memset(d, 0, sizeof *d);
	d->atime = d->mtime = time0;
	d->name = estrdup9p(kidname);
	d->mode = kidmode;
	d->qid = (Qid){Qkid, 0, kidmode>>24};
	d->qid.type = d->mode>>24;
	d->uid = estrdup9p("stub");
	d->gid = estrdup9p("stub");
	d->muid = estrdup9p("");
	return 0;
}

void
fsread(Req *r)
{
	int q;

	q = r->fid->qid.path;
	switch(q){
	default:
		respond(r, "bug");
		return;

	case Qroot:
		dirread9p(r, dirgen, nil);
		respond(r, nil);
		return;
	}
}

void
fswrite(Req *r)
{
	respond(r, "no writing");
}

void
fsopen(Req *r)
{
	if(r->fid->qid.path != Qroot){
		respond(r, "permission denied");
		return;
	}

	if(r->ifcall.mode != OREAD)
		respond(r, "permission denied");
	else
		respond(r, nil);
}

Srv fs = {
	.attach=	fsattach,
	.open=	fsopen,
	.read=	fsread,
	.write=	fswrite,
	.stat=	fsstat,
	.walk1=	fswalk1,
};

void
usage(void)
{
	fprint(2, "usage: aux/stub [-Dd] path/name\n");
	exits("usage");
}

void
main(int argc, char **argv)
{
	char *p, *mtpt;

	quotefmtinstall();

	time0 = time(0);
	ARGBEGIN{
	case 'D':
		chatty9p++;
		break;
	case 'd':
		kidmode = DMDIR;
		break;
	default:
		usage();
	}ARGEND

	if(argc != 1)
		usage();

	if((p = strrchr(argv[0], '/')) == 0){
		mtpt = ".";
		kidname = argv[0];
	}else if(p == argv[0]){
		mtpt = "/";
		kidname = argv[0]+1;
	}else{
		mtpt = argv[0];
		*p++ = '\0';
		kidname = p;
	}
	postmountsrv(&fs, nil, mtpt, MBEFORE);
	exits(nil);
}
