#include	"all.h"

/*
 * multiple cat devices
 */
void
mcatinit(Device dev)
{
	int lb, hb;

	print("mcat init\n");
	lb = dev.unit;
	hb = dev.part;
	while(lb < hb) {
		devinit(cwdevs[lb]);
		lb++;
	}
}

long
mcatsize(Device dev)
{
	int lb, hb;
	long l;

	lb = dev.unit;
	hb = dev.part;
	l = 0;
	while(lb < hb) {
		l += devsize(cwdevs[lb]);
		lb++;
	}
	return l;
}

int
mcatread(Device dev, long b, void *c)
{
	int lb, hb;
	long l, m;

	lb = dev.unit;
	hb = dev.part;
	l = 0;
	while(lb < hb) {
		m = devsize(cwdevs[lb]);
		if(b < l+m)
			return devread(cwdevs[lb], b-l, c);
		l += m;
		lb++;
	}
	print("mcatread %ld %ld", b, lb);
	return 1;
}

int
mcatwrite(Device dev, long b, void *c)
{
	int lb, hb;
	long l, m;

	lb = dev.unit;
	hb = dev.part;
	l = 0;
	while(lb < hb) {
		m = devsize(cwdevs[lb]);
		if(b < l+m)
			return devwrite(cwdevs[lb], b-l, c);
		l += m;
		lb++;
	}
	print("mcatwrite %ld %ld", b, lb);
	return 1;
}

/*
 * multiple interleave devices
 */
void
mlevinit(Device dev)
{
	int lb, hb;

	print("mlev init\n");
	lb = dev.unit;
	hb = dev.part;
	while(lb < hb) {
		devinit(cwdevs[lb]);
		lb++;
	}
}

long
mlevsize(Device dev)
{
	int lb, hb, n;
	long l, min;

	lb = dev.unit;
	hb = dev.part;
	n = hb-lb;
	min = 0;
	while(lb < hb) {
		l = devsize(cwdevs[lb]);
		if(min == 0 || l < min)
			min = l;
		lb++;
	}
	return n * min;
}

int
mlevread(Device dev, long b, void *c)
{
	int lb, hb, n;

	lb = dev.unit;
	hb = dev.part;
	n = hb-lb;
	return devread(cwdevs[lb+b%n], b/n, c);
}

int
mlevwrite(Device dev, long b, void *c)
{
	int lb, hb, n;

	lb = dev.unit;
	hb = dev.part;
	n = hb-lb;
	return devwrite(cwdevs[lb+b%n], b/n, c);
}

/*
 * partition device
 */
void
partinit(Device dev)
{
	Device d;

	print("part init\n");
	d = cwdevs[dev.ctrl];
	devinit(d);
}

long
partsize(Device dev)
{
	long size, l;
	Device d;

	d = cwdevs[dev.ctrl];
	l = devsize(d) / 100;
	size = dev.part * l;
	if(size == 0)
		size = l*100;
	return size;
}

int
partread(Device dev, long b, void *c)
{
	long base, size, l;
	Device d;

	d = cwdevs[dev.ctrl];
	l = devsize(d) / 100;
	base = dev.unit * l;
	size = dev.part * l;
	if(size == 0)
		size = l*100;
	if(b < size)
		return devread(d, base+b, c);
	print("partread %ld %ld\n", b, size);
	return 1;
}

int
partwrite(Device dev, long b, void *c)
{
	long base, size, l;
	Device d;

	d = cwdevs[dev.ctrl];
	l = devsize(d) / 100;
	base = dev.unit * l;
	size = dev.part * l;
	if(size == 0)
		size = l*100;
	if(b < size)
		return devwrite(d, base+b, c);
	print("partwrite %ld %ld\n", b, size);
	return 1;
}
