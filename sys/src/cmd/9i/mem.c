#include <u.h>
#include <libc.h>
#include <bio.h>
#include <mach.h>
#define Extern extern
#include "sim.h"

extern ulong	textbase;

ulong
ifetch(ulong addr)
{
	uchar *va;

	if(addr&3)
		error("Address error (I-fetch) vaddr %.8lux\n", addr);

	if(icache.on)
		updateicache(addr);

	if(addr >= Btext && addr < Etext)
		iprof[(addr-Btext)/PROFGRAN]++;

	va = vaddr(addr, 1);
	va += addr&(BY2PG-1);

	return va[0]<<24 | va[1]<<16 | va[2]<<8 | va[3];
}

ulong
getmem_4(ulong addr)
{
	ulong val;
	int i;

	val = 0;
	for(i = 0; i < 4; i++)
		val = val<<8 | getmem_1(addr++);
	return val;
}

ushort
getmem_2(ulong addr)
{
	ulong val;

	val = getmem_1(addr);
	val = val<<8 | getmem_1(addr+1);

	return val;
}

uchar
getmem_1(ulong addr)
{
	uchar *va;

	va = vaddr(addr, 0);
	va += addr&(BY2PG-1);
	return va[0];
}

ulong
getmem_w(ulong addr)
{
	uchar *va;

	if(addr&3)
		error("Address error (Load) vaddr %.8lux\n", addr);

	if(membpt)
		brkchk(addr, Read);

	va = vaddr(addr, 1);
	va += addr&(BY2PG-1);

	return va[0]<<24 | va[1]<<16 | va[2]<<8 | va[3];;
}

ushort
getmem_h(ulong addr)
{
	uchar *va;

	if(addr&1)
		error("Address error (Load) vaddr %.8lux\n", addr);

	if(membpt)
		brkchk(addr, Read);

	va = vaddr(addr, 1);
	va += addr&(BY2PG-1);

	return va[0]<<8 | va[1];
}

uchar
getmem_b(ulong addr)
{
	uchar *va;

	if(membpt)
		brkchk(addr, Read);

	va = vaddr(addr, 1);
	va += addr&(BY2PG-1);
	return va[0];
}

void
putmem_4(ulong addr, ulong data)
{
	putmem_1(addr++, data>>24);
	putmem_1(addr++, data>>16);
	putmem_1(addr++, data>>8);
	putmem_1(addr, data);
}

void
putmem_2(ulong addr, ushort data)
{
	putmem_1(addr++, data>>8);
	putmem_1(addr, data);
}

void
putmem_1(ulong addr, uchar data)
{
	uchar *va;

	va = vaddr(addr, 0);
	va += addr&(BY2PG-1);
	va[0] = data;
}

void
putmem_w(ulong addr, ulong data)
{
	uchar *va;

	if(addr&3)
		error("Address error (Store) vaddr %.8lux\n", addr);

	va = vaddr(addr ,1);
	va += addr&(BY2PG-1);

	va[0] = data>>24;
	va[1] = data>>16;
	va[2] = data>>8;
	va[3] = data;
	if(membpt)
		brkchk(addr, Write);
}

void
putmem_h(ulong addr, ushort data)
{
	uchar *va;

	if(addr&1)
		error("Address error (Store) vaddr %.8lux\n", addr);

	va = vaddr(addr, 1);
	va += addr&(BY2PG-1);
	va[0] = data>>8;
	va[1] = data;
	if(membpt)
		brkchk(addr, Write);
}

void
putmem_b(ulong addr, uchar data)
{
	uchar *va;

	va = vaddr(addr, 1);
	va += addr&(BY2PG-1);
	va[0] = data;
	if(membpt)
		brkchk(addr, Write);
}

char *
memio(char *mb, ulong mem, int size, int dir)
{
	int i;
	char *buf, c;

	if(mb == 0)
		mb = emalloc(size);

	buf = mb;
	switch(dir) {
	default:
		fatal("memio");
	case MemRead:
		while(size--)
			*mb++ = getmem_1(mem++);
		break;
	case MemReadstring:
		for(;;) {
			if(size-- == 0)
				error("memio: user/kernel copy too long for mipsim\n");
			c = getmem_1(mem++);
			*mb++ = c;
			if(c == '\0')
				break;
		}
		break;
	case MemWrite:
		for(i = 0; i < size; i++)
			putmem_1(mem++, *mb++);
		break;
	}
	return buf;
}

void
dotlb(ulong vaddr)
{
	ulong *l, *e;

	vaddr &= ~(BY2PG-1);

	e = &tlb.tlbent[tlb.tlbsize];
	for(l = tlb.tlbent; l < e; l++)
		if(*l == vaddr) {
			tlb.hit++;
			return;
		}

	tlb.miss++;
	tlb.tlbent[lnrand(tlb.tlbsize)] = vaddr;
}

void *
vaddr(ulong addr, int running)
{
	Segment *s, *es;
	int off, foff, l, n;
	uchar **p, *a;

	if(tlb.on && running)
		dotlb(addr);

	es = &memory.seg[Nseg];
	for(s = memory.seg; s < es; s++) {
		if(addr >= s->base && addr < s->end) {
			s->refs++;
			off = (addr-s->base)/BY2PG;
			p = &s->table[off];
			if(*p)
				return *p;
			s->rss++;
			switch(s->type) {
			default:
				fatal(0, "vaddr");
			case Text:
				*p = emalloc(BY2PG);
				if(seek(text, s->fileoff+(off*BY2PG), 0) < 0)
					fatal("vaddr text seek: %r");
				if(read(text, *p, BY2PG) < 0)
					fatal("vaddr text read: %r");
				return *p;
			case Data:
				*p = emalloc(BY2PG);
				foff = s->fileoff+(off*BY2PG);
				if(seek(text, foff, 0) < 0)
					fatal("vaddr data seek: %r");
				n = read(text, *p, BY2PG);
				if(n < 0)
					fatal( "vaddr data read: %r");
				if(foff + n > s->fileend) {
					l = BY2PG - (s->fileend-foff);
					a = *p+(s->fileend-foff);
					memset(a, 0, l);
				}
				return *p;
			case Bss:
			case Stack:
				*p = emalloc(BY2PG);
				return *p;
			}
		}
	}
	error("User TLB miss vaddr 0x%.8lux\n", addr);
	return 0;		/*to stop compiler whining*/
}

int
membrk(ulong addr)
{
	ulong osize, nsize;
	Segment *s;

	if(addr < memory.seg[Data].base+datasize) {
		strcpy(errbuf, "address below segment");
		return -1;
	}
	if(addr > memory.seg[Stack].base) {
		strcpy(errbuf, "segment too big");
		return -1;
	}
	s = &memory.seg[Bss];
	if(addr > s->end) {
		osize = ((s->end-s->base)/BY2PG)*BY2WD;
		addr = ((addr)+(BY2PG-1))&~(BY2PG-1);
		s->end = addr;
		nsize = ((s->end-s->base)/BY2PG)*BY2WD;
		s->table = erealloc(s->table, nsize);
		if(nsize > osize)
			memset((char*)s->table+osize, 0, nsize-osize);
	}	

	return 0;	
}
