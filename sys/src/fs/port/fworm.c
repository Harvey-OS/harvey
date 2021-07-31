#include	"all.h"

#define	DEBUG		0

long
fwormsize(Device dev)
{
	Device wdev;
	long l;

	wdev = CDEV(dev);
	l = devsize(wdev);
	l -= l/(BUFSIZE*8) + 1;
	return l;
}

void
fwormream(Device dev)
{
	Iobuf *p;
	Device wdev;
	long a, b;

	print("fworm ream\n");
	devinit(dev);
	wdev = CDEV(dev);
	a = fwormsize(dev);
	b = devsize(wdev);
	print("	fwsize = %ld\n", a);
	print("	bwsize = %ld\n", b-a);
	for(; a < b; a++) {
		p = getbuf(wdev, a, Bmod|Bres);
		if(!p)
			panic("fworm: init");
		memset(p->iobuf, 0, RBUFSIZE);
		settag(p, Tvirgo, a);
		putbuf(p);
	}
}

void
fworminit(Device dev)
{

	print("fworm init\n");
	devinit(CDEV(dev));
}

int
fwormread(Device dev, long b, void *c)
{
	Iobuf *p;
	Device wdev;
	long l;

	if(DEBUG)
		print("fworm read  %ld\n", b);
	wdev = CDEV(dev);
	l = devsize(wdev);
	l -= l/(BUFSIZE*8) + 1;
	if(b >= l)
		panic("fworm: rbounds %ld\n", b);
	l += b/(BUFSIZE*8);
	p = getbuf(wdev, l, Bread|Bres);
	if(!p || checktag(p, Tvirgo, l))
		panic("fworm: checktag %ld\n", l);
	l = b % (BUFSIZE*8);
	if(!(p->iobuf[l/8] & (1<<(l%8)))) {
		putbuf(p);
		print("fworm: read %ld\n", b);
		return 1;
	}
	putbuf(p);
	return devread(wdev, b, c);
}

int
fwormwrite(Device dev, long b, void *c)
{
	Iobuf *p;
	Device wdev;
	long l;

	if(DEBUG)
		print("fworm write %ld\n", b);
	wdev = CDEV(dev);
	l = devsize(wdev);
	l -= l/(BUFSIZE*8) + 1;
	if(b >= l)
		panic("fworm: wbounds %ld\n", b);
	l += b/(BUFSIZE*8);
	wdev = CDEV(dev);
	p = getbuf(wdev, l, Bread|Bmod|Bres);
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
	return devwrite(wdev, b, c);
}
