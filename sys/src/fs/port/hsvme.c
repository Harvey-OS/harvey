#include	"all.h"
#include	"dk.h"
#include	"io.h"

/*
 *  initialize the urp transmitter and go into INITING state
 *	called with up locked
 */
void
urpxinit(Chan *up, int wins)
{
	up->dkp.seq = 0;
	up->dkp.rej = 8;
	up->dkp.maxout = 3;		/* default */
	up->dkp.mlen = 256-4;		/* default */
	if(wins >= 16) {
		if(wins >= 16*up->dkp.maxout) {
			wins = (wins/(up->dkp.maxout+1))-4;
			if(wins < up->dkp.mlen)
				up->dkp.mlen = wins;
		} else {
			up->dkp.maxout = 1;
			up->dkp.mlen = wins-4;
		}
		if(up->dkp.mlen > 1010)	/* hsvme protection */
			up->dkp.mlen = 1010;
	}

	CPRINT("dk %d: xINIT1 urpxinit mlen/maxout=%d,%d\n", up->dkp.cno,
		up->dkp.mlen, up->dkp.maxout);
	up->dkp.urpstate = INITING;
	up->dkp.timeout = DKTIME(20);
	dkreply(up, CINIT1, 0);
}

/*
 *  initialize urp receiver
 *	called with up->dkp locked
 */
void
urprinit(Chan *up)
{
	up->dkp.gcc = 0;
	up->dkp.icc = 0;
	up->dkp.bot = 0;
	up->dkp.ack = 0;
	up->dkp.ec = CECHO0;
}

/*
 *  initialize urp xmitter and receiver
 *	called with up->dkp locked
 */
void
urpinit(Chan *up)
{
	urprinit(up);
	urpxinit(up, 0);
}

/*
 * output process for getting filesystem msgs
 *	pack them up into up and send to dk output
 */
void
dkoutput(void)
{
	Chan *up;
	Msgbuf *mb;
	Dk *dk;

	dk = getarg();
	print("dk output %d\n", dk-dkctlr);

loop:
	mb = recv(dk->reply, 0);
	if(mb == 0) {
		print("dkoutput: null\n");
		goto loop;
	}
	up = mb->chan;
	dklock(dk, up);
	dkqmesg(mb, up);
	dkunlock(dk, up);
	goto loop;
}

/*
 * work part of dkoutput
 *	argument is a message that is to
 *	be output. put on queue and
 *	flag the poke process.
 *	called with up locked
 */
void
dkqmesg(Msgbuf *mb, Chan *up)
{
	Msgbuf *mb1;

	DPRINT("dkoutput: %d %d bytes\n",
		up->dkp.cno, mb->count);
	mb->next = 0;
	mb1 = up->dkp.outmsg;
	if(mb1) {
		/*
		 * if already there,
		 * queue on end and let poke
		 * process figure it out
		 */
		while(mb1->next)
			mb1 = mb1->next;
		mb1->next = mb;
	} else {
		up->dkp.outmsg = mb;
		up->dkp.nout = 0;
		up->dkp.mout = 0;
		up->dkp.mcount = mb->count;
	}
	up->dkp.pokeflg = 1;
}

/*
 * output process for poking the dk output driver
 *	messages are channel pointers to decypher
 */
int
halfempty(void *arg)
{
	Hsdev *hsd;

	hsd = arg;
	return hsd->csr & XHF;
}

void
dkpoke(void)
{
	Chan *up;
	Msgbuf *mb;
	Dk *dk;
	Hsdev *hsd;
	char *b;
	int c, n, bot, seq, tot;

	dk = getarg();
	print("dk poke %d\n", dk-dkctlr);

	hsd = dk->vme->address;
	goto loop1;

loop:
	if(tot)
		hsd->data = TXEOD;
	dkunlock(dk, up);

loop1:
	up = recv(dk->poke, 0);
	if(up == 0) {
		print("dkwrite: null\n");
		goto loop1;
	}
	if(!(hsd->csr & XHF))
		sleep(&dk->xren, halfempty, hsd);
	dklock(dk, up);
	tot = 0;	/* number of chars since half empty */

/*
 * put out control characters
 */
	n = up->dkp.repr;
	while(n != up->dkp.repw) {
		if(tot == 0) {
			hsd->data = up->dkp.cno | CHNO;
			tot++;
		}
		hsd->data = (up->dkp.repchar[n] & 0xff) | CTL;
		tot++;
		n++;
		if(n >= sizeof(up->dkp.repchar))
			n = 0;
	}
	up->dkp.repr = n;
	/*
	 *  don't send any messages until the channel
	 *  is inited
	 */
	if(up->dkp.urpstate != INITED) {
		DPRINT("dkpoke %d no output urpstate=%d\n",
			up->dkp.cno, up->dkp.urpstate);
		goto loop;
	}

more:
	if(up->dkp.nout >= up->dkp.maxout)
		goto loop;
	n = up->dkp.mout * up->dkp.mlen;
	c = up->dkp.mcount - n;
	mb = up->dkp.outmsg;
	if(c <= 0) {
		if(up->dkp.nout == 0) {
			up->dkp.seq += up->dkp.mout;
			up->dkp.mcount = 0;
			up->dkp.mout = 0;
			if(mb) {
				mb = mb->next;
				mbfree(up->dkp.outmsg);
				up->dkp.outmsg = mb;
				if(mb)
					up->dkp.mcount = mb->count;
				goto more;
			}
		}
		goto loop;
	}
	if(mb == 0) {
		print("dk %d: work, no buffer\n");
		up->dkp.mcount = 0;
		up->dkp.mout = 0;
		up->dkp.nout = 0;
		goto loop;
	}
	b = mb->data + n;
	bot = CTL|CBOT;
	if(c > up->dkp.mlen) {
		c = up->dkp.mlen;
		bot = CTL|CBOTM;
	}

	if(tot+c >= 1018) {
		if(tot != 0)
			hsd->data = TXEOD;
		dkunlock(dk, up);
		if(!(hsd->csr & XHF))
			sleep(&dk->xren, halfempty, hsd);
		dklock(dk, up);
		tot = 0;
		goto more;
	}

	if(tot == 0) {
		hsd->data = up->dkp.cno | CHNO;
		tot++;
	}
	n = c;
	while(n > 0) {
		hsd->data = *b++ & 0xff;
		n--;
	}
	tot += c;
	seq = CSEQ0 + ((up->dkp.seq + up->dkp.mout + 1) & 7);
	CPRINT("dk %d: %d bytes xSEQ%d\n", up->dkp.cno, c, seq&7);
	hsd->data = bot;
	hsd->data = c & 0xff;
	hsd->data = (c >> 8) & 0xff;
	hsd->data = CTL|seq;
	tot += 4;
	dk->orate.count += c;
	up->dkp.nout++;
	up->dkp.mout++;
	up->dkp.timeout = DKTIME(20);
	goto more;
}

/*
 * the ONE interrupt
 */
void
hsvmeintr(Vmedevice *vp)
{
	Hsdev *hsd;
	Dk *dk;
	int csr;

	hsd = vp->address;
	dk = vp->private;
	csr = hsd->csr;
	if(csr & REF)
		wakeup(&dk->rren);
	if(csr & XHF)
		wakeup(&dk->xren);
}

int
hsvmeinit(Vmedevice *vp)
{
	Dk *dk;
	Hsdev *hsd;
	ushort csr;

	print("hsvme init %d\n", vp->ctlr);
	if(vp->ctlr >= MaxDk)
		return -1;
	dk = &dkctlr[vp->ctlr];
	if(dk->vme)
		return -1;
	hsd = vp->address;

	/*
	 * does the board exist?
	if (probe(&hsd->csr, sizeof(hsd->csr)))
		return -1;
	 */

	hsd->csr = RESET;
	wbflush();
	delay(20);

	csr = hsd->csr;
	USED(csr);
	hsd->vector = vp->vector;
	hsd->csr = NORESET|IPL(vp->irq)|IENABLE|ALIVE;
	wbflush();
	delay(1);

	csr = hsd->csr;
	USED(csr);
	hsd->csr = NORESET|IPL(vp->irq)|FORCEW|IENABLE|ALIVE;
	wbflush();
	delay(1);

	dk->vme = vp;
	vp->private = dk;
	
	return 0;
}

/*
 * receiver loop process
 */
int
notempty(void *arg)
{
	Hsdev *dk;

	dk = arg;
	return dk->csr & REF;
}

void
dkinput(void)
{
	Dk *dk;
	Hsdev *hsd;
	Chan *up;
	Msgbuf *mb;
	int c, i, x;

	dk = getarg();
	print("dk input %d\n", dk-dkctlr);
	hsd = dk->vme->address;

	up = 0;		/* 0 means no channo found */

rint0:
	sleep(&dk->rren, notempty, hsd);

rint1:
	c = hsd->data;

rint2:
	if(c == 0xffff)
		goto rint0;

	/*
	 * channel number byte
	 */
	if(c & CHNO) {
		i = c & 0x1ff;
		if(i >= NDK) {
/*			print("dkinput: %d %x chan out of range\n", i, c); /**/
			up = 0;
			goto rint1;
		}
		up = &dk->dkchan[i];
		goto rint1;
	}
	if(up == 0) {
		DPRINT("dkinput: no chan\n");
		goto rint1;
	}

	dklock(dk, up);

	/*
	 * data byte(s)
	 */
	if(!(c & NND)) {
		mb = up->dkp.inmsg;
		if(mb == 0) {
			mb = mballoc(LARGEBUF, up, Mbdkinput);
			up->dkp.inmsg = mb;
			up->dkp.icc = 0;
			up->dkp.gcc = 0;
		}
		i = up->dkp.icc;
		if(i >= LARGEBUF-5) {		/* overflow */
			print("dkinput %d: input overflow\n", up->dkp.cno);
			i = up->dkp.gcc;
			up->dkp.icc = i;
		}
		for(;;) {
			mb->data[i] = c;
			i++;
			c = hsd->data;
			if(c & (NND|CHNO))
				break;
			if(i >= LARGEBUF-5)
				break;
		}
		up->dkp.icc = i;
		dkunlock(dk, up);
		goto rint2;
	}

	/*
	 * control byte
	 */
	c &= 0xff;
	switch(c) {

	default:		/* characters to ignore */
		break;

	case CECHO0+0:	case CECHO0+4:
	case CECHO0+1:	case CECHO0+5:
	case CECHO0+2:	case CECHO0+6:
	case CECHO0+3:	case CECHO0+7:
		up->dkp.timeout = DKTIME(4);
		up->dkp.rej = 8;
		c &= 7;
		for(i=up->dkp.nout-1; i>=0; i--) {
			if(c == ((up->dkp.seq + up->dkp.mout - i) & 7)) {
				CPRINT("dk %d: rECHO%d\n",
						up->dkp.cno, c);
				up->dkp.nout = i;
				up->dkp.pokeflg = 1;
				goto brk;
			}
		}
		CPRINT("dk %d: rECHO%d, window = (%d,%d]\n",
			up->dkp.cno, c,
			(up->dkp.seq+up->dkp.mout-up->dkp.nout)&7,
			(up->dkp.seq+up->dkp.mout)&7);
		break;

	case CREJ0+0:	case CREJ0+4:
	case CREJ0+1:	case CREJ0+5:
	case CREJ0+2:	case CREJ0+6:
	case CREJ0+3:	case CREJ0+7:
		up->dkp.timeout = DKTIME(4);
		c &= 7;
		if(up->dkp.rej == 8) {
			for(i=up->dkp.nout; i>0; i--) {
				if(c == ((up->dkp.seq + up->dkp.mout - i) & 7)) {
					CPRINT("dk %d: rREJ%d\n",
						up->dkp.cno, c);
					up->dkp.mout -= i;
					up->dkp.nout = 0;
					up->dkp.pokeflg = 1;
					goto brk;
				}
			}
			CPRINT("dk %d: rREJ%d, window = (%d,%d)\n",
				up->dkp.cno, c,
				(up->dkp.seq+up->dkp.mout-up->dkp.nout)&7,
				(up->dkp.seq+up->dkp.mout)&7);
		} else
			CPRINT("dk %d: rREJ%d not following rECHO or xENQ\n",
				up->dkp.cno, c);
		up->dkp.rej = c;
		break;

	case CACK0+0:	case CACK0+4:
	case CACK0+1:	case CACK0+5:
	case CACK0+2:	case CACK0+6:
	case CACK0+3:	case CACK0+7:
		up->dkp.timeout = DKTIME(4);
		up->dkp.rej = 8;
		c &= 7;
		for(i=up->dkp.nout; i>0; i--) {
			if(c == ((up->dkp.seq + up->dkp.mout - i) & 7)) {
				CPRINT("dk %d: rACK%d\n",
					up->dkp.cno, c);
				up->dkp.mout -= i;
				up->dkp.nout = 0;
				up->dkp.pokeflg = 1;
				goto brk;
			}
		}
		CPRINT("dk %d: rACK%d, window = (%d,%d]\n",
			up->dkp.cno, c,
			(up->dkp.seq+up->dkp.mout-up->dkp.nout)&7,
			(up->dkp.seq+up->dkp.mout)&7);
		break;

	case CBOT:
	case CBOTS:
	case CBOTM:
		up->dkp.bot = up->dkp.icc;
		up->dkp.last = c;
		break;

	case CENQ:
		if(CEBUG)
		switch(up->dkp.ec) {
		case CECHO0+0:	case CECHO0+4:
		case CECHO0+1:	case CECHO0+5:
		case CECHO0+2:	case CECHO0+6:
		case CECHO0+3:	case CECHO0+7:
			CPRINT("dk %d: rENQ xECHO%d xACK%d\n",
				up->dkp.cno,
				up->dkp.ec-CECHO0, up->dkp.ack);
			break;

		case CREJ0+0:	case CREJ0+4:
		case CREJ0+1:	case CREJ0+5:
		case CREJ0+2:	case CREJ0+6:
		case CREJ0+3:	case CREJ0+7:
			CPRINT("dk %d: rENQ xECHO%d xACK%d\n",
				up->dkp.cno, up->dkp.ec-CREJ0, up->dkp.ack);
			break;

		default:
			CPRINT("dk %d: rENQ xGOK%d xACK%d\n",
				up->dkp.cno, up->dkp.ec, up->dkp.ack);
		}
		up->dkp.bot = 0;
		up->dkp.icc = up->dkp.gcc;
		dkreply(up, up->dkp.ec, up->dkp.ack+CACK0);
		break;

	case CINIT1:
		urprinit(up);
		CPRINT("dk %d: rINIT1 xAINIT\n",
			up->dkp.cno);
		dkreply(up, CAINIT, 0);
		break;

	case CAINIT:
		CPRINT("dk %d: rAINIT\n",
			up->dkp.cno);
		up->dkp.urpstate = INITED;
		up->dkp.pokeflg = 1;
		break;

	case CINITREQ:
		urpxinit(up, 0);
		break;

	case CSEQ0+0:	case CSEQ0+4:
	case CSEQ0+1:	case CSEQ0+5:
	case CSEQ0+2:	case CSEQ0+6:
	case CSEQ0+3:	case CSEQ0+7:
		if(up->dkp.dkstate != OPENED) {
			/* don't accept messages on any disconnected channel */
			up->dkp.icc = up->dkp.gcc;
			up->dkp.bot = 0;
			break;
		}
		c &= 7;
		i = up->dkp.icc-2;
		if(i != up->dkp.bot){
			x = 1;
			goto bad;
		}
		mb = up->dkp.inmsg;
		if(mb == 0){
			x = 2;
			goto bad;
		}
		x = i - up->dkp.gcc;
		if((((mb->data[i+1]&0xff) << 8) | (mb->data[i+0]&0xff)) != x){
			x = 3;
			goto bad;
		}
		if(up->dkp.last != CBOTS)
		if(c != ((up->dkp.ack+1)&7)){
			x = 4;
			goto bad;
		}
		up->dkp.icc = i;
		up->dkp.gcc = i;
		up->dkp.bot = 0;
		up->dkp.ack = c;
		CPRINT("dk %d: rSEQ%d xECHO%d count=%d last=%d\n",
			up->dkp.cno, c, c, i, up->dkp.last);
		up->dkp.ec = CECHO0 + c;
		dkreply(up, up->dkp.ec, 0);
		if(up->dkp.last != CBOTM) {
			dk->irate.count += i;
			mb->count = i;
			up->dkp.inmsg = 0;
			dkunlock(dk, up);
			send(up->send, mb);	/* full message */
			goto rint1;
		}
		break;

	bad:
		up->dkp.icc = up->dkp.gcc;
		up->dkp.bot = 0;
		CPRINT("dk %d: rSEQ%d xREJ%d reason %d\n",
			up->dkp.cno, c, up->dkp.ack, x);
		up->dkp.ec = CREJ0 + up->dkp.ack;
		dkreply(up, up->dkp.ec, 0);
		break;
	}
brk:
	dkunlock(dk, up);
	goto rint1;
}

/*
 * put control characters in circular
 * buffer for output routine, then poke
 *	up is locked
 */
void
dkreply(Chan *up, int c1, int c2)
{
	int i;

	i = up->dkp.repw;
	if(i < 0 || i >= sizeof(up->dkp.repchar)) {
		i = 0;
		up->dkp.repw = 0;
		up->dkp.repr = 0;
	}
	if(c1) {
		up->dkp.repchar[i] = c1;
		i++;
		if(i >= sizeof(up->dkp.repchar))
			i = 0;
	}
	if(c2) {
		up->dkp.repchar[i] = c2;
		i++;
		if(i >= sizeof(up->dkp.repchar))
			i = 0;
	}
	up->dkp.repw = i;
	up->dkp.pokeflg = 1;
}
