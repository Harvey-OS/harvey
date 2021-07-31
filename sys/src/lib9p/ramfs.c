#include <u.h>
#include <libc.h>
#include <auth.h>
#include <fcall.h>
#include <thread.h>
#include "9p.h"

typedef struct Ramfile	Ramfile;

struct Ramfile {
	char *data;
	int ndata;
};

static char Ebad[] = "something bad happened";
static char Enomem[] = "no memory";

void
ramread(Req *r, Fid *fid, void *buf, long *count, vlong offset)
{
	Ramfile *rf;

	rf = fid->file->aux;

//print("read %ld %lld\n", *count, offset);
	if(offset >= rf->ndata){
		*count = 0;
		respond(r, nil);
		return;
	}

	if(offset+*count >= rf->ndata)
		*count = rf->ndata - offset;

	memmove(buf, rf->data+offset, *count);
	respond(r, nil);
}

void
ramwrite(Req *r, Fid *fid, void *buf, long *count, vlong offset)
{
	void *v;
	Ramfile *rf;

	rf = fid->file->aux;
	if(offset+*count >= rf->ndata){
		v = realloc(rf->data, offset+*count);
		if(v == nil){
			respond(r, Enomem);
			return;
		}
		rf->data = v;
		rf->ndata = offset+*count;
		fid->file->length = rf->ndata;
	}
	memmove(rf->data+offset, buf, *count);
	respond(r, nil);
}

void
ramcreate(Req *r, Fid *fid, char *name, int, ulong perm, Qid *qid)
{
	Ramfile *rf;
	File *f;

	if(f = fcreate(fid->file, name, fid->uid, fid->uid, perm)){
		rf = emalloc(sizeof *rf);
		f->aux = rf;
		fid->file = f;
		*qid = f->qid;
		respond(r, nil);
		return;
	}
	respond(r, Ebad);
}

void
ramrmaux(File *f)
{
	Ramfile *rf;

	rf = f->aux;
	free(rf->data);
	free(rf);
}

Srv ramsrv = {
	.read=	ramread,
	.write=	ramwrite,
	.create=	ramcreate,
};

void
usage(void)
{
	fprint(2, "usage: ramfs [-v] [-p | -s srvname] [-m mtpt]\n");
	exits("usage");
}

void
main(int argc, char **argv)
{
	char *srvname = "ramfs";
	char *mtpt = "/tmp";

	ramsrv.tree = mktree(nil, nil, CHDIR|0777);
	ramsrv.tree->rmaux = ramrmaux;

	ARGBEGIN{
	case 'v':
		lib9p_chatty++;
		break;
	case 'p':
		srvname = nil;
		break;
	case 's':
		srvname = ARGF();
		break;
	case 'm':
		mtpt = ARGF();
		break;
	default:
		usage();
	}ARGEND;

	if(argc)
		usage();

	postmountsrv(&ramsrv, srvname, mtpt, MREPL|MCREATE);
	exits(0);
}
