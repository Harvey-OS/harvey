#include	"all.h"
#include	"mem.h"
#include	"io.h"
#include	"ureg.h"

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
		authreply = newqueue(Nqueue);

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
	Fcall fi, fo;
	Msgbuf *mb, *mb1;
	Chan *cp;
	int t, n;

loop:
	qlock(&reflock);
	mb = recv(serveq, 0);
	cp = mb->chan;
	rlock(&cp->reflock);
	qunlock(&reflock);

	rlock(&mainlock);

	/*
	 * conversion to structure and
	 * simple syntax checks.
	 */
	if(convM2S(mb->data, &fi, mb->count) == 0) {
		print("bad M2S conversion\n");
		goto error;
	}
	t = fi.type;

	if(t < 0 || t >= MAXSYSCALL || (t&1) || !p9call[t]) {
		print("bad message type\n");
		goto error;
	}

	/*
	 * allocate reply message
	 */
	if(t == Tread) {
		mb1 = mballoc(MAXMSG+MAXDAT, cp, Mbreply2);
		fo.data = mb1->data + 8;
	} else
		mb1 = mballoc(MAXMSG, cp, Mbreply3);

	/*
	 * call the file system
	 */
	cons.work[0].count++;
	cons.work[1].count++;
	cons.work[2].count++;
	cp->work.count++;
	cons.rate[0].count += mb->count;
	cons.rate[1].count += mb->count;
	cons.rate[2].count += mb->count;
	cp->rate.count += mb->count;
	fo.err = 0;

	(*p9call[t])(cp, &fi, &fo);

	fo.type = t+1;
	fo.tag = fi.tag;

	if(fo.err) {
		if(cons.flags&errorflag)
			print("	type %d: error: %s\n", t,
				errstr[fo.err]);
		if(CHAT(cp))
			print("	error: %s\n", errstr[fo.err]);
		fo.type = Rerror;
		strncpy(fo.ename, errstr[fo.err], sizeof(fo.ename));
	}

	n = convS2M(&fo, mb1->data);
	if(n == 0) {
		print("bad S2M conversion\n");
		mbfree(mb1);
		goto error;
	}
	mb1->count = n;
	mb1->param = mb->param;
	cons.rate[0].count += n;
	cons.rate[1].count += n;
	cons.rate[2].count += n;
	cp->rate.count += n;
	send(cp->reply, mb1);

out:
	mbfree(mb);
	runlock(&mainlock);
	runlock(&cp->reflock);
	goto loop;

error:
	print("type=%d count=%d\n", mb->data[0], mb->count);
	print(" %.2x %.2x %.2x %.2x\n",
		mb->data[1]&0xff, mb->data[2]&0xff,
		mb->data[3]&0xff, mb->data[4]&0xff);
	print(" %.2x %.2x %.2x %.2x\n",
		mb->data[5]&0xff, mb->data[6]&0xff,
		mb->data[7]&0xff, mb->data[8]&0xff);
	print(" %.2x %.2x %.2x %.2x\n",
		mb->data[9]&0xff, mb->data[10]&0xff,
		mb->data[11]&0xff, mb->data[12]&0xff);

	mb1 = mballoc(3, cp, Mbreply4);
	mb1->data[0] = Rnop;	/* your nop was ok */
	mb1->data[1] = ~0;
	mb1->data[2] = ~0;
	mb1->count = 3;
	mb1->param = mb->param;
	send(cp->reply, mb1);
	goto out;
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

	ntoytime = time() + HOUR(1);

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
