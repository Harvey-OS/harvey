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

	list = ialloc(d->cat.ndev*sizeof(Device*), 0);
	d->private = list;
	for(x=d->cat.first; x; x=x->link) {
		*list++ = x;
		x->size = devsize(x);
	}
}

long
mcatsize(Device *d)
{
	Device *x;
	long l, m;

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
mcatread(Device *d, long b, void *c)
{
	Device *x;
	long l, m;

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
	print("mcatread %ld %ld\n", b, l);
	return 1;
}

int
mcatwrite(Device *d, long b, void *c)
{
	Device *x;
	long l, m;

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
	print("mcatwrite %ld %ld\n", b, l);
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

long
mlevsize(Device *d)
{
	Device *x;
	int n;
	long m, min;

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
mlevread(Device *d, long b, void *c)
{
	int n;
	Device **list;

	n = d->cat.ndev;
	list = d->private;
	return devread(list[b%n], b/n, c);
}

int
mlevwrite(Device *d, long b, void *c)
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

long
partsize(Device *d)
{
	long size, l;

	l = d->part.d->size / 100;
	size = d->part.size * l;
	if(size == 0)
		size = l*100;
	return size;
}

int
partread(Device *d, long b, void *c)
{
	long base, size, l;

	l = d->part.d->size / 100;
	base = d->part.base * l;
	size = d->part.size * l;
	if(size == 0)
		size = l*100;
	if(b < size)
		return devread(d->part.d, base+b, c);
	print("partread %ld %ld\n", b, size);
	return 1;
}

int
partwrite(Device *d, long b, void *c)
{
	long base, size, l;

	l = d->part.d->size / 100;
	base = d->part.base * l;
	size = d->part.size * l;
	if(size == 0)
		size = l*100;
	if(b < size)
		return devwrite(d->part.d, base+b, c);
	print("partwrite %ld %ld\n", b, size);
	return 1;
}
