#include	"all.h"

#define	DEBUG		0
#define	FDEV(d)		(d->fw.fw)

long
fwormsize(Device *d)
{
	Device *fdev;
	long l;

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
	long a, b;

	print("fworm ream\n");
	devinit(d);
	fdev = FDEV(d);
	a = fwormsize(d);
	b = devsize(fdev);
	print("	fwsize = %ld\n", a);
	print("	bwsize = %ld\n", b-a);
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
fwormread(Device *d, long b, void *c)
{
	Iobuf *p;
	Device *fdev;
	long l;

	if(DEBUG)
		print("fworm read  %ld\n", b);
	fdev = FDEV(d);
	l = devsize(fdev);
	l -= l/(BUFSIZE*8) + 1;
	if(b >= l)
		panic("fworm: rbounds %ld\n", b);
	l += b/(BUFSIZE*8);

	p = getbuf(fdev, l, Bread|Bres);
	if(!p || checktag(p, Tvirgo, l))
		panic("fworm: checktag %ld\n", l);
	l = b % (BUFSIZE*8);
	if(!(p->iobuf[l/8] & (1<<(l%8)))) {
		putbuf(p);
		print("fworm: read %ld\n", b);
		return 1;
	}
	putbuf(p);
	return devread(fdev, b, c);
}

int
fwormwrite(Device *d, long b, void *c)
{
	Iobuf *p;
	Device *fdev;
	long l;

	if(DEBUG)
		print("fworm write %ld\n", b);
	fdev = FDEV(d);
	l = devsize(fdev);
	l -= l/(BUFSIZE*8) + 1;
	if(b >= l)
		panic("fworm: wbounds %ld\n", b);
	l += b/(BUFSIZE*8);

	p = getbuf(fdev, l, Bread|Bmod|Bres);
	if(!p || checktag(p, Tvirgo, l))
		panic("fworm: checktag %ld", l);
	l = b % (BUFSIZE*8);
	if((p->iobuf[l/8] & (1<<(l%8)))) {
		putbuf(p);
		print("fworm: write %ld\n", b);
		return 1;
	}
	p->iobuf[l/8] |= 1<<(l%8);
	putbuf(p);
	return devwrite(fdev, b, c);
}
