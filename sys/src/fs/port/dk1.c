#include	"all.h"
#include	"dk.h"

/*
 * called once at startup time
 */
void
dkinit(Dk *dk)
{
	Chan *up;
	int i;

/*	cons.flags |= dkflag;	/**/

	dk->dkchan = chaninit(Devdk, NDK);
	dk->poke = newqueue(NQUEUE);
	dk->reply = newqueue(NQUEUE);
	dk->local = newqueue(NQUEUE);
	dk->anytime = 0;

	for(i=0; i<NDK; i++) {
		up = &dk->dkchan[i];
		up->dkp.name = "dk";
		dklock(dk, up);
		up->dkp.cno = i;

		xpinit(up);
		switch(i) {
		case SRCHAN:
		case CCCHAN:
		case CKCHAN:
			up->send = dk->local;
			up->reply = 0;
			break;

		default:
			up->send = serveq;
			up->reply = dk->reply;
			break;
		}

		dkunlock(dk, up);
	}
	dofilter(&dk->irate);
	dofilter(&dk->orate);
	wakeup(&dk->rren);
	wakeup(&dk->xren);
}

enum
{
	Hunt,		/* looking for newline */
	Skipb,		/* skipping blanks */
	Digits		/* reading digits */
};

long
readclock(char *buf, int n)
{
	static int state, ndigits;	/* BOTCH */
	static long number;		/* BOTCH */
	int c, i;
	long v;

	v = 0;
	for(i=0; i<n; i++) {
		c = buf[i];
		switch(state) {
		case Hunt:
			if(c == '\n')
				state = Skipb;
			break;

		case Skipb:
			if(c == ' ')
				break;
			state = Digits;
			ndigits = 0;
			number = 0;

		case Digits:
			if(c >= '0' && c <= '9') {
				ndigits++;
				number = number*10 + c - '0';
				break;
			}
			state = Hunt;
			if(c =='\r' || c == '\n')
			if(ndigits >= 9)
				v = number;
			break;
		}
	}
	return v;
}

static
void
stats(Dk *dk)
{
	Chan *up;
	char flag[10], *fp;
	int i, n, l, w;
	Msgbuf *mb, *mb1;

	print("dkstats %d\n", dk-dkctlr);
	n = 0;
	for(i=0; i<NDK; i++) {
		fp = flag;
		up = &dk->dkchan[i];

		l = 1;
		if(!canqlock(&up->dkp)) {
			l = 0;
			*fp++ = 'l';
		}

		if(up->dkp.inmsg)
			*fp++ = 'r';
		if(mb = up->dkp.outmsg) {
			*fp++ = 'w';
			if(l) {
				w = 0;
				while(mb) {
					w++;
					mb = mb->next;
				}
				if(w >= 100) {
					*fp++ = '*';
					*fp++ = '*';
					mb = up->dkp.outmsg;
					up->dkp.outmsg = 0;
					while(mb) {
						mb1 = mb;
						mb = mb->next;
						mbfree(mb1);
					}
				} else {
					if(w >= 10)
						*fp = w/10 + '0';
					*fp++ = w%10 + '0';
				}
			}
		}
		if(up->dkp.dkstate == OPENED)
			n++;
		if(fp != flag) {
			*fp++ = up->dkp.dkstate+'0';
			*fp = 0;
			print("	cno=%3d %s\n",
				up->dkp.cno,
				flag);
		}
		if(i == CCCHAN || i == SRCHAN) {
			print("chan %d\n", i);
			print("	nout     %d\n", up->dkp.nout);
			print("	mout     %d\n", up->dkp.mout);
			print("	mcount   %d\n", up->dkp.mcount);
			print("	urpstate %d\n", up->dkp.urpstate);
			print("	dkstate  %d\n", up->dkp.dkstate);
			print("	timeout  %d\n", up->dkp.timeout);
			print("	outmsg   %lux\n", up->dkp.outmsg);
		}
		if(l)
			qunlock(&up->dkp);
	}
	print("	%d opened dk channels\n", n);
	print("	in  t=%F cbps\n", (Filta){&dk->irate, 100});
	print("	out t=%F cbps\n", (Filta){&dk->orate, 100});
}

static
void
cmd_statk(int argc, char *argv[])
{
	Dk *dk;

	USED(argc);
	USED(argv);

	for(dk = &dkctlr[0]; dk < &dkctlr[MaxDk]; dk++)
		if(dk->vme)
			stats(dk);
}

void
dklock(Dk *dk, Chan *up)
{

	USED(dk);
	qlock(&up->dkp);
	if(up->dkp.pokeflg)
		print("dk:%d pokeflg before start\n", up->dkp.cno);
	if(up->dkp.ccmsg)
		print("dk:%d ccmsg before start\n", up->dkp.cno);
}

void
dkunlock(Dk *dk, Chan *up)
{
	int f;
	Msgbuf *mb, *mb1;

loop:
	f = up->dkp.pokeflg;
	up->dkp.pokeflg = 0;

	mb = up->dkp.ccmsg;
	up->dkp.ccmsg = 0;

	qunlock(&up->dkp);
	if(f)
		send(dk->poke, up);

	if(mb) {
		up = &dk->dkchan[CCCHAN];
		dklock(dk, up);
		while(mb) {
			mb1 = mb->next;
			dkqmesg(mb, up);
			mb = mb1;
		}
		goto loop;
	}
		
}

void
dkreset(Dk *dk, int cno)
{
	int i;
	Chan *up;

	for(i=0; i<NDK; i++) {
		up = &dk->dkchan[i];
		if(up->chan != cno)
			continue;
		qlock(&up->dkp);
		xpinit(up);
		qunlock(&up->dkp);
	}
}

void
dkstart(void)
{
	Dk *dk;
	int any;

	any = 0;
	for(dk = &dkctlr[0]; dk < &dkctlr[MaxDk]; dk++)
		if(dk->vme) {
			dkinit(dk);
			userinit(dkinput, dk, "dki");
			userinit(dkoutput, dk, "dko");
			userinit(dkpoke, dk, "dkp");
			userinit(dktimer, dk, "dkt");
			userinit(dklocal, dk, "dkl");

			cmd_install("statk", "-- dk stats", cmd_statk);
			dkflag = flag_install("dkit", "-- datakit noise");
			any = 1;
		}
	USED(any);
}
