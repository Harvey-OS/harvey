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

void
markblock(Block *b, ulong from)
{
	Data *d;
	ulong top;
	Block *nb;

USED(from);
//print("trace 0x%.8lux from 0x%.8lux (%d)\n", b->addr, from, b->mark);
	if(b->mark)
		return;
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
	}
}

void
main(void)
{
	Biobuf bio;
	char *p, *f[10];
	int nf;
	Data *d, *ed;
	Block *b, *eb;

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
	}

	qsort(block, nblock, sizeof(Block), addrcmp);
	qsort(data, ndata, sizeof(Data), addrcmp);

	ed = edata = data+ndata;
	for(d=data; d<ed; d++) {
		if(d->type == 'a')
			continue;
		if(b = findblock(d->val-8))		// pool header 2 words
			markblock(b, d->addr);
		else if(b = findblock(d->val-8-8))	// sometimes malloc header 2 words
			markblock(b, d->addr);
		else
			;//print("noblock %.8lux\n", d->val);
	}

	Binit(&bio, 1, OWRITE);
	eb = block+nblock;
	for(b=block; b<eb; b++)
		if(b->mark == 0)
			Bprint(&bio, "block 0x%.8lux 0x%.8lux 0x%.8lux 0x%.8lux\n", b->addr, b->size, b->w0, b->w1);
	Bterm(&bio);
}
