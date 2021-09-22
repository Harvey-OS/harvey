#include <u.h>
#include <libc.h>
#include <bio.h>
#include <mach.h>
#define Extern extern
#include "arm64.h"

extern ulong	textbase;

ulong
ifetch(uvlong addr)
{
	uchar *va;
	ulong px;

	if(addr&3) {
		Bprint(bioout, "instruction_address_not_aligned [addr %.16llux]\n", addr);
		longjmp(errjmp, 0);
	}

	if(icache.on)
		updateicache(addr);

	va = vaddr(addr);
	px = (addr-textbase)/PROFGRAN;
	if(px < iprofsize)
		iprof[px]++;

	va += addr&(BY2PG-1);

	return va[3]<<24 | va[2]<<16 | va[1]<<8 | va[0];
}

ulong
getmem_4(uvlong addr)
{
	ulong val;
	int i;

	val = 0;
	for(i = 0; i < 4; i++)
		val = (val>>8) | (getmem_b(addr++)<<24);
	return val;
}

ulong
getmem_2(uvlong addr)
{
	ulong val;
	int i;

	val = 0;
	for(i = 0; i < 2; i++)
		val = (val>>8) | (getmem_b(addr++)<<16);
	return val;
}

uvlong
getmem_v(uvlong addr)
{
	return ((uvlong)getmem_w(addr+4) << 32) | getmem_w(addr);
}

ulong
getmem_w(uvlong addr)
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
getmem_h(uvlong addr)
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
getmem_b(uvlong addr)
{
	uchar *va;

	if(membpt)
		brkchk(addr, Read);

	va = vaddr(addr);
	va += addr&(BY2PG-1);
	return va[0];
}

void
putmem_v(uvlong addr, uvlong data)
{
	putmem_w(addr, data);	/* two stages, to catch brkchk */
	putmem_w(addr+4, data>>32);
}

void
putmem_w(uvlong addr, ulong data)
{
	uchar *va;

	if(addr&3) {
		Bprint(bioout, "mem_address_not_aligned [store addr %.16llux pc 0x%lux]\n", addr, reg.pc);
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
putmem_b(uvlong addr, uchar data)
{
	uchar *va;

	va = vaddr(addr);
	va += addr&(BY2PG-1);
	va[0] = data;
	if(membpt)
		brkchk(addr, Write);
}

void
putmem_h(uvlong addr, short data)
{
	uchar *va;

	if(addr&1) {
		Bprint(bioout, "mem_address_not_aligned [store addr %.16llux] pc 0x%lux]\n", addr, reg.pc);
		longjmp(errjmp, 0);
	}

	va = vaddr(addr);
	va += addr&(BY2PG-1);
	va[1] = data>>8;
	va[0] = data;
	if(membpt)
		brkchk(addr, Write);
}

char *
memio(char *mb, uvlong mem, int size, int dir)
{
	int i;
	char *buf, c;

	if(size < 0) {
		Bprint(bioout, "memio: invalid size: %d\n", size);
		longjmp(errjmp, 0);
	}
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
				Bprint(bioout, "memio: user/kernel copy too long for 7i\n");
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

void *
vaddr(uvlong addr)
{
	Segment *s, *es;
	int off, foff, l, n;
	uchar **p, *a;

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
				if(pread(text, *p, BY2PG, s->fileoff+(off*BY2PG)) < 0)
					fatal(1, "vaddr text read");
				return *p;
			case Data:
				*p = emalloc(BY2PG);
				foff = s->fileoff+(off*BY2PG);
				n = pread(text, *p, BY2PG, foff);
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
	Bprint(bioout, "data_access_MMU_miss [addr 0x%.16llux pc 0x%lux]\n", addr, reg.pc);
	longjmp(errjmp, 0);
	return 0;		/*to stop compiler whining*/
}
