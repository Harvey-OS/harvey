#include <u.h>
#include <libc.h>
#include <auth.h>
#include <fcall.h>
#include <bio.h>

uint messagesize = 65536;	/* just a buffer size */

void
usage(void)
{
	fprint(2, "usage: aux/9pcon [-m messagesize] /srv/service | -c command | -n networkaddress\n");
	exits("usage");
}

int
connectcmd(char *cmd)
{
	int p[2];

	if(pipe(p) < 0)
		return -1;
	switch(fork()){
	case -1:
		fprint(2, "fork failed: %r\n");
		_exits("exec");
	case 0:
		dup(p[0], 0);
		dup(p[0], 1);
		close(p[1]);
		execl("/bin/rc", "rc", "-c", cmd, nil);
		fprint(2, "exec failed: %r\n");
		_exits("exec");
	default:
		close(p[0]);
		return p[1];
	}
}

void
watch(int fd)
{
	int n;
	uchar *buf;
	Fcall f;
	
	buf = malloc(messagesize);
	if(buf == nil)
		sysfatal("out of memory");

	while((n = read9pmsg(fd, buf, messagesize)) > 0){
		if(convM2S(buf, n, &f) == 0){
			print("convM2S: %r\n");
			continue;
		}
		print("\t<- %F\n", &f);
	}
	if(n == 0)
		print("server eof\n");
	else
		print("read9pmsg from server: %r\n");
}

char*
tversion(Fcall *f, int, char **argv)
{
	f->msize = atoi(argv[0]);
	if(f->msize > messagesize)
		return "message size too big; use -m option on command line";
	f->version = argv[1];
	return nil;
}

char*
tauth(Fcall *f, int, char **argv)
{
	f->afid = atoi(argv[0]);
	f->uname = argv[1];
	f->aname = argv[2];
	return nil;
}

char*
tflush(Fcall *f, int, char **argv)
{
	f->oldtag = atoi(argv[0]);
	return nil;
}

char*
tattach(Fcall *f, int, char **argv)
{
	f->fid = atoi(argv[0]);
	f->afid = atoi(argv[1]);
	f->uname = argv[2];
	f->aname = argv[3];
	return nil;
}

char*
twalk(Fcall *f, int argc, char **argv)
{
	int i;

	if(argc < 2)
		return "usage: Twalk tag fid newfid [name...]";
	f->fid = atoi(argv[0]);
	f->newfid = atoi(argv[1]);
	f->nwname = argc-2;
	if(f->nwname > MAXWELEM)
		return "too many names";
	for(i=0; i<argc-2; i++)
		f->wname[i] = argv[2+i];
	return nil;
}

char*
topen(Fcall *f, int, char **argv)
{
	f->fid = atoi(argv[0]);
	f->mode = atoi(argv[1]);
	return nil;
}

char*
tcreate(Fcall *f, int, char **argv)
{
	f->fid = atoi(argv[0]);
	f->name = argv[1];
	f->perm = strtoul(argv[2], 0, 8);
	f->mode = atoi(argv[3]);
	return nil;
}

char*
tread(Fcall *f, int, char **argv)
{
	f->fid = atoi(argv[0]);
	f->offset = strtoll(argv[1], 0, 0);
	f->count = strtol(argv[2], 0, 0);
	return nil;
}

char*
twrite(Fcall *f, int, char **argv)
{
	f->fid = atoi(argv[0]);
	f->offset = strtoll(argv[1], 0, 0);
	f->data = argv[2];
	f->count = strlen(argv[2]);
	return nil;
}

char*
tclunk(Fcall *f, int, char **argv)
{
	f->fid = atoi(argv[0]);
	return nil;
}

char*
tremove(Fcall *f, int, char **argv)
{
	f->fid = atoi(argv[0]);
	return nil;
}

char*
tstat(Fcall *f, int, char **argv)
{
	f->fid = atoi(argv[0]);
	return nil;
}

ulong
xstrtoul(char *s)
{
	if(strcmp(s, "~0") == 0)
		return ~0UL;
	return strtoul(s, 0, 0);
}

uvlong
xstrtoull(char *s)
{
	if(strcmp(s, "~0") == 0)
		return ~0ULL;
	return strtoull(s, 0, 0);
}

char*
twstat(Fcall *f, int, char **argv)
{
	static uchar buf[DIRMAX];
	Dir d;

	memset(&d, 0, sizeof d);
	nulldir(&d);
	d.name = argv[1];
	d.uid = argv[2];
	d.gid = argv[3];
	d.mode = xstrtoul(argv[4]);
	d.mtime = xstrtoul(argv[5]);
	d.length = xstrtoull(argv[6]);

	f->fid = atoi(argv[0]);
	f->stat = buf;
	f->nstat = convD2M(&d, buf, sizeof buf);
	if(f->nstat < BIT16SZ)
		return "convD2M failed (internal error)";

	return nil;
}

int taggen;

char*
settag(Fcall*, int, char **argv)
{
	static char buf[120];

	taggen = atoi(argv[0])-1;
	snprint(buf, sizeof buf, "next tag is %d", taggen+1);
	return buf;
}

typedef struct Cmd Cmd;
struct Cmd {
	char *name;
	int type;
	int argc;
	char *usage;
	char *(*fn)(Fcall *f, int, char**);
};

Cmd msg9p[] = {
	"Tversion", Tversion, 2, "messagesize version", tversion,
	"Tauth", Tauth, 3, "afid uname aname", tauth,
	"Tflush", Tflush, 1, "oldtag", tflush,
	"Tattach", Tattach, 4, "fid afid uname aname", tattach,
	"Twalk", Twalk, 0, "fid newfid [name...]", twalk,
	"Topen", Topen, 2, "fid mode", topen,
	"Tcreate", Tcreate, 4, "fid name perm mode", tcreate,
	"Tread", Tread, 3, "fid offset count", tread,
	"Twrite", Twrite, 3, "fid offset data", twrite,
	"Tclunk", Tclunk, 1, "fid", tclunk,
	"Tremove", Tremove, 1, "fid", tremove,
	"Tstat", Tstat, 1, "fid", tstat,
	"Twstat", Twstat, 7, "fid name uid gid mode mtime length", twstat,
	"nexttag", 0, 0, "", settag,
};

void
shell9p(int fd)
{
	char *e, *f[10], *p;
	uchar *buf;
	int i, n, nf;
	Biobuf b;
	Fcall t;

	buf = malloc(messagesize);
	if(buf == nil){
		fprint(2, "out of memory\n");
		return;
	}

	taggen = 0;
	Binit(&b, 0, OREAD);
	while(p = Brdline(&b, '\n')){
		p[Blinelen(&b)-1] = '\0';
		if(p[0] == '#')
			continue;
		if((nf = tokenize(p, f, nelem(f))) == 0)
			continue;
		for(i=0; i<nelem(msg9p); i++)
			if(strcmp(f[0], msg9p[i].name) == 0)
				break;
		if(i == nelem(msg9p)){
			fprint(2, "?unknown message\n");
			continue;
		}
		memset(&t, 0, sizeof t);
		t.type = msg9p[i].type;
		if(t.type == Tversion)
			t.tag = NOTAG;
		else
			t.tag = ++taggen;
		if(nf < 1 || (msg9p[i].argc && nf != 1+msg9p[i].argc)){
			fprint(2, "?usage: %s %s\n", msg9p[i].name, msg9p[i].usage);
			continue;
		}
		if(e = msg9p[i].fn(&t, nf-1, f+1)){
			fprint(2, "?%s\n", e);
			continue;
		}
		n = convS2M(&t, buf, messagesize);
		if(n <= BIT16SZ){
			fprint(2, "?message too large for buffer\n");
			continue;
		}
		if(write(fd, buf, n) != n){
			fprint(2, "?write fails: %r\n");
			break;
		}
		print("\t-> %F\n", &t);
	}
}
		
void
main(int argc, char **argv)
{
	int fd, pid, cmd, net;

	cmd = 0;
	net = 0;
	ARGBEGIN{
	case 'c':
		cmd = 1;
		break;
	case 'm':
		messagesize = atoi(EARGF(usage()));
		break;
	case 'n':
		net = 1;
		break;
	default:
		usage();
	}ARGEND

	fmtinstall('F', fcallfmt);
	fmtinstall('D', dirfmt);
	fmtinstall('M', dirmodefmt);

	if(argc != 1)
		usage();

	if(cmd && net)
		usage();

	if(cmd)
		fd = connectcmd(argv[0]);
	else if(net){
		fd = dial(netmkaddr(argv[0], "net", "9fs"), 0, 0, 0);
		if(fd < 0)
			sysfatal("dial: %r");
	}else{
		fd = open(argv[0], ORDWR);
		if(fd < 0)
			sysfatal("open: %r");
	}

	switch(pid = rfork(RFPROC|RFMEM)){
	case -1:
		sysfatal("rfork: %r");
		break;
	case 0:
		watch(fd);
		postnote(PNPROC, getppid(), "kill");
		break;
	default:
		shell9p(fd);
		postnote(PNPROC, pid, "kill");
		break;
	}
	exits(nil);
}
