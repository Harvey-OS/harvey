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
	conf.nproc = 50;

	conf.mem = meminit();
	conf.sparemem = conf.mem/12;		/* 8% spare for chk etc */

	conf.nalarm = 200;
	conf.nuid = 1000;
	conf.nserve = 15;
	conf.nfile = 30000;
	conf.nlgmsg = 100;
	conf.nsmmsg = 500;
	conf.wcpsize = 1024*1024;
	localconfinit();

	conf.nwpath = conf.nfile*8;
	conf.nauth = conf.nfile/10;
	conf.gidspace = conf.nuid*3;

	cons.flags = 0;
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

		print("iobufinit\n");
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

loop:
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
	goto loop;
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

loop:
	qlock(&reflock);
	mb = recv(serveq, 0);
	cp = mb->chan;
	rlock(&cp->reflock);
	qunlock(&reflock);

	rlock(&mainlock);

	/*
	 */
	if(cp->protocol == nil){
		for(i = 0; fsprotocol[i] != nil; i++){
			if(fsprotocol[i](mb) == 0)
				continue;
			cp->protocol = fsprotocol[i];
			break;
		}
		if(cp->protocol == 0){
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
	goto loop;
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
	delay(300);	/* time to drain print q */
	splhi();
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
	User *u;

	cancel(a);
	u = arg;
	wakeup(&u->tsleep);
}

void
waitsec(int msec)
{
	alarm(msec, callsec, u);
	sleep(&u->tsleep, no, 0);
}

#define	DUMPTIME	5	/* 5 am */
#define	WEEKMASK	0	/* every day (1=sun, 2=mon 4=tue) */

/*
 * process to copy dump blocks from
 * cache to worm. it runs flat out when
 * it gets work, but only looks for
 * work every 10 seconds.
 */
void
wormcopy(void)
{
	int f;
	Filsys *fs;
	ulong nddate, ntoytime, t;
	long dt;

recalc:
	/*
	 * calculate the next dump time.
	 * minimum delay is 100 minutes.
	 */
	t = time();
	nddate = nextime(t+MINUTE(100), DUMPTIME, WEEKMASK);
	if(!conf.nodump)
		print("next dump at %T\n", nddate);

	ntoytime = time();

loop:
	dt = time() - t;
	if(dt < 0) {
		print("time went back\n");
		goto recalc;
	}
	if(dt > MINUTE(100)) {
		print("time jumped ahead\n");
		goto recalc;
	}
	t += dt;
	f = 0;

	if(t > ntoytime) {
		dt = time() - rtctime();
		if(dt < 0)
			dt = -dt;
		if(dt > 10)
			print("rtc time more than 10 secounds out\n");
		else
		if(dt > 1)
			settime(rtctime());
		ntoytime = time() + HOUR(1);
		goto loop;
	}

	if(!f) {
		if(t > nddate) {
			if(!conf.nodump) {
				print("automatic dump %T\n", t);
				for(fs=filsys; fs->name; fs++)
					if(fs->dev->type == Devcw)
						cfsdump(fs);
			}
			goto recalc;
		}
	}

	rlock(&mainlock);
	for(fs=filsys; fs->name; fs++)
		if(fs->dev->type == Devcw)
			f |= dumpblock(fs->dev);
	runlock(&mainlock);

	if(!f)
		f = dowcp();

	if(!f)
		waitsec(10000);
	wormprobe();
	goto loop;
}

/*
 * process to synch blocks
 * it puts out a block/line every second
 * it waits 10 seconds if caught up.
 * in both cases, it takes about 10 seconds
 * to get up-to-date.
 */
void
synccopy(void)
{
	int f;

loop:
	rlock(&mainlock);
	f = syncblock();
	runlock(&mainlock);
	if(!f)
		waitsec(10000);
	else
		waitsec(1000);
/*	pokewcp();	*/
	goto loop;
}
