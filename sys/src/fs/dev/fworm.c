#include	"all.h"

#define	DEBUG		0
#define	FDEV(d)		(d->fw.fw)

Devsize
fwormsize(Device *d)
{
	Device *fdev;
	Devsize l;

	fdev = FDEV(d);
	l = devsize(fdev);
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
	print("	fwsize = %lld\n", (Wideoff)a);
	print("	bwsize = %lld\n", (Wideoff)b-a);
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
		panic("fworm: rbounds %lld\n", (Wideoff)b);
	l += b/(BUFSIZE*8);

	p = getbuf(fdev, l, Bread|Bres);
	if(!p || checktag(p, Tvirgo, l))
		panic("fworm: checktag %lld\n", (Wideoff)l);
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
		panic("fworm: wbounds %lld\n", (Wideoff)b);
	l += b/(BUFSIZE*8);

	p = getbuf(fdev, l, Bread|Bmod|Bres);
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
