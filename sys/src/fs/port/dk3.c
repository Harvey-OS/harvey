#include	"all.h"
#include	"dk.h"

/*
 * poll process
 */
void
calldk(Alarm *a, Dk *dk)
{
	cancel(a);
	wakeup(&dk->dkto);
}

void
dktimer(void)
{

	long t, toy;
	Chan *up;
	Dk *dk;

	dk = getarg();
	print("dk local %d\n", dk-dkctlr);

	cccinit(dk);
	toy = 0;
	memset(dk->timeout, 0, sizeof(dk->timeout));
	t = toytime();
	dk->timeout[DKATO] = t + DKATIME;
	dk->timeout[DKBTO] = t + DKBTIME;
	dk->timeout[TIMETO] = t + SECOND(20);

loop:
	alarm(300, calldk, dk);	/* 3 times/sec */
	sleep(&dk->dkto, no, 0);

	/*
	 * general timeouts
	 */
	t = toytime();
	if(t != toy) {
		if(t < toy)		/* decreasing */
			memset(dk->timeout, 0, sizeof(dk->timeout));
		toy = t;
		if(t >= dk->timeout[DKATO]) {
			dk_timer(dk);
			dk->timeout[DKATO] = t + DKATIME;
		}
		if(t >= dk->timeout[DKBTO]) {
			/*
			 *  if the service channel is closed, reannounce
			 */
			up = &dk->dkchan[SRCHAN];
			dklock(dk, up);
			if(up->dkp.dkstate == CLOSED)
				xpcall(up, service, 1);
			dkunlock(dk, up);

			/*
			 *  send a keep alive on the signalling channel
			 */
			up = &dk->dkchan[CCCHAN];
			dklock(dk, up);
			xpmesg(up, T_ALIVE, D_STILL, 0, 0);
			dkunlock(dk, up);

			dk->timeout[DKBTO] = t + DKBTIME;
		}
		if(t >= dk->timeout[TIMETO]) {
			/*
			 *  if clock is closed, redial
			 */
			up = &dk->dkchan[CKCHAN];
			dklock(dk, up);
			if(up->dkp.dkstate == CLOSED) {
				if(0)
					xpcall(up, "nj/mhe/mhpbs.clock\nrfs\n", 0);
				else
					xpcall(up, "nj/astro/clock\nrfs\n", 0);
			}
			dkunlock(dk, up);

			dk->timeout[TIMETO] = t + TIMEQTIME;
			if(dk->anytime)
				dk->timeout[TIMETO] = t + TIMENTIME;
		}
	}
	goto loop;
}

void
dk_timer(Dk *dk)
{
	int i;
	Chan *up;

	/*
	 * called every 2 second
	 * thus the DKTIME macro is n/2
	 */
	for(i=0; i<NDK; i++) {
		up = &dk->dkchan[i];
		dklock(dk, up);
		/*
		 *  dialup timeout
		 */
		if(up->dkp.dkstate == DIALING){
			up->dkp.calltimeout--;
			if(up->dkp.calltimeout <= 0)
				xphup(up);
		}
		/*
		 *  all other timeouts
		 */
		up->dkp.timeout--;
		if(up->dkp.timeout > 0) {
			dkunlock(dk, up);
			continue;
		}
		switch(up->dkp.dkstate) {
		case LCLOSE:
			/*
			 *  No ISCLOSED received.
			 *  remind dk of channels that we've closed
			 */
			up->dkp.timeout = DKTIME(20);
			xpmesg(up, T_CHG, D_CLOSE, up->dkp.cno, 0);
			break;
		case DIALING:
		case OPENED:
			/*
			 *  NO AINIT received
			 *  resend INIT1
			 */
			if(up->dkp.urpstate == INITING) {
				CPRINT("dk %d: xINIT1 dk_timer\n", up->dkp.cno);
				up->dkp.timeout = DKTIME(20);
				dkreply(up, CINIT1, 0);
				break;
			}
			/*
			 *  No acknowledges
			 *  send ENQ to get state of receiver
			 */
			if(up->dkp.nout != 0) {
				CPRINT("dk %d: xENQ, window = (%d,%d]\n",
					up->dkp.cno,
					(up->dkp.seq+up->dkp.mout-up->dkp.nout)&7,
					(up->dkp.seq+up->dkp.mout)&7);
				up->dkp.timeout = DKTIME(4);
				dkreply(up, CENQ, 0);
				break;
			}
			break;
		}
		dkunlock(dk, up);
	}
}

/*
 * local (non-filesystem) messages 
 */
void
dklocal(void)
{
	Chan *up;
	Msgbuf *mb;
	long t;
	Dk *dk;

	dk = getarg();
	print("dk local %d\n", dk-dkctlr);

loop:
	mb = recv(dk->local, 0);
	if(mb == 0) {
		print("dklocal: null\n");
		goto loop;
	}
	up = mb->chan;

	switch(up->dkp.cno) {
	case CCCHAN:
		xplisten(dk, mb->data, mb->count);
		break;

	case SRCHAN:
		srlisten(dk, mb->data, mb->count);
		break;

	case CKCHAN:
		t = readclock(mb->data, mb->count);
		if(t) {
			settime(t);
			setrtc(t);
			prdate();
			dk->anytime = 1;
		}
		break;
	}

	mbfree(mb);
	goto loop;

}
