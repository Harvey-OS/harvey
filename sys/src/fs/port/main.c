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
	conf.nmach = 2;
	conf.nproc = 45;

	conf.mem = meminit();
	conf.sparemem = conf.mem/12;		/* 8% spare for chk etc */

	conf.nalarm = 200;
	conf.nuid = 1000;
	conf.nserve = 15;
	conf.nrahead = 3;
	conf.nfile = 30000;
	conf.nlgmsg = 100;
	conf.nsmmsg = 500;
	conf.wcpsize = 1024*1024;

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
void
rahead(void)
{
	Rabuf *rb;
	Iobuf *p;

loop:
	rb = recv(raheadq, 0);
	if(rb == 0)
		goto loop;
	p = getbuf(rb->dev, rb->addr, Bread);
	if(p)
		putbuf(p);

	lock(&rabuflock);
	rb->link = rabuffree;
	rabuffree = rb;
	unlock(&rabuflock);
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
	cons.work.count++;
	cons.rate.count += mb->count;
	fo.err = 0;

	(*p9call[t])(cp, &fi, &fo);

	fo.type = t+1;
	fo.tag = fi.tag;

	if(fo.err) {
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
	cons.rate.count += n;
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
	duartreset();
	firmware();
}

/*
 * process to copy dump blocks from
 * cache to worm. it runs flat out when
 * it gets work, but only looks for
 * work every 10 seconds.
 */
static	Rendez	wcp;

static
void
callwc(Alarm *a, void *arg)
{

	USED(arg);
	cancel(a);
	wakeup(&wcp);
}

#define	DUMPTIME	5	/* 5 am */
#define	WEEKMASK	0	/* every day (1=sun, 2=mon 4=tue) */

void
wormcopy(void)
{
	int f;
	Filsys *fs;
	long nddate, t, dt;

recalc:
	/*
	 * calculate the next dump time.
	 * minimum delay is 100 minutes.
	 */
	t = time();
	nddate = nextime(t+MINUTE(100), DUMPTIME, WEEKMASK);
	print("next dump at %T\n", nddate);

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

	if(!f) {
		if(t > nddate) {
			print("automatic dump %T\n", t);
			for(fs=filsys; fs->name; fs++)
				if(fs->dev.type == Devcw)
					cfsdump(fs);
			goto recalc;
		}
	}

	rlock(&mainlock);
	for(fs=filsys; fs->name; fs++)
		if(fs->dev.type == Devcw)
			f |= dumpblock(fs->dev);
	runlock(&mainlock);

	if(!f)
		f = dowcp();

	if(!f)
		f = dowcmp();

	if(!f) {
		alarm(10000, callwc, 0);
		sleep(&wcp, no, 0);
	}

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
static	Rendez	scp;

static
void
callsc(Alarm *a, void *arg)
{

	USED(arg);
	cancel(a);
	wakeup(&scp);
}

void
synccopy(void)
{
	int f;

loop:
	rlock(&mainlock);
	f = syncblock();
	runlock(&mainlock);
	alarm(f?1000: 10000, callsc, 0);
	sleep(&scp, no, 0);
/*	pokewcp();	*/
	goto loop;
}
