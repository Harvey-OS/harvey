#include	"all.h"

int	sfd;
int	rfd;
int	chat;
extern	char *wrenfile;
char	*myname;
int	cmdfd;
int	writeallow;	/* never on; for compatibility with fs */
int	wstatallow;
int	srvfd(char*, int);
void	usage(void);
void	confinit(void);
Chan	*chaninit(char*);
void	consinit(void);

void
main(int argc, char *argv[])
{
	Filsys *fs;
	int i, ream, fsok;
	int newbufsize, nocheck;
	char buf[NAMELEN];

	progname = "kfs";
	procname = "init";

	/*
	 * insulate from invokers environment
	 */
	rfork(RFNAMEG|RFNOTEG);

	confinit();
	sfd = -1;
	ream = 0;
	newbufsize = 0;
	nocheck = 0;
	wrenfile = "/dev/sd0fs";
	buf[0] = '\0';
	ARGBEGIN{
	case 'b':
		newbufsize = atol(ARGF());
		break;
	case 'c':
		nocheck = 1;
		break;
	case 'f':
		wrenfile = ARGF();
		break;
	case 'n':
		strncpy(buf, ARGF(), NAMELEN-1);
		buf[NAMELEN-1] = '\0';
		break;
	case 'r':
		ream = 1;
		break;
	case 's':
		sfd = 0;
		rfd = 1;
		break;
	case 'B':
		conf.niobuf = strtoul(ARGF(), 0, 0);
		break;
	case 'C':
		chat = 1;
		break;
	default:
		usage();
	}ARGEND
	USED(argc, argv);

	cmdfd = 2;

	formatinit();
	lockinit();
	lock(&newfplock);
	unlock(&newfplock);

	if(buf[0])
		sprint(service, "kfs.%s", buf);
	else
		strcpy(service, "kfs");
	chan = chaninit(service);
	consinit();
	files = ialloc(conf.nfile * sizeof(*files));
	for(i=0; i<conf.nfile; i++) {
		qlock(&files[i]);
		qunlock(&files[i]);
	}
	tlocks = ialloc(NTLOCK * sizeof *tlocks);
	wpaths = ialloc(conf.nwpath * sizeof(*wpaths));
	uid = ialloc(conf.nuid * sizeof(*uid));
	uidspace = ialloc(conf.uidspace * sizeof(*uidspace));
	gidspace = ialloc(conf.gidspace * sizeof(*gidspace));

	/*
	 * init global locks
	 */
	wlock(&mainlock); wunlock(&mainlock);
	lock(&wpathlock); unlock(&wpathlock);

	/*
	 * init the file system, ream it if needed, and get the block sizes
	 */
	ream = fsinit(ream, newbufsize);
	iobufinit();
	for(fs=filsys; fs->name; fs++)
		if(fs->flags & FREAM){		/* set by fsinit if reamed */
			ream++;
			rootream(fs->dev, getraddr(fs->dev));
			superream(fs->dev, superaddr(fs->dev));
		}

	boottime = time();

	consserve();
	fsok = superok(filsys[0].dev, superaddr(filsys[0].dev), 0);
	if(!nocheck && !ream && !fsok)
		cmd_exec("check fq");

	/*
	 * start up procs
	 */
	for(i=0; i<conf.nserve; i++)
		startproc(serve, "srv");

	startproc(syncproc, "sync");

	exits(0);
}

Chan *
getmsg(char *buf, Fcall *f, int *n)
{
	int fd;

	fd = chan->chan;
	for(;;){
		*n = read(fd, buf, MAXMSG + MAXDAT);
		if(chat)
			print("read msg %d\n", *n);
		if(*n == 0){
			continue;
		}
		if(*n < 0)
			return 0;
		if(convM2S(buf, f, *n))
			return chan;
		if(chat)
			print("bad convM2S\n");
	}
	return 0;
}

void
send(Chan *c, char *buf, int n)
{
	int fd, m;

	fd = c->chan;
	for(;;){
		m = write(fd, buf, n);
		if(m == n)
			return;
	}
}

/*
 * main filesystem server loop.
 * entered by many processes.
 * they wait for messages and
 * then process them.
 */
void
serve(void)
{
	Chan *cp;
	Fcall fi, fo;
	char msgbuf[MAXMSG + MAXDAT];
	int t, n;

loop:
	cp = getmsg(msgbuf, &fi, &n);
	if(!cp)
		panic("input channel read error");

	/*
	 * simple syntax checks.
	 */
	t = fi.type;
	if(t < 0 || t >= MAXSYSCALL || (t&1) || !p9call[t]){
		print("bad message type\n");
		fo.tag = fi.tag;
		fo.type = Terror+1;
		strncpy(fo.ename, "unknown message type", sizeof(fo.ename));
		goto reply;
	}

	/*
	 * set up reply message
	 */
	fo.err = 0;
	if(t == Tread)
		fo.data = msgbuf + 8;

	/*
	 * stats
	 */
	cons.work.count++;
	cons.rate.count += n;

	/*
	 * call the file system
	 */
	rlock(&mainlock);
	rlock(&cp->reflock);

	(*p9call[t])(cp, &fi, &fo);

	runlock(&cp->reflock);
	runlock(&mainlock);

	fo.type = t + 1;
	fo.tag = fi.tag;

	if(fo.err) {
		if(CHAT(cp))
			print("	error: %s\n", errstr[fo.err]);
		fo.type = Terror+1;
		strncpy(fo.ename, errstr[fo.err], sizeof(fo.ename));
	}

reply:
	n = convS2M(&fo, msgbuf);
	if(!n) {
		print("bad S2M conversion\n");
		print("type=%d count=%d\n", msgbuf[0], n);
		print(" %.2x %.2x %.2x %.2x\n",
			msgbuf[1]&0xff, msgbuf[2]&0xff,
			msgbuf[3]&0xff, msgbuf[4]&0xff);
		print(" %.2x %.2x %.2x %.2x\n",
			msgbuf[5]&0xff, msgbuf[6]&0xff,
			msgbuf[7]&0xff, msgbuf[8]&0xff);
		print(" %.2x %.2x %.2x %.2x\n",
			msgbuf[9]&0xff, msgbuf[10]&0xff,
			msgbuf[11]&0xff, msgbuf[12]&0xff);
	}else{
		send(cp, msgbuf, n);
		cons.rate.count += n;
	}
	goto loop;
}

static
struct
{
	int	nfilter;
	Filter*	filters[100];
}f;

void
catchalarm(void *regs, char *msg)
{
	USED(regs, msg);
	if(strcmp(msg, "alarm") == 0)
		noted(NCONT);
	else
		noted(NDFLT);
}

/*
 * process to synch blocks
 * it puts out a block/line every second
 * it waits 10 seconds if catches up.
 * in both cases, it takes about 10 seconds
 * to get up-to-date.
 *
 * it also updates the filter stats
 * and executes commands
 */
void
syncproc(void)
{
	char buf[4*1024];
	Filter *ft;
	ulong c0, c1;
	long t, n, d;
	int i, p[2];

	/*
	 * make a pipe for commands
	 */
	if(pipe(p) < 0)
		panic("command pipe");
	sprint(buf, "#s/%s.cmd", service);
	srvfd(buf, p[0]);
	close(p[0]);
	cmdfd = p[1];
	notify(catchalarm);

	t = time();
	for(;;){
		i = syncblock();
		alarm(i ? 1000: 10000);
		n = read(cmdfd, buf, sizeof buf - 1);
		alarm(0);
		if(n > 0){
			buf[n] = '\0';
			if(cmd_exec(buf))
				fprint(cmdfd, "done");
			else
				fprint(cmdfd, "unknown command");
		}
		n = time();
		d = n - t;
		if(d < 0 || d > 5*60)
			d = 0;
		while(d >= 1) {
			d -= 1;
			for(i=0; i<f.nfilter; i++) {
				ft = f.filters[i];
				c0 = ft->count;
				c1 = c0 - ft->oldcount;
				ft->oldcount = c0;
				ft->filter[0] = famd(ft->filter[0], c1, 59, 60);
				ft->filter[1] = famd(ft->filter[1], c1, 599, 600);
				ft->filter[2] = famd(ft->filter[2], c1, 5999, 6000);
			}
		}
		t = n;
	}
}

void
dofilter(Filter *ft)
{
	int i;

	i = f.nfilter;
	if(i >= sizeof f.filters / sizeof f.filters[0]) {
		print("dofilter: too many filters\n");
		return;
	}
	f.filters[i] = ft;
	f.nfilter = i+1;
}

void
startproc(void (*f)(void), char *name)
{
	switch(rfork(RFMEM|RFFDG|RFPROC)){
	case -1:
		panic("can't fork");
	case 0:
		break;
	default:
		return;
	}
	procname = name;
	f();
}

void
confinit(void)
{
	int mul;

	conf.niobuf = 0;
	conf.nuid = 300;
	conf.nserve = 2;
	conf.uidspace = conf.nuid*6;
	conf.gidspace = conf.nuid*3;

	/*
	 * nfile should be close to conf.nchan for kernel
	 */
	mul = 1;
	conf.nfile = 250*mul;
	conf.nwpath = conf.nfile*8;

	cons.flags = 0;
}

Chan*
chaninit(char *server)
{
	Chan *cp;
	char buf[3*NAMELEN];
	int p[2];

	sprint(buf, "#s/%s", server);
	if(sfd < 0){
		if(pipe(p) < 0)
			panic("can't make a pipe");
		sfd = p[0];
		rfd = p[1];
	}
	srvfd(buf, sfd);
	close(sfd);
	cp = ialloc(sizeof *cp);
	cp->chan = rfd;
	strncpy(cp->whoname, "<none>", sizeof(cp->whoname));
	fileinit(cp);
	wlock(&cp->reflock);
	wunlock(&cp->reflock);
	lock(&cp->flock);
	unlock(&cp->flock);
	return cp;
}

int
srvfd(char *s, int sfd)
{
	int fd;
	char buf[32];

	fd = create(s, OWRITE, 0666);
	if(fd < 0){
		remove(s);
		fd = create(s, OWRITE, 0666);
		if(fd < 0)
			panic(s);
	}
	sprint(buf, "%d", sfd);
	if(write(fd, buf, strlen(buf)) != strlen(buf))
		panic("srv write");
	close(fd);
	return sfd;
}

void
consinit(void)
{
	int i;

	cons.chan = ialloc(sizeof(Chan));
	wlock(&cons.chan->reflock);
	wunlock(&cons.chan->reflock);
	lock(&cons.chan->flock);
	unlock(&cons.chan->flock);
	dofilter(&cons.work);
	dofilter(&cons.rate);
	dofilter(&cons.bhit);
	dofilter(&cons.bread);
	dofilter(&cons.binit);
	for(i = 0; i < MAXTAG; i++)
		dofilter(&cons.tags[i]);
}

/*
 * always called with mainlock locked
 */
void
syncall(void)
{
	for(;;)
		if(!syncblock())
			return;
}

int
askream(Filsys *fs)
{
	char c;

	print("File system %s inconsistent\n", fs->name);
	print("Would you like to ream it (y/n)? ");
	read(0, &c, 1);
	return c == 'y';
}

ulong
memsize(void)
{
	char *p, buf[128];
	int fd, n, by2pg, secs;

	by2pg = 4*1024;
	p = getenv("cputype");
	if(p && strcmp(p, "68020") == 0)
		by2pg = 8*1024;

	secs = 4*1024*1024;
	
	fd = open("/dev/swap", OREAD);
	if(fd < 0)
		return secs;
	n = read(fd, buf, sizeof(buf)-1);
	close(fd);
	if(n <= 0)
		return secs;
	buf[n] = 0;
	p = strchr(buf, '/');
	if(p)
		secs = strtoul(p+1, 0, 0)*by2pg;
	return secs;
}

/*
 * init the devices
 * wipe some of the file systems, or all if ream is set
 * this code really assumes that only one file system exists
 */
int
fsinit(int ream, int newbufsize)
{
	Filsys *fs;

	RBUFSIZE = 4 * 1024;
	for(fs=filsys; fs->name; fs++)
		(*devcall[fs->dev.type].init)(fs->dev);
	if(newbufsize == 0)
		newbufsize = RBUFSIZE;

	if(conf.niobuf == 0) {
		conf.niobuf = memsize()/10;
		if(conf.niobuf > 2*1024*1024)
			conf.niobuf = 2*1024*1024;
		conf.niobuf /= newbufsize;
		if(conf.niobuf < 30)
			conf.niobuf = 30;
	}

	BUFSIZE = RBUFSIZE - sizeof(Tag);

	for(fs=filsys; fs->name; fs++)
		if(ream || (*devcall[fs->dev.type].check)(fs->dev) && askream(fs)){
			RBUFSIZE = newbufsize;
			BUFSIZE = RBUFSIZE - sizeof(Tag);
			(*devcall[fs->dev.type].ream)(fs->dev);
			fs->flags |= FREAM;
			ream = 1;
		}

	/*
	 * set up the block size dependant variables
	 */
	BUFSIZE = RBUFSIZE - sizeof(Tag);
	DIRPERBUF = BUFSIZE / sizeof(Dentry);
	INDPERBUF = BUFSIZE / sizeof(long);
	INDPERBUF2 = INDPERBUF * INDPERBUF;
	FEPERBUF = (BUFSIZE - sizeof(Super1) - sizeof(long)) / sizeof(long);
	return ream;
}

/*
 * allocate rest of mem
 * for io buffers.
 */
#define	HWIDTH	5	/* buffers per hash */
void
iobufinit(void)
{
	long i;
	Iobuf *p, *q;
	Hiob *hp;

	i = conf.niobuf*RBUFSIZE;
	niob = i / (sizeof(Iobuf) + RBUFSIZE + sizeof(Hiob)/HWIDTH);
	nhiob = niob / HWIDTH;
	while(!prime(nhiob))
		nhiob++;
	if(chat)
		print("	%ld buffers; %ld hashes\n", niob, nhiob);
	hiob = ialloc(nhiob * sizeof(Hiob));
	hp = hiob;
	for(i=0; i<nhiob; i++) {
		lock(hp);
		unlock(hp);
		hp++;
	}
	p = ialloc(niob * sizeof(Iobuf));
	hp = hiob;
	for(i=0; i<niob; i++) {
		qlock(p);
		qunlock(p);
		if(hp == hiob)
			hp = hiob + nhiob;
		hp--;
		q = hp->link;
		if(q) {
			p->fore = q;
			p->back = q->back;
			q->back = p;
			p->back->fore = p;
		} else {
			hp->link = p;
			p->fore = p;
			p->back = p;
		}
		p->dev = devnone;
		p->addr = -1;
		p->xiobuf = ialloc(RBUFSIZE);
		p->iobuf = (char*)-1;
		p++;
	}
}

void
usage(void)
{
	fprint(2, "usage: kfs [-cCr] [-b bufsize] [-s infd outfd] [-f fsfile]\n");
	exits(0);
}
