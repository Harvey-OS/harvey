
/*
Oh even better, just add it to our proc like magic. 
We know where our proc is. 

. jUst put it there. 

so what are we doing here? 

that is a good question. 

how are we going to do this? I can just read it. 
need to create filesystem. 
*/

enum {
	Xdynld	=1,
	Xdynsyms,
};

struct {
	char *s;
	int id;
	int mode;
} tab[] = {
	"dynsyms",		Xdynsyms,		0444,
	"dynld",	Xdynld,	0444,
};


void
dopen(Req *r)
{
	respond(r, nil);
}

extern long readdl(void *a, ulong n, ulong offset);
static long readsyms(char *a, ulong n, ulong offset);

void
dread(Req *r)
{
	char *buf;
	// how do I get the information.
	switch((uintptr)r->fid->file->aux) {
	case Xdynld:
		readdl(r->ofcall.data, r->ifcall.count, r->ifcall.offset);
		respond(r, nil);
		break;
	case Xdynsyms:
		readsyms(r->ofcall.data, r->ifcall.count, r->ifcall.offset);
		respond(r, nil);
		break;
	}

}

Srv dynsrv = {
	.open=	dopen,
	.read=	dread,
};


void
adddynproc() {
	int fd;
	char buf[32];
	chatty9p++;
	print("postmountsrving");
	snprint(buf, sizeof(buf), "/proc/%d", getpid());
	postmountsrv(&dynsrv, "dynsrv", buf, MAFTER);
}