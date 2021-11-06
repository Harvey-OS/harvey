#include "all.h"

#define	FDEV(d)		((d)->fw.fw)

enum { DEBUG = 0 };

Devsize
fwormsize(Device *d)
{
	Devsize l;

	l = devsize(FDEV(d));
	l -= l/(BUFSIZE*8) + 1;
	return l;
}

void
fwormream(Device *d)
{
	Iobuf *p;
	Device *fdev;
	Off a, b;

	print("fworm ream\n");
	devinit(d);
	fdev = FDEV(d);
	a = fwormsize(d);
	b = devsize(fdev);
	print("\tfwsize = %lld\n", (Wideoff)a);
	print("\tbwsize = %lld\n", (Wideoff)b-a);
	for(; a < b; a++) {
		p = getbuf(fdev, a, Bmod|Bres);
		if(!p)
			panic("fworm: init");
		memset(p->iobuf, 0, RBUFSIZE);
		settag(p, Tvirgo, a);
		putbuf(p);
	}
}

void
fworminit(Device *d)
{
	print("fworm init\n");
	devinit(FDEV(d));
}

int
fwormread(Device *d, Off b, void *c)
{
	Iobuf *p;
	Device *fdev;
	Devsize l;

	if(DEBUG)
		print("fworm read  %lld\n", (Wideoff)b);
	fdev = FDEV(d);
	l = devsize(fdev);
	l -= l/(BUFSIZE*8) + 1;
	if(b >= l)
		panic("fworm: rbounds %lld", (Wideoff)b);
	l += b/(BUFSIZE*8);

	p = getbuf(fdev, l, Brd|Bres);
	if(!p || checktag(p, Tvirgo, l))
		panic("fworm: checktag %lld", (Wideoff)l);
	l = b % (BUFSIZE*8);
	if(!(p->iobuf[l/8] & (1<<(l%8)))) {
		putbuf(p);
		print("fworm: read %lld\n", (Wideoff)b);
		return 1;
	}
	putbuf(p);
	return devread(fdev, b, c);
}

int
fwormwrite(Device *d, Off b, void *c)
{
	Iobuf *p;
	Device *fdev;
	Devsize l;

	if(DEBUG)
		print("fworm write %lld\n", (Wideoff)b);
	fdev = FDEV(d);
	l = devsize(fdev);
	l -= l/(BUFSIZE*8) + 1;
	if(b >= l)
		panic("fworm: wbounds %lld", (Wideoff)b);
	l += b/(BUFSIZE*8);

	p = getbuf(fdev, l, Brd|Bmod|Bres);
	if(!p || checktag(p, Tvirgo, l))
		panic("fworm: checktag %lld", (Wideoff)l);
	l = b % (BUFSIZE*8);
	if((p->iobuf[l/8] & (1<<(l%8)))) {
		putbuf(p);
		print("fworm: write %lld\n", (Wideoff)b);
		return 1;
	}
	p->iobuf[l/8] |= 1<<(l%8);
	putbuf(p);
	return devwrite(fdev, b, c);
}
