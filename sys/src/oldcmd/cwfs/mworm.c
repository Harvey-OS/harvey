#include	"all.h"

/*
 * multiple cat devices
 */
void
mcatinit(Device *d)
{
	Device *x, **list;

	d->cat.ndev = 0;
	for(x=d->cat.first; x; x=x->link) {
		devinit(x);
		d->cat.ndev++;
	}

	list = malloc(d->cat.ndev*sizeof(Device*));
	d->private = list;
	for(x=d->cat.first; x; x=x->link) {
		*list++ = x;
		x->size = devsize(x);
	}
}

Devsize
mcatsize(Device *d)
{
	Device *x;
	Devsize l, m;

	l = 0;
	for(x=d->cat.first; x; x=x->link) {
		m = x->size;
		if(m == 0) {
			m = devsize(x);
			x->size = m;
		}
		l += m;
	}
	return l;
}

int
mcatread(Device *d, Off b, void *c)
{
	Device *x;
	Devsize l, m;

	l = 0;
	for(x=d->cat.first; x; x=x->link) {
		m = x->size;
		if(m == 0) {
			m = devsize(x);
			x->size = m;
		}
		if(b < l+m)
			return devread(x, b-l, c);
		l += m;
	}
	print("mcatread past end: %Z block %lld, %lld beyond end\n",
		d, (Wideoff)b, (Wideoff)l);
	return 1;
}

int
mcatwrite(Device *d, Off b, void *c)
{
	Device *x;
	Devsize l, m;

	l = 0;
	for(x=d->cat.first; x; x=x->link) {
		m = x->size;
		if(m == 0) {
			m = devsize(x);
			x->size = m;
		}
		if(b < l+m)
			return devwrite(x, b-l, c);
		l += m;
	}
	print("mcatwrite past end: %Z block %lld, %lld beyond end\n",
		d, (Wideoff)b, (Wideoff)l);
	return 1;
}

/*
 * multiple interleave devices
 */
void
mlevinit(Device *d)
{
	Device *x;

	mcatinit(d);
	for(x=d->cat.first; x; x=x->link)
		x->size = devsize(x);
}

Devsize
mlevsize(Device *d)
{
	Device *x;
	int n;
	Devsize m, min;

	min = 0;
	n = 0;
	for(x=d->cat.first; x; x=x->link) {
		m = x->size;
		if(m == 0) {
			m = devsize(x);
			x->size = m;
		}
		if(min == 0 || m < min)
			min = m;
		n++;
	}
	return n * min;
}

int
mlevread(Device *d, Off b, void *c)
{
	int n;
	Device **list;

	n = d->cat.ndev;
	list = d->private;
	return devread(list[b%n], b/n, c);
}

int
mlevwrite(Device *d, Off b, void *c)
{
	int n;
	Device **list;

	n = d->cat.ndev;
	list = d->private;
	return devwrite(list[b%n], b/n, c);
}

/*
 * partition device
 */
void
partinit(Device *d)
{

	devinit(d->part.d);
	d->part.d->size = devsize(d->part.d);
}

Devsize
partsize(Device *d)
{
	Devsize size, l;

	l = d->part.d->size / 100;
	size = d->part.size * l;
	if(size == 0)
		size = l*100;
	return size;
}

int
partread(Device *d, Off b, void *c)
{
	Devsize base, size, l;

	l = d->part.d->size / 100;
	base = d->part.base * l;
	size = d->part.size * l;
	if(size == 0)
		size = l*100;
	if(b < size)
		return devread(d->part.d, base+b, c);
	print("partread past end: %Z blk %lld size %lld\n",
		d, (Wideoff)b, (Wideoff)size);
	return 1;
}

int
partwrite(Device *d, Off b, void *c)
{
	Devsize base, size, l;

	l = d->part.d->size / 100;
	base = d->part.base * l;
	size = d->part.size * l;
	if(size == 0)
		size = l*100;
	if(b < size)
		return devwrite(d->part.d, base+b, c);
	print("partwrite past end: %Z blk %lld size %lld\n",
		d, (Wideoff)b, (Wideoff)size);
	return 1;
}

/*
 * mirror device
 */
void
mirrinit(Device *d)
{
	Device *x;

	mcatinit(d);
	for(x=d->cat.first; x; x=x->link)
		x->size = devsize(x);
}

Devsize
mirrsize(Device *d)
{
	Device *x;
	int n;
	Devsize m, min;

	min = 0;
	n = 0;
	for(x=d->cat.first; x; x=x->link) {
		m = x->size;
		if(m == 0) {
			m = devsize(x);
			x->size = m;
		}
		if(min == 0 || m < min)
			min = m;
		n++;
	}
	return min;
}

int
mirrread(Device *d, Off b, void *c)
{
	Device *x;

	if (d->cat.first == nil) {
		print("mirrread: empty mirror %Z\n", d);
		return 1;
	}
	for(x=d->cat.first; x; x=x->link) {
		if(x->size == 0)
			x->size = devsize(x);
		if (devread(x, b, c) == 0)	/* okay? */
			return 0;
	}
	// DANGER WILL ROBINSON
	print("mirrread: all mirrors of %Z block %lld are bad\n",
		d, (Wideoff)b);
	return 1;
}

/*
 * write the mirror(s) first so that a power outage, for example, will
 * find the main device written only if the mirrors are too, thus
 * checking the main device will also correctly check the mirror(s).
 *
 * devread and devwrite are synchronous; all buffering must be
 * implemented at higher levels.
 */
static int
ewrite(Device *x, Off b, void *c)
{
	if(x->size == 0)
		x->size = devsize(x);
	if (devwrite(x, b, c) != 0) {
		print("mirrwrite: error at %Z block %lld\n", x, (Wideoff)b);
		return 1;
	}
	return 0;
}

static int
wrmirrs1st(Device *x, Off b, void *c)	// write any mirrors of x, then x
{
	int e;

	if (x == nil)
		return 0;
	e = wrmirrs1st(x->link, b, c);
	return e | ewrite(x, b, c);
}

int
mirrwrite(Device *d, Off b, void *c)
{
	if (d->cat.first == nil) {
		print("mirrwrite: empty mirror %Z\n", d);
		return 1;
	}
	return wrmirrs1st(d->cat.first, b, c);
}
