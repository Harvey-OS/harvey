#include <u.h>
#include <libc.h>
#include <bio.h>
#include <auth.h>
#include <fcall.h>
#include <thread.h>
#include <9p.h>
#include "snap.h"

typedef struct PD PD;
struct PD {
	int isproc;
	union {
		Proc *p;
		Data *d;
	};
};

PD*
PDProc(Proc *p)
{
	PD *pd;

	pd = emalloc(sizeof(*pd));
	pd->isproc = 1;
	pd->p = p;
	return pd;
}

PD*
PDData(Data *d)
{
	PD *pd;

	pd = emalloc(sizeof(*pd));
	pd->isproc = 0;
	pd->d = d;
	return pd;
}

void
usage(void)
{
	fprint(2, "usage: snapfs [-m mtpt] file\n");
	exits("usage");
}

char*
memread(Proc *p, File *f, void *buf, long *count, vlong offset)
{
	Page *pg;
	int po;

	po = offset%Pagesize;
	if(!(pg = findpage(p, p->pid, f->name[0], offset-po)))
		return "address not mapped";

	if(*count > Pagesize-po)
		*count = Pagesize-po;

	memmove(buf, pg->data+po, *count);
	return nil;
}

char*
dataread(Data *d, void *buf, long *count, vlong offset)
{
	assert(d != nil);

	if(offset >= d->len) {
		*count = 0;
		return nil;
	}

	if(offset+*count >= d->len)
		*count = d->len - offset;

	memmove(buf, d->data+offset, *count);
	return nil;
}

void
snapread(Req *r, Fid *fid, void *buf, long *count, vlong offset)
{
	PD *pd;

	pd = fid->file->aux;
	if(pd->isproc)
		respond(r, memread(pd->p, fid->file, buf, count, offset));
	else
		respond(r, dataread(pd->d, buf, count, offset));
}

Srv snapsrv = {
	.read = snapread,
};

void
main(int argc, char **argv)
{
	Biobuf *b;
	Data *d;
	File *fdir, *f;
	Proc *p, *plist;
	Tree *tree;
	char *mtpt, buf[NAMELEN];
	int i, mflag;

	mtpt = "/proc";
	mflag = MBEFORE;

	ARGBEGIN{
	case 'a':
		mflag = MAFTER;
		break;
	case 'm':
		mtpt = ARGF();
		break;
	default:
		usage();
	}ARGEND

	if(argc != 1)
		usage();

	b = Bopen(argv[0], OREAD);
	if(b == nil) {
		fprint(2, "cannot open \"%s\": %r\n", argv[0]);
		exits("Bopen");
	}

	if((plist = readsnap(b)) == nil) {
		fprint(2, "readsnap fails\n");
		exits("readsnap");
	}

	tree = mktree(nil, nil, CHDIR|0555);
	snapsrv.tree = tree;

	for(p=plist; p; p=p->link) {
		print("process %ld %.*s\n", p->pid, NAMELEN, p->d[Pstatus] ? p->d[Pstatus]->data : "");

		sprint(buf, "%ld", p->pid);
		fdir = fcreate(tree->root, buf, nil, nil, CHDIR|0555);

		fcreate(fdir, "ctl", nil, nil, 0777);
		if(p->text) {
			f = fcreate(fdir, "text", nil, nil, 0777);
			f->aux = PDProc(p);
		}

		f = fcreate(fdir, "mem", nil, nil, 0666);
		f->aux = PDProc(p);

		for(i=0; i<Npfile; i++) {
			if(d = p->d[i]) {
				f = fcreate(fdir, pfile[i], nil, nil, 0666);
				f->aux = PDData(d);
				f->length = d->len;
			}
		}
	}

	postmountsrv(&snapsrv, nil, mtpt, mflag);
	exits(0);
}
