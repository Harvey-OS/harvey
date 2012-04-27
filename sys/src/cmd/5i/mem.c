#include <u.h>
#include <libc.h>
#include <bio.h>
#include <mach.h>
#include "arm.h"

extern ulong	textbase;

ulong
ifetch(ulong addr)
{
	uchar *va;

	if(addr&3) {
		Bprint(bioout, "Address error (I-fetch) vaddr %.8lux\n", addr);
		longjmp(errjmp, 0);
	}

	if(icache.on)
		updateicache(addr);
	iprof[(addr-textbase)/PROFGRAN]++;

	va = vaddr(addr);
	va += addr&(BY2PG-1);

	return va[3]<<24 | va[2]<<16 | va[1]<<8 | va[0];
}

ulong
getmem_4(ulong addr)
{
	ulong val;
	int i;

	val = 0;
	for(i = 0; i < 4; i++)
		val = (val>>8) | (getmem_b(addr++)<<24);
	return val;
}

ulong
getmem_2(ulong addr)
{
	ulong val;
	int i;

	val = 0;
	for(i = 0; i < 2; i++)
		val = (val>>8) | (getmem_b(addr++)<<16);
	return val;
}

ulong
getmem_w(ulong addr)
{
	uchar *va;
	ulong w;

	if(addr&3) {
		w = getmem_w(addr & ~3);
		while(addr & 3) {
			w = (w>>8) | (w<<24);
			addr--;
		}
		return w;
	}
	if(membpt)
		brkchk(addr, Read);

	va = vaddr(addr);
	va += addr&(BY2PG-1);

	return va[3]<<24 | va[2]<<16 | va[1]<<8 | va[0];
}

ushort
getmem_h(ulong addr)
{
	uchar *va;
	ulong w;

	if(addr&1) {
		w = getmem_h(addr & ~1);
		while(addr & 1) {
			w = (w>>8) | (w<<8);
			addr--;
		}
		return w;
	}
	if(membpt)
		brkchk(addr, Read);

	va = vaddr(addr);
	va += addr&(BY2PG-1);

	return va[1]<<8 | va[0];
}

uchar
getmem_b(ulong addr)
{
	uchar *va;

	if(membpt)
		brkchk(addr, Read);

	va = vaddr(addr);
	va += addr&(BY2PG-1);
	return va[0];
}

uvlong
getmem_v(ulong addr)
{
	return ((uvlong)getmem_w(addr+4) << 32) | getmem_w(addr);
}

void
putmem_h(ulong addr, ushort data)
{
	uchar *va;

	if(addr&1) {
		Bprint(bioout, "Address error (Store) vaddr %.8lux\n", addr);
		longjmp(errjmp, 0);
	}

	va = vaddr(addr);
	va += addr&(BY2PG-1);

	va[1] = data>>8;
	va[0] = data;
	if(membpt)
		brkchk(addr, Write);
}

void
putmem_w(ulong addr, ulong data)
{
	uchar *va;

	if(addr&3) {
		Bprint(bioout, "Address error (Store) vaddr %.8lux\n", addr);
		longjmp(errjmp, 0);
	}

	va = vaddr(addr);
	va += addr&(BY2PG-1);

	va[3] = data>>24;
	va[2] = data>>16;
	va[1] = data>>8;
	va[0] = data;
	if(membpt)
		brkchk(addr, Write);
}

void
putmem_b(ulong addr, uchar data)
{
	uchar *va;

	va = vaddr(addr);
	va += addr&(BY2PG-1);
	va[0] = data;
	if(membpt)
		brkchk(addr, Write);
}

void
putmem_v(ulong addr, uvlong data)
{
	putmem_w(addr, data);	/* two stages, to catch brkchk */
	putmem_w(addr+4, data>>32);
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
		fatal(0, "memio");
	case MemRead:
		while(size--)
			*mb++ = getmem_b(mem++);
		break;
	case MemReadstring:
		for(;;) {
			if(size-- == 0) {
				Bprint(bioout, "memio: user/kernel copy too long for arm\n");
				longjmp(errjmp, 0);
			}
			c = getmem_b(mem++);
			*mb++ = c;
			if(c == '\0')
				break;
		}
		break;
	case MemWrite:
		for(i = 0; i < size; i++)
			putmem_b(mem++, *mb++);
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
vaddr(ulong addr)
{
	Segment *s, *es;
	int off, foff, l, n;
	uchar **p, *a;

	if(tlb.on)
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
					fatal(1, "vaddr text seek");
				if(read(text, *p, BY2PG) < 0)
					fatal(1, "vaddr text read");
				return *p;
			case Data:
				*p = emalloc(BY2PG);
				foff = s->fileoff+(off*BY2PG);
				if(seek(text, foff, 0) < 0)
					fatal(1, "vaddr text seek");
				n = read(text, *p, BY2PG);
				if(n < 0)
					fatal(1, "vaddr text read");
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
	Bprint(bioout, "User TLB miss vaddr 0x%.8lux\n", addr);
	longjmp(errjmp, 0);
	return 0;		/*to stop compiler whining*/
}
