#include <alef.h>
#include <fcall.h>
#include <libg.h>
#include <frame.h>
#include "y.h"

byte backgrnd[] = {
	0xEE, 0xEE, 0xBB, 0xBB, 0xEE, 0xEE, 0xBB, 0xBB,
	0xEE, 0xEE, 0xBB, 0xBB, 0xEE, 0xEE, 0xBB, 0xBB,
	0xEE, 0xEE, 0xBB, 0xBB, 0xEE, 0xEE, 0xBB, 0xBB,
	0xEE, 0xEE, 0xBB, 0xBB, 0xEE, 0xEE, 0xBB, 0xBB,
};

intern chan(Mesg) mreq, kreq;
intern Point mpos;
intern Cursor *mgc;
intern Rectangle rz;

void
main(int argc, byte **argv)
{
	Mesg r;
	byte *f;
	chan(Mesg) c;

	f = nil;
	if(argc > 1) {
		f = argv[1];
		putenv("font", f);
	}

	alloc mreq, kreq, wmgr, dispatch;

	binit(error, f, "Y");
	bgrnd = balloc(Rect(0, 0, 16, 16), 0);
	if(bgrnd == nil)
		error("no bitmap space");

	wrbitmap(bgrnd, 0, 16, backgrnd);
	texture(&screen, screen.r, bgrnd, S);

	ncur = balloc(Rect(0, 0, 32, 32), 0);
	if(ncur == nil)
		error("no bitmap space");
	f = malloc(128);
	f[0] = 0x80;
	wrbitmap(ncur, 0, 16, f);
	free(f);

	setsub();
	bflush();

	proc mouse(mreq);
	proc keybd(kreq);
	proc files();
	task dispatcher();
	task winmgr();

	cut  = newbuf(Cbsize);
	cut2 = newbuf(Cbsize);

	for(;;) {
		sweeper(On);
		cuset(&r);
		bflush();
		alt {
		case r = <-mreq:
			sweeper(Off);
			if(r.buttons == 0)
				grab = nil;
			else if(wtrls)
				break;
			wtrls = 0;

			if(mgrstate != 0 || whead == nil) {
				wmgr <-= r;
				break;
			}

			c = whead->in;
			if(grab != nil)
				c = grab->in;

			c <-= r;
			break;

		case r = <-kreq:
			sweeper(Off);
			if(wcurr)
				wcurr->in <-= r;
			break;
		}
	}
}

void
dispatcher()
{
	Mesg m;
	Rlist *r;

	for(;;) {
		m = <-dispatch;
		if(rpend) {
			for(r = m.refresh; r->next; r = r->next)
				;
			r->next = rpend;
			rpend = nil;
		}
		for(;;) {
			if(whead)
				whead->in <-= m;
			else
				wmgr <-= m;

			if(rpend == nil)
				break;

			m.refresh = rpend;
			rpend = nil;
		}
	}
}

void
cuset(Mesg *r)
{
	Cursor *c;
	Window *w;

	if(r != nil && r->type == MsgMouse)
		mpos = r->xy;

	c = nil;
	w = wcurr;
	if(mgrcurs)
		c = mgrcurs;
	else
	if(w != nil && w->dcursor == 0 && ptinrect(mpos, w->win)) {
		c = wcurr;
		if(w->chgcurs) {
			mgc = nil;
			w->chgcurs = 0;
		}
	}

	if(c != mgc) {
		cursorswitch(c);
		mgc = c;
	}
}

void
sweeper(int k)
{
	if(sweep.doit == 0)
		return;

	switch(k) {
	case On:
		border(&screen, sweep.on, 2, notD);
		sweep.off = sweep.on;
		break;
	case Off:
		border(&screen, sweep.off, 2, notD);
		sweep.off = rz;
		break;
	}
}

void
keybd(chan(Mesg) r)
{
	Mesg m;
	byte *p, buf[128];
	int fd, fdc, n, w, kbdcnt;

	fd = open("/dev/cons", OREAD);
	if(fd < 0)
		error("cons open: %r");

	fdc = open("/dev/consctl", OWRITE);
	if(fdc < 0)
		error("consctl open: %r");
	write(fdc, "rawon", 5);

	m.type = MsgKeybd;
	p = buf;
	kbdcnt = 0;
	for(;;) {
		n = read(fd, p, sizeof(buf)-kbdcnt);
		if(n < 0)
			break;
		p += n;
		kbdcnt += n;
		while(kbdcnt && fullrune(buf, kbdcnt)) {
			w = chartorune(&m.keyb, buf);
			memmove(buf, buf+w, kbdcnt-w);
			kbdcnt -= w;
			p -= w;
			r <-= m;
		}
	}

}

void
mouse(chan(Mesg) r)
{
	Mesg m;
	int fd, n;
	byte buf[128];

	fd = open("/dev/mouse", OREAD);
	if(fd < 0)
		error("mouse open: %r");

	m.type = MsgMouse;
	for(;;) {
		n = read(fd, buf, sizeof(buf));
		if(n < 0)
			break;
		if(buf[0] != 'm')
			continue;

		m.buttons = buf[1] & 15;
		m.xy.x = BGLONG(buf+2);
		m.xy.y = BGLONG(buf+6);
		m.msec = BGLONG(buf+10);
		r <-= m;
	}
}

void
error(byte *fmt, ...)
{
	byte buf[128];
	
	doprint(buf, buf+sizeof(buf), fmt, ...);
	fprint(2, "Y: (fatal error) %s %r\n", buf);
	abort();
}

void
frgetmouse()
{
}
