#include	<u.h>
#include	"comm.h"
#include	"all.h"
#include	"fns.h"
#include	"io.h"

enum
{
	/* NB. These addresses are known to the rom ! */
	SystemResetpc	= 0xF0C00048,	
	SystemPrcb	= 0xFFFFFF30,
	Resetreq	= 3<<8,
};

void
main(void)
{
	extern char edata[], end[];
	int i;

	memset(edata, 0, end-edata);

	duartinit();
	
	memset(COMM, 0, sizeof(Comm));
	COMM->vmevec = 0xd2;	/* default */

	print("Cyclone VME-fibre.\nversion %d A32 space\n", DATE);
	print("end=0x%lux com=0x%lux buf=0x%d\n", end, COMM, NBUF);

	for(i=0; i<NBUF; i++) {
		b.mesg[i].next = b.freebuflist;
		b.freebuflist = &b.mesg[i];
	}

	initvme();
	initsql();

	/*
	 * scan
	 */
	for(;;) {
		runvme();
		runsql();
		chkabort();
	}
}

/*
 * exits - reinitialise the board
 */
void
exits(char *s)
{
	int i;

	print("exits: %s\n", s);

	i = COMM->vmevec;
	memset(COMM, 0, sizeof(Comm));
	COMM->vmevec = i;

	for(i=0; i<NBUF; i++) {
		b.mesg[i].next = b.freebuflist;
		b.freebuflist = &b.mesg[i];
	}
	return;

	sysctl(Resetreq, SystemResetpc, SystemPrcb);
}
	
/*
 * work from vme
 */
void
runvme(void)
{
	Mesg *m;
	ulong u, s, p, fu;
	long i, c;

loop:
	i = COMM->reqstp;
	if(i < 0 || i >= NRQ) {
		print("vme q sanity q=%d\n", i);
		COMM->reqstp = 0;
		goto loop;
	}
	u = COMM->reqstq[i];
	if(u == 0) {
		if(b.pingpend) {
			while(FIFOXMTBLK)
				runsql();
			XMIT(Upingreply + (2<<16));
			XMIT(b.pingvalue);
			b.pingpend = 0;
		}
		return;
	}
	while(FIFOXMTBLK)
		runsql();

	u = swapl(u);
	i = vmefetch(u);

	switch(i) {
	default:
		print("unknown vme command 0x%lux\n", i);
		break;

	case Ureset:
		/* vmeUreset */
		print("v reset u=%.8lux VEC %.2lux\n", u, COMM->vmevec);

		memset(COMM->replyq, 0, sizeof(COMM->replyq));
		COMM->replyp = 0;
		for(i=0; i<2; i++) {
			XMIT(Ureset + (1<<16));
		}

		/* BOTCH -- DO MORE */
		break;

	case Ureboot:
		/* vmeUreboot */

		exits("v reboot");
		return;

	case Uwrite:
		/* vmeUwrite; xbufp; xbsize; csum */
		p = vmefetch(u + 1*HPP);
		c = vmefetch(u + 2*HPP);
		s = vmefetch(u + 3*HPP);
		DEBUG("v write u=%.8lux p=%.8lux c=%ld s=%.8lux\n", u, p, c, s);

		m = b.freebuflist;
		if(m == 0) {
			print("v write: out of buffers\n");
			break;
		}
		b.freebuflist = m->next;

		m->type = Uwrite;
		m->haddr = u;
		m->bufp = p;
		m->bsize = c;
		m->csum = s;

		m->next = b.wbuflist;
		b.wbuflist = m;
		break;

	case Uread:
		/* vmeUread; rbufp; rbsize; <rbcount; <csum */
		p = vmefetch(u + 1*HPP);
		c = vmefetch(u + 2*HPP);
		DEBUG("v read u=%.8lux p=%.8lux c=%ld\n", u, p, c);

		for(m=b.wbuflist; m; m=m->next)
			if(m->haddr == u)
				goto rfound;

		/* botch do more */
		print("v read: buffer not found u=%.8lux\n", u);
		for(m=b.wbuflist; m; m=m->next)
			print("	%d %.8lux\n", m->type, m->haddr);
		break;

	rfound:
		if(m->type != Uwrite) {
			print("v read: buffer not write u=%.8lux\n", u);
			break;
		}
		if(c < 0 || c > MAXXMIT) {
			print("v read: write buf too big c=%d\n", c);
			break;
		}

		/*
		 * transmit:
		 *	Uwrite + (count<<16)
		 *	u
		 *	bsize
		 *	csum
		 *	buffer[0..(bsize+3)/4]
		 */
		i = (m->bsize+3) / 4;

		XMIT(Uwrite + ((i+4) << 16));
		XMIT(u);
		XMIT(m->bsize);
		XMIT(m->csum);

		vmexmit(m->bufp, i);

		m->type = Uread;
		m->bufp = p;
		m->bsize = c;
		break;

	case Ureply:
		/* vmeUreply; fu; bufp; bsize; csum */
		fu = vmefetch(u + 1*HPP);
		p = vmefetch(u + 2*HPP);
		c = vmefetch(u + 3*HPP);
		s = vmefetch(u + 4*HPP);
		DEBUG("v reply u=%.8lux fu=%.8lux p=%.8lux p[0]=%.8lux c=%ld s=%.8lux\n",
			u, fu, p, vmefetch(p), c, s);

		if(c < 0 || c >= MAXXMIT) {
			print("v reply: buf too big %d\n", c);
			break;
		}

		/*
		 * transmit:
		 *	Ureply + (count<<16)
		 *	fu
		 *	bsize
		 *	csum
		 *	buffer*(bsize+3)/4
		 */
		i = (c+3) / 4;
		XMIT(Ureply + ((i+4) << 16));
		XMIT(fu);
		XMIT(c);
		XMIT(s);

		vmexmit(p, i);

		break;

	case Ubuf:
		/* vmeUbuf; >u; bufp; bsize; >bcount; >csum */
		p = vmefetch(u + 2*HPP);
		c = vmefetch(u + 3*HPP);
		DEBUG("v buf u=%.8lux b=%.8lux c=%ld\n", u, p, c);

		m = b.freebuflist;
		if(m == 0) {
			print("v buf: out of buffers\n");
			break;
		}
		b.freebuflist = m->next;

		m->type = Ubuf;
		m->haddr = u;
		m->bufp = p;
		m->bsize = c;

		if(c >= MEDIAN) {
			m->next = b.lbuflist;
			b.lbuflist = m;
		} else {
			m->next = b.sbuflist;
			b.sbuflist = m;
		}
		break;

	case Utest:
		DEBUG("v test\n");
		break;
	}

	i = COMM->reqstp;
	COMM->reqstq[i] = 0;		/* free the rq slot */
	i++;
	if(i >= NRQ)
		i = 0;
	COMM->reqstp = i;
	goto loop;
}

/*
 * work from fiber
 * runsql is called both at
 * the top level and indirectly out
 * of runvme through vmexmit.
 * the main consequence is that
 * the fiber transmitter may be
 * in the middle of a message and
 * therefore runsql cannot transmit.
 */
void
runsql(void)
{
	Mesg *m, *om;
	ulong u, s;
	long c, t, n;

	if(FIFOE)
		return;

	if(b.vmeaddr) {
		recvvme();
		return;
	}

	RECV(c);
	t = c & 0xffff;			/* type */
	c >>= 16;			/* longs in message */

	switch(t) {
	default:
		print("unknown fiber command %d\n", t);
		goto error;

	case Ureset:
		 /* gazUreset+(count<<16); */
		if(c != 1) {
			print("g reset: botch c = %d\n", c);
			goto error;
		}
		print("g reset\n");
		break;

	case Uwrite:
		 /* gazUwrite+(count<<16); u; bsize; csum; buffer[0..(bsize+3)/4] */
		if(c > 2048+128) {
			print("g write: botch c = %d\n", c);
			goto error;
		}
		DEBUG("g write t=%d c=%d\n", t, c);

		while(FIFOE); RECV(u);
		while(FIFOE); RECV(n);
		while(FIFOE); RECV(s);

		if(n > SMALLSIZE) {
			m = b.lbuflist;
			if(m == 0) {
				print("g write: large buffer empty\n");
				goto error;
			}
			b.lbuflist = m->next;
		} else {
			m = b.sbuflist;
			if(m == 0) {
				print("g write: small buffer empty\n");
				goto error;
			}
			b.sbuflist = m->next;
		}
		if(n > m->bsize) {
			print("g write: buffer too small is %d need %d\n",
				n, m->bsize);
			goto error;
		}

		c -= 4;
		if(c != (n+3)/4) {
			print("g write: botch %d %d\n", c, n);
			goto error;
		}

		b.vmeaddr = m->bufp;
		b.vmecount = c;
		b.vmesum = 0;

		b.vmeblk.type = 1;
		b.vmeblk.haddr = m->haddr;
		b.vmeblk.u = u;
		b.vmeblk.s = s;
		b.vmeblk.n = n;

		m->next = b.freebuflist;
		b.freebuflist = m;

		if(FIFOE)
			break;

		recvvme();
		break;

	case Ureply:
		 /* gazUreply+(count<<16); u; bsize; csum; buffer[0..(bsize+3)/4] */
		if(c > 2048+128) {
			print("g write: botch c = %d\n", c);
			goto error;
		}

		while(FIFOE); RECV(u);
		while(FIFOE); RECV(n);
		while(FIFOE); RECV(s);

		om = 0;
		for(m=b.wbuflist; m; m=m->next) {
			if(m->haddr == u)
				goto rfound;
			om = m;
		}

		print("g reply: buffer not found u=%.8lux\n", u);
		for(m=b.wbuflist; m; m=m->next)
			print("	%d %.8lux\n", m->type, m->haddr);
		for(n=4; n<c; n++) {
			while(FIFOE); RECV(u);
			USED(u);
		}
		/* botch -- do more */
		break;

	rfound:
		if(om)
			om->next = m->next;
		else
			b.wbuflist = m->next;

		if(m->type != Uread) {
			print("g reply: buffer not write u=%.8lux\n", u);
			goto error;
		}
		if(m->bsize < n) {
			print("g reply: read buf too small c=%d\n", n);
			goto error;
		}

		DEBUG("g reply t=%d c=%d n=%d s=%.8lux\n", t, c, n, s);
		c -= 4;
		if(c != (n+3)/4) {
			print("g reply: botch c=%d n=%d\n", c, n);
			goto error;
		}

		b.vmeaddr = m->bufp;
		b.vmecount = c;
		b.vmesum = 0;

		b.vmeblk.type = 0;
		b.vmeblk.haddr = m->haddr;
		b.vmeblk.u = u;
		b.vmeblk.s = s;
		b.vmeblk.n = n;

		m->next = b.freebuflist;
		b.freebuflist = m;
		recvvme();
		break;

	case Uping:
		/*
		 * arrange a pingreply of the recv value
		 */
		if(c != 2) {
			print("g ping: botch c = %d\n", c);
			goto error;
		}

		while(FIFOE);
		RECV(s);

		b.pingvalue = s;
		b.pingpend = 1;
		break;

	case Upingreply:
		/*
		 * ignored
		 */
		if(c != 2) {
			print("g pingreply: botch c = %d\n", c);
			goto error;
		}
		while(FIFOE);
		RECV(s);
		USED(s);
		break;
	}
	return;

error:
	errorflush();
}

ulong
vmefetch(ulong a)
{
	ulong u;

	u = *(ulong*)((a & 0x0fffffff) | 0x40000000);
	return swapl(u);
}

void
vmestore(ulong a, ulong bb)
{
	ulong u;

	u = swapl(bb);
	*(ulong*)((a & 0x0fffffff) | 0x40000000) = u;
}

void
errorflush(void)
{
	int i, j, state;
	ulong r;


	state = 0;
	i = 0;

	print("errorflush ");
start:
	while(i<5) {
		do {
			chkabort();
		} while(FIFOXMTBLK);

		XMIT(Uping + (2<<16));
		b.pingcount++;
		XMIT(b.pingcount);

		for(j=0; j<1000;) {
			if(FIFOE) {
				for(r=0; r<1000; r++)
					;
				j++;
				continue;
			}
			RECV(r);
			if(r == (Uping + (2<<16))) {
				state = 1;
				continue;
			}
			if(r == (Upingreply + (2<<16))) {
				state = 2;
				continue;
			}
			if(state == 1) {
				XMIT(Upingreply + (2<<16));
				XMIT(r);
			}
			if(state == 2) {
				if(r == b.pingcount) {
					i++;
					goto start;
				}
			}
			state = 0;
		}
		i = 0;
	}
	print("sync\n");
}

void
delay(long ms)
{
	int i;

	ms = 5000*ms;
	for(i = 0; i < ms; i++)
		;
}

void
initvme(void)
{
	Vic *v;

	print("vic init\n");

	v = VIC;
	v->lbtr   = 0x49;	/* Local bus time */
	v->ttor   = 0xFE;	/* 512uS bus time out */
	v->iconf  = 0x44;
	v->ss0cr0 = 0x12;
	v->ss0cr1 = 0x30;
	v->ss1cr0 = 0x16;
	v->ss1cr1 = 0x30;
	v->rcr    = 0x00;
	v->amsr   = 0x8b;
	v->btdr	  = 0x02;
	v->besr   = 0;		/* Reset error status */

	/*
	 * SGI machines need AM code 9 (User Data Extentenenenended)
	 * so the processor must run in user mode 
	 */
	modpc(2, 0);		/* Clear em -> user mode */
	print("vic init done, u-mode\n");
}

void
initsql(void)
{
	Taxi *t;
	ulong i;

	t = SQL;

	t->rst1 = 0;		/* Reset taxi chips */
	delay(10);
	t->rst0 = 0;
	delay(10);
	t->rst1 = 0;
	delay(10);

	i = t->ctl;		/* Clear command bits */
	USED(i);
	i = t->csr;
	USED(i);
	t->fiforst = 0;
	print("taxi reset\n");

	i = t->fifocsr;
	if((i&Xmit_flag) == 0)
		print("taxiinit: Xmit fifo flags %lux\n", i & 0xff);

	t->txoffwr = CHUNK;	/* tx fifo threshold in words */
	/*
	 * Off by one error !
	 */
	t->rcvoffwr = CHUNK-1;
	print("taxi fifo block size %d words\n", CHUNK);
	print("csr misc =%lux\n", &t->csr);
}

void
irq5(uchar vector)
{
	VIC->irq5 = vector;
	while(VIC->irsr & (1<<5))	/* Loop until I5 is clear, so we can post */
		;
	VIC->irsr = (1<<5) | 1;		/* Post the interrupt */
}

ulong
swapl(ulong v)
{
	return	(v>>24) |
		((v>>8)&0xFF00) |
		((v<<8)&0xFF0000) |
		(v<<24);
}
