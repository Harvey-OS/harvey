/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include <u.h>
#include <libc.h>
#include <bio.h>
#include <mach.h>
#define Extern extern
#include "mips.h"

extern uint32_t textbase;

uint32_t
ifetch(uint32_t addr)
{
	uint8_t* va;

	if(addr & 3) {
		Bprint(bioout, "Address error (I-fetch) vaddr %.8lux\n", addr);
		longjmp(errjmp, 0);
	}

	if(icache.on)
		updateicache(addr);

	iprof[(addr - textbase) / PROFGRAN]++;

	va = vaddr(addr);
	va += addr & (BY2PG - 1);

	return va[0] << 24 | va[1] << 16 | va[2] << 8 | va[3];
}

uint32_t
getmem_4(uint32_t addr)
{
	uint32_t val;
	int i;

	val = 0;
	for(i = 0; i < 4; i++)
		val = val << 8 | getmem_b(addr++);
	return val;
}

uint32_t
getmem_2(uint32_t addr)
{
	uint32_t val;

	val = getmem_b(addr);
	val = val << 8 | getmem_b(addr + 1);

	return val;
}

uint32_t
getmem_w(uint32_t addr)
{
	uint8_t* va;

	if(addr & 3) {
		Bprint(bioout, "Address error (Load) vaddr %.8lux\n", addr);
		longjmp(errjmp, 0);
	}
	if(membpt)
		brkchk(addr, Read);

	va = vaddr(addr);
	va += addr & (BY2PG - 1);

	return va[0] << 24 | va[1] << 16 | va[2] << 8 | va[3];
	;
}

uint16_t
getmem_h(uint32_t addr)
{
	uint8_t* va;

	if(addr & 1) {
		Bprint(bioout, "Address error (Load) vaddr %.8lux\n", addr);
		longjmp(errjmp, 0);
	}
	if(membpt)
		brkchk(addr, Read);

	va = vaddr(addr);
	va += addr & (BY2PG - 1);

	return va[0] << 8 | va[1];
}

uint8_t
getmem_b(uint32_t addr)
{
	uint8_t* va;

	if(membpt)
		brkchk(addr, Read);

	va = vaddr(addr);
	va += addr & (BY2PG - 1);
	return va[0];
}

void
putmem_w(uint32_t addr, uint32_t data)
{
	uint8_t* va;

	if(addr & 3) {
		Bprint(bioout, "Address error (Store) vaddr %.8lux\n", addr);
		longjmp(errjmp, 0);
	}

	va = vaddr(addr);
	va += addr & (BY2PG - 1);

	va[0] = data >> 24;
	va[1] = data >> 16;
	va[2] = data >> 8;
	va[3] = data;
	if(membpt)
		brkchk(addr, Write);
}
void
putmem_b(uint32_t addr, uint8_t data)
{
	uint8_t* va;

	va = vaddr(addr);
	va += addr & (BY2PG - 1);
	va[0] = data;
	if(membpt)
		brkchk(addr, Write);
}

void
putmem_h(uint32_t addr, int16_t data)
{
	uint8_t* va;

	if(addr & 1) {
		Bprint(bioout, "Address error (Store) vaddr %.8lux\n", addr);
		longjmp(errjmp, 0);
	}

	va = vaddr(addr);
	va += addr & (BY2PG - 1);
	va[0] = data >> 8;
	va[1] = data;
	if(membpt)
		brkchk(addr, Write);
}

char*
memio(char* mb, uint32_t mem, int size, int dir)
{
	int i;
	char* buf, c;

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
				Bprint(bioout, "memio: user/kernel copy too "
				               "long for mipsim\n");
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
dotlb(uint32_t vaddr)
{
	uint32_t* l, *e;

	vaddr &= ~(BY2PG - 1);

	e = &tlb.tlbent[tlb.tlbsize];
	for(l = tlb.tlbent; l < e; l++)
		if(*l == vaddr) {
			tlb.hit++;
			return;
		}

	tlb.miss++;
	tlb.tlbent[lnrand(tlb.tlbsize)] = vaddr;
}

void*
vaddr1(uint32_t addr)
{
	Segment* s, *es;
	int off, foff, l, n;
	uint8_t** p, *a;

	if(tlb.on)
		dotlb(addr);

	es = &memory.seg[Nseg];
	for(s = memory.seg; s < es; s++) {
		if(addr >= s->base && addr < s->end) {
			s->refs++;
			off = (addr - s->base) / BY2PG;
			p = &s->table[off];
			if(*p)
				return *p;
			s->rss++;
			switch(s->type) {
			default:
				fatal(0, "vaddr");
			case Text:
				*p = emalloc(BY2PG);
				if(seek(text, s->fileoff + (off * BY2PG), 0) <
				   0)
					fatal(1, "vaddr text seek");
				if(read(text, *p, BY2PG) < 0)
					fatal(1, "vaddr text read");
				return *p;
			case Data:
				*p = emalloc(BY2PG);
				foff = s->fileoff + (off * BY2PG);
				if(seek(text, foff, 0) < 0)
					fatal(1, "vaddr text seek");
				n = read(text, *p, BY2PG);
				if(n < 0)
					fatal(1, "vaddr text read");
				if(foff + n > s->fileend) {
					l = BY2PG - (s->fileend - foff);
					a = *p + (s->fileend - foff);
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
	return 0;
}

void*
vaddr(uint32_t addr)
{
	void* v;

	v = vaddr1(addr);
	if(v == 0) {
		Bprint(bioout, "User TLB miss vaddr 0x%.8lux\n", addr);
		longjmp(errjmp, 0);
	}
	return v;
}

int
badvaddr(uint32_t addr, int n)
{
	void* v;

	if(addr & (n - 1))
		return 1;
	v = vaddr1(addr);
	if(v == 0)
		return 1;
	return 0;
}
