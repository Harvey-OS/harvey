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
	fprint(2, "usage: snapfs [-a] [-m mtpt] file\n");
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
fsread(Req *r)
{
	char *e;
	PD *pd;
	Fid *fid;
	void *data;
	vlong offset;
	long count;

	fid = r->fid;
	data = r->ofcall.data;
	offset = r->ifcall.offset;
	count = r->ifcall.count;
	pd = fid->file->aux;

	if(pd->isproc)
		e = memread(pd->p, fid->file, data, &count, offset);
	else
		e = dataread(pd->d, data, &count, offset);

	if(e == nil)
		r->ofcall.count = count;
	respond(r, e);
}

Srv fs = {
	.read = fsread,
};

File*
ecreatefile(File *a, char *b, char *c, ulong d, void *e)
{
	File *f;

	f = createfile(a, b, c, d, e);
	if(f == nil)
		sysfatal("error creating snap tree: %r");
	return f;
}

void
main(int argc, char **argv)
{
	Biobuf *b;
	Data *d;
	File *fdir, *f;
	Proc *p, *plist;
	Tree *tree;
	char *mtpt, buf[32];
	int i, mflag;

	mtpt = "/proc";
	mflag = MBEFORE;

	ARGBEGIN{
	case 'D':
		chatty9p++;
		break;
	case 'd':
		debug = 1;
		break;
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

	tree = alloctree(nil, nil, DMDIR|0555, nil);
	fs.tree = tree;

	for(p=plist; p; p=p->link) {
		print("process %ld %.*s\n", p->pid, 28, p->d[Pstatus] ? p->d[Pstatus]->data : "");

		snprint(buf, sizeof buf, "%ld", p->pid);
		fdir = ecreatefile(tree->root, buf, nil, DMDIR|0555, nil);
		ecreatefile(fdir, "ctl", nil, 0777, nil);
		if(p->text)
			ecreatefile(fdir, "text", nil, 0777, PDProc(p));

		ecreatefile(fdir, "mem", nil, 0666, PDProc(p));
		for(i=0; i<Npfile; i++) {
			if(d = p->d[i]) {
				f = ecreatefile(fdir, pfile[i], nil, 0666, PDData(d));
				f->length = d->len;
			}
		}
	}

	postmountsrv(&fs, nil, mtpt, mflag);
	exits(0);
}
