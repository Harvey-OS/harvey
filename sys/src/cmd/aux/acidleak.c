#include <u.h>
#include <libc.h>
#include <bio.h>

void*
emalloc(ulong sz)
{
	void *v;

	v = malloc(sz);
	if(v == nil)
		sysfatal("malloc %lud fails\n", sz);
	memset(v, 0, sz);
	return v;
}

void*
erealloc(void *v, ulong sz)
{
	v = realloc(v, sz);
	if(v == nil)
		sysfatal("realloc %lud fails\n", sz);
	return v;
}

typedef struct Block Block;
typedef struct Data Data;
struct Block {
	ulong addr;
	ulong size;
	ulong w0;
	ulong w1;
	int mark;
	Data *d;
};

struct Data {
	ulong addr;
	ulong val;
	uchar type;
	Block *b;
};

Block *block;
int nblock;

Data *data;
Data *edata;
int ndata;

int
addrcmp(void *va, void *vb)
{
	ulong *a, *b;

	a = va;
	b = vb;
	if(*a < *b)
		return -1;
	if(*a > *b)
		return 1;
	return 0;
}

Block*
findblock(ulong addr)
{
	int lo, hi, m;

	lo = 0;
	hi = nblock;

	while(lo < hi) {
		m = (lo+hi)/2;
		if(block[m].addr < addr)
			lo = m+1;
		else if(addr < block[m].addr)
			hi = m;
		else
			return &block[m];
	}
	return nil;
}

Data*
finddata(ulong addr)
{
	int lo, hi, m;

	lo = 0;
	hi = ndata;

	while(lo < hi) {
		m = (lo+hi)/2;
		if(data[m].addr < addr)
			lo = m+1;
		else if(addr < data[m].addr)
			hi = m;
		else
			return &data[m];
	}
	if(0 <= lo && lo < ndata)
		return &data[lo];
}

int nmark;

int
markblock(Block *b, ulong from)
{
	Data *d;
	ulong top;
	Block *nb;

USED(from);
//print("trace 0x%.8lux from 0x%.8lux (%d)\n", b->addr, from, b->mark);
	if(b->mark)
		return 0;
	b->mark = 1;
	nmark++;

	if(d = finddata(b->addr)) {
		assert(d->addr >= b->addr);
		b->d = d;
		top = b->addr+b->size;
		for(; d < edata && d->addr < top; d++) {
			assert(d->b == 0);
			d->b = b;
			if((nb = findblock(d->val-8)) || (nb = findblock(d->val-8-8)))
				markblock(nb, b->addr);
		}
		return 1;
	}
	return 0;
}

enum {
	AllocColor = 2,	// dark blue: completely allocated region
	HdrColor = 54,		// bright blue: region with header
	LeakColor = 205,	// dark red: region with leak
	LeakHdrColor = 240,	// bright red: region with leaked header
	FreeColor = 252,	// bright yellow: completely free region
	NoColor = 255,		// padding, white
};

int
rXr(int as, int ae, int bs, int be)
{
	return bs < ae && as < be;
}

void
main(int argc, char **argv)
{
	Biobuf bio;
	char *p, *f[10];
	int bitmap, c, nf, resolution, n8, n16, hdr, nhdr, nlhdr, nleak, x, y, nb;
	ulong allocstart, allocend, len, u;
	Data *d, *ed;
	Block *b, *eb;

	bitmap = 0;
	resolution = 8;
	x = 512;
	ARGBEGIN{
	case 'b':
		bitmap=1;
		break;
	case 'r':
		resolution = atoi(EARGF(sysfatal("usage")));
		break;
	case 'x':
		x = atoi(EARGF(sysfatal("usage")));
		break;
	}ARGEND

	n8 = n16 = 0;
	allocstart = allocend = 0;
	Binit(&bio, 0, OREAD);
	while(p=Brdline(&bio, '\n')) {
		p[Blinelen(&bio)-1] = '\0';
		nf = tokenize(p, f, nelem(f));
		if(nf >= 4 && strcmp(f[0], "data") == 0) {
			if(ndata%64==0)
				data = erealloc(data, (ndata+64)*sizeof(Data));
			data[ndata].addr = strtoul(f[1], nil, 0);
			data[ndata].val = strtoul(f[2], nil, 0);
			data[ndata].type = f[3][0];
			data[ndata].b = 0;
			ndata++;
		}
		if(nf >= 5 && strcmp(f[0], "block") == 0) {
			if(nblock%64 == 0)
				block = erealloc(block, (nblock+64)*sizeof(Block));
			block[nblock].addr = strtoul(f[1], nil, 0);
			block[nblock].size = strtoul(f[2], nil, 0);
			block[nblock].w0 = strtoul(f[3], nil, 0);
			block[nblock].w1 = strtoul(f[4], nil, 0);
			block[nblock].mark = 0;
			block[nblock].d = 0;
			nblock++;
		}
		if(nf >= 4 && strcmp(f[0], "range") == 0 && strcmp(f[1], "alloc") == 0) {
			allocstart = strtoul(f[2], 0, 0)&~15;
			allocend = strtoul(f[3], 0, 0);
		}
	}

	qsort(block, nblock, sizeof(Block), addrcmp);
	qsort(data, ndata, sizeof(Data), addrcmp);

	ed = edata = data+ndata;
	for(d=data; d<ed; d++) {
		if(d->type == 'a')
			continue;
		if(b = findblock(d->val-8))		// pool header 2 words
			n8 += markblock(b, d->addr);
		else if(b = findblock(d->val-8-8))	// sometimes malloc header 2 words
			n16 += markblock(b, d->addr);
		else
			{}//print("noblock %.8lux\n", d->val);
	}

	Binit(&bio, 1, OWRITE);
	if(bitmap){
		if(n8 > n16)		// guess size of header
			hdr = 8;
		else
			hdr = 16;

		for(d=data; d<ed; d++)
			if(d->type=='a')
				break;
		if(d==ed)
			sysfatal("no allocated data region");

		len = (allocend-allocstart+resolution-1)/resolution;
		y = (len+x-1)/x;
		Bprint(&bio, "%11s %11d %11d %11d %11d ", "m8", 0, 0, x, y);

//fprint(2, "alloc %lux %lux x %d y %d res %d\n", allocstart, allocend, x, y, resolution);

		b = block;
		eb = block+nblock;
		for(u = allocstart; u<allocend; u+=resolution){
//fprint(2, "u %lux %lux baddr %lux\n", u, u+resolution, b->addr);
			while(b->addr+b->size <= u && b < eb)
//{
//fprint(2, "\tskip %lux %lux\n", b->addr, b->addr+b->size);
				b++;
//}
			nhdr = 0;
			nleak = 0;
			nb = 0;
			nlhdr = 0;
			if(block < b && u < (b-1)->addr+(b-1)->size)
				b--;

			for(; b->addr < u+resolution && b < eb; b++){
//fprint(2, "\tblock %lux %lux %d\n", b->addr, b->addr+b->size, b->mark);
				if(rXr(b->addr, b->addr+hdr, u, u+resolution)
				|| rXr(b->addr+b->size-8, b->addr+b->size, u, u+resolution)){
					if(b->mark == 0)
						nlhdr++;
					else
						nhdr++;
				}
				if(b->mark == 0)
					nleak++;
				nb++;
			}
			if(nhdr)
				c = HdrColor;
			else if(nlhdr)
				c = LeakHdrColor;
			else if(nleak)
				c = LeakColor;
			else if(nb)
				c = AllocColor;
			else
				c = FreeColor;
//fprint(2, "\t%d\n", c);
			Bputc(&bio, c);
		}
		allocend = allocstart+x*y*resolution;
		for(; u < allocend; u+=resolution)
			Bputc(&bio, NoColor);
	}else{
		eb = block+nblock;
		for(b=block; b<eb; b++)
			if(b->mark == 0)
				Bprint(&bio, "block 0x%.8lux 0x%.8lux 0x%.8lux 0x%.8lux\n", b->addr, b->size, b->w0, b->w1);
	}
	Bterm(&bio);
}
