#include	"all.h"
#include	"mem.h"
#include	"io.h"
#include	"ureg.h"

#include	"9p1.h"

void
machinit(void)
{
	int n;

	n = m->machno;
	memset(m, 0, sizeof(Mach));
	m->machno = n;
	m->mmask = 1<<m->machno;
	m->lights = 0;

	active.exiting = 0;
	active.machs = 1;
}

static
void
confinit(void)
{
	conf.nmach = 1;
	conf.nproc = 40;

	conf.mem = meminit();
	conf.sparemem = conf.mem/12;		/* 8% spare for chk etc */

	conf.nalarm = 200;
	conf.nuid = 1000;
	conf.nserve = 15;
	conf.nfile = 30000;
	conf.nlgmsg = 100;
	conf.nsmmsg = 500;
	/*
	 * if you have trouble with IDE DMA or RWM (multi-sector transfers),
	 * perhaps due to old hardware, set idedma to zero in localconfinit().
	 */
	conf.idedma = 1;

	localconfinit();

	conf.nwpath = conf.nfile*8;
	conf.nauth = conf.nfile/10;
	conf.gidspace = conf.nuid*3;

	cons.flags = 0;
}

/*
 * compute BUFSIZE*(NDBLOCK+INDPERBUF+INDPERBUF⁲+INDPERBUF⁳+INDPERBUF⁴)
 * while watching for overflow; in that case, return 0.
 */

static uvlong
adduvlongov(uvlong a, uvlong b)
{
	uvlong r = a + b;

	return (r < a || r < b)? 0: r;
}

static uvlong
muluvlongov(uvlong a, uvlong b)
{
	uvlong r = a * b;

	return (r < a || r < b)? 0: r;
}

static uvlong
maxsize(void)
{
	int i;
	uvlong max = NDBLOCK, ind = 1;

	for (i = 0; i < NIBLOCK; i++) {
		ind = muluvlongov(ind, INDPERBUF);	/* power of INDPERBUF */
		if (ind == 0)
			return 0;
		max = adduvlongov(max, ind);
		if (max == 0)
			return 0;
	}
	return muluvlongov(max, BUFSIZE);
}

enum {
	INDPERBUF⁲ = ((Off)INDPERBUF *INDPERBUF),
	INDPERBUF⁴ = ((Off)INDPERBUF⁲*INDPERBUF⁲),
};

static void
printsizes(void)
{
	uvlong max = maxsize();

	print("\tblock size = %d; ", RBUFSIZE);
	if (max == 0)
		print("max file size exceeds 2⁶⁴ bytes\n");
	else {
		uvlong offlim = 1ULL << (sizeof(Off)*8 - 1);

		if (max >= offlim)
			max = offlim - 1;
		print("max file size = %,llud\n", (Wideoff)max);
	}
	print("\tINDPERBUF = %d, INDPERBUF^4 = %,lld, ", INDPERBUF,
		(Wideoff)INDPERBUF⁴);
	print("CEPERBK = %d\n", CEPERBK);
	print("\tsizeofs: Dentry = %d, Cache = %d\n",
		sizeof(Dentry), sizeof(Cache));
}

void
main(void)
{
	int i;

	echo = 1;
	predawn = 1;
		formatinit();
		machinit();
		vecinit();
		confinit();
		lockinit();
		printinit();
		procinit();
		clockinit();
		alarminit();

		mainlock.wr.name = "mainr";
		mainlock.rd.name = "mainw";
		reflock.name = "ref";

		qlock(&reflock);
		qunlock(&reflock);
		serveq = newqueue(1000);
		raheadq = newqueue(1000);

		mbinit();

		sntpinit();
		otherinit();

		files = ialloc(conf.nfile * sizeof(*files), 0);
		for(i=0; i<conf.nfile; i++) {
			qlock(&files[i]);
			files[i].name = "file";
			qunlock(&files[i]);
		}

		wpaths = ialloc(conf.nwpath * sizeof(*wpaths), 0);
		uid = ialloc(conf.nuid * sizeof(*uid), 0);
		gidspace = ialloc(conf.gidspace * sizeof(*gidspace), 0);
		authinit();

		print("Plan 9 %d-bit file server with %d-deep indir blks%s\n",
			sizeof(Off)*8 - 1, NIBLOCK,
			(conf.idedma? " and IDE DMA+RWM": ""));
		printsizes();

		print("iobufinit:");
		iobufinit();

		arginit();

		userinit(touser, 0, "ini");

	predawn = 0;
		launchinit();
		schedinit();
}

/*
 * read ahead processes.
 * read message from q and then
 * read the device.
 */
int
rbcmp(void *va, void *vb)
{
	Rabuf *ra, *rb;

	ra = *(Rabuf**)va;
	rb = *(Rabuf**)vb;
	if(rb == 0)
		return 1;
	if(ra == 0)
		return -1;
	if(ra->dev > rb->dev)
		return 1;
	if(ra->dev < rb->dev)
		return -1;
	if(ra->addr > rb->addr)
		return 1;
	if(ra->addr < rb->addr)
		return -1;
	return 0;
}

void
rahead(void)
{
	Rabuf *rb[50];
	Iobuf *p;
	int i, n;

	for (;;) {
		rb[0] = recv(raheadq, 0);
		for(n=1; n<nelem(rb); n++) {
			if(raheadq->count <= 0)
				break;
			rb[n] = recv(raheadq, 0);
		}
		qsort(rb, n, sizeof(rb[0]), rbcmp);
		for(i=0; i<n; i++) {
			if(rb[i] == 0)
				continue;
			p = getbuf(rb[i]->dev, rb[i]->addr, Bread);
			if(p)
				putbuf(p);
			lock(&rabuflock);
			rb[i]->link = rabuffree;
			rabuffree = rb[i];
			unlock(&rabuflock);
		}
	}
}

/*
 * main filesystem server loop.
 * entered by many processes.
 * they wait for message buffers and
 * then process them.
 */
void
serve(void)
{
	int i;
	Chan *cp;
	Msgbuf *mb;

	for (;;) {
		qlock(&reflock);
		mb = recv(serveq, 0);
		cp = mb->chan;
		rlock(&cp->reflock);
		qunlock(&reflock);

		rlock(&mainlock);

		if(cp->protocol == nil){
			/* do we recognise the protocol in this packet? */
			for(i = 0; fsprotocol[i] != nil; i++)
				if(fsprotocol[i](mb) != 0) {
					cp->protocol = fsprotocol[i];
					break;
				}
			if(cp->protocol == nil){
				print("no protocol for message\n");
				for(i = 0; i < 12; i++)
					print(" %2.2uX", mb->data[i]);
				print("\n");
			}
		}
		else
			cp->protocol(mb);

		mbfree(mb);
		runlock(&mainlock);
		runlock(&cp->reflock);
	}
}

void
init0(void)
{
	m->proc = u;
	u->state = Running;
	u->mach = m;
	spllo();

	(*u->start)();
}

void*
getarg(void)
{
	return u->arg;
}

enum {
	Keydelay = 5*60,	/* seconds to wait for key press */
};

void
exit(void)
{
	u = 0;
	lock(&active);
	active.machs &= ~(1<<m->machno);
	active.exiting = 1;
	unlock(&active);
	if(!predawn)
		spllo();
	print("cpu %d exiting\n", m->machno);
	while(active.machs)
		delay(1);

	print("halted.  press a key to reboot sooner than %d mins.\n",
		Keydelay/60);
	delay(300);		/* time to drain print q */

	splhi();
	/* reboot after delay (for debugging) or at key press */
	rawchar(Keydelay);
	consreset();
	firmware();
}

/*
 * 1 sec timer
 * process+alarm+rendez+rwlock
 */
static	Rendez	sec;

static
void
callsec(Alarm *a, void *arg)
{
	User *u = arg;

	cancel(a);
	wakeup(&u->tsleep);
}

void
waitsec(int msec)
{
	alarm(msec, callsec, u);
	sleep(&u->tsleep, no, 0);
}

#define	DUMPTIME	5	/* 5 am */
#define	WEEKMASK	0	/* every day (1=sun, 2=mon, 4=tue, etc.) */

/*
 * calculate the next dump time.
 * minimum delay is 100 minutes.
 */
Timet
nextdump(Timet t)
{
	Timet nddate = nextime(t+MINUTE(100), DUMPTIME, WEEKMASK);

	if(!conf.nodump)
		print("next dump at %T\n", nddate);
	return nddate;
}

/*
 * process to copy dump blocks from
 * cache to worm. it runs flat out when
 * it gets work, but only looks for
 * work every 10 seconds.
 */
void
wormcopy(void)
{
	int f, dorecalc = 1;
	Timet dt, t = 0, nddate = 0, ntoytime = 0;
	Filsys *fs;

	for (;;) {
		if (dorecalc) {
			dorecalc = 0;
			t = time();
			nddate = nextdump(t);		/* chatters */
			ntoytime = time();
		}
		dt = time() - t;
		if(dt < 0 || dt > MINUTE(100)) {
			if(dt < 0)
				print("time went back\n");
			else
				print("time jumped ahead\n");
			dorecalc = 1;
			continue;
		}
		t += dt;
		f = 0;
		if(t > ntoytime) {
			dt = time() - rtctime();
			if(dt < 0)
				dt = -dt;
			if(dt > 10)
				print("rtc time more than 10 seconds out\n");
			else if(dt > 1)
				settime(rtctime());
			ntoytime = time() + HOUR(1);
		} else if(/* !f && */ t > nddate) { /* !f is always true here */
			if(!conf.nodump) {
				print("automatic dump %T\n", t);
				for(fs=filsys; fs->name; fs++)
					if(fs->dev->type == Devcw)
						cfsdump(fs);
			}
			dorecalc = 1;
		} else {
			rlock(&mainlock);
			for(fs=filsys; fs->name; fs++)
				if(fs->dev->type == Devcw)
					f |= dumpblock(fs->dev);
			runlock(&mainlock);

			if(0 && f == 0)
				f = 0;		/* f = dowcp(); */
			if(!f)
				waitsec(10000);
			wormprobe();
		}
	}
}

/*
 * process to synch blocks
 * it puts out a block/cache-line every second
 * it waits 10 seconds if caught up.
 * in both cases, it takes about 10 seconds
 * to get up-to-date.
 */
void
synccopy(void)
{
	int f;

	for (;;) {
		rlock(&mainlock);
		f = syncblock();
		runlock(&mainlock);
		if(!f)
			waitsec(10000);
		else
			waitsec(1000);
		/* pokewcp();	*/
	}
}
