#include <u.h>
#include <libc.h>
#include <bio.h>
#include <mach.h>
#define Extern extern
#include "3210.h"

Segment	segments[] = {
	[Ram0]	{"ram0"},
	[Ram1]	{"ram1"},
	[Mema]	{"mema"},
	[Memb]	{"memb"},
};

void
initseg(void)
{
	segments[Ram0].table = emalloc(sizeof(char*));
	segments[Ram1].table = emalloc(sizeof(char*));
	segments[Ram0].table[0] = emalloc(BY2PG);
	segments[Ram1].table[0] = emalloc(BY2PG);
	segments[Ram0].ntbl= segments[Ram1].ntbl = 1;
}

int
membrk(ulong addr)
{
	USED(addr);
	return 0;
}

ulong
ifetch(ulong addr)
{
	uchar *va;

	if(addr & 3)
		error("address error (i-fetch) vaddr %.8lux", addr);

	mprof = &notext;
	if(addr < Etext && addr >= Btext)
		mprof = &iprof[(addr-Btext)/PROFGRAN];
	else if(addr < Eram && addr >= Bram)
		mprof = &ramprof[(addr-Bram)/PROFGRAN];

	va = vaddr(addr, 1, Fetch);
	va += addr & (BY2PG-1);

	if(isbigend)
		return va[0]<<24 | va[1]<<16 | va[2]<<8 | va[3];
	return va[0] | va[1]<<8 | va[2]<<16 | va[3]<<24;
}

ulong
getmem_4(ulong addr)
{
	if(isbigend)
		return getmem_1(addr)<<24 | getmem_1(addr+1)<<16
			| getmem_1(addr+2)<<8 | getmem_1(addr+3);

	return getmem_1(addr) | getmem_1(addr+1)<<8
		| getmem_1(addr+2)<<16 | getmem_1(addr+3)<<24;
}

ushort
getmem_2(ulong addr)
{
	if(isbigend)
		return getmem_1(addr)<<8 | getmem_1(addr+1);
	return getmem_1(addr) | getmem_1(addr+1)<<8;
}

uchar
getmem_1(ulong addr)
{
	uchar *va;

	va = vaddr(addr, 0, 0);
	va += addr & (BY2PG-1);
	return va[0];
}

ulong
getmem_w(ulong addr)
{
	uchar *va;

	if(addr & 3)
		error("address error (load) vaddr %.8lux", addr);
	if(membpt)
		brkchk(addr, Read);

	va = vaddr(addr, 1, Load);
	va += addr & (BY2PG-1);
	loads++;

	if(isbigend)
		return va[0]<<24 | va[1]<<16 | va[2]<<8 | va[3];
	return va[0] | va[1]<<8 | va[2]<<16 | va[3]<<24;
}

ushort
getmem_h(ulong addr)
{
	uchar *va;

	if(addr & 1)
		error("address error (load) vaddr %.8lux", addr);
	if(membpt)
		brkchk(addr, Read);

	va = vaddr(addr, 1, Load);
	va += addr & (BY2PG-1);
	loads++;

	if(isbigend)
		return va[0]<<8 | va[1];
	return va[0] | va[1]<<8;
}

uchar
getmem_b(ulong addr)
{
	uchar *va;

	if(membpt)
		brkchk(addr, Read);

	va = vaddr(addr, 1, Load);
	va += addr & (BY2PG-1);
	loads++;
	return va[0];
}

void
putmem_4(ulong addr, ulong data)
{
	if(isbigend){
		putmem_1(addr++, data>>24);
		putmem_1(addr++, data>>16);
		putmem_1(addr++, data>>8);
		putmem_1(addr, data);
	}else{
		putmem_1(addr++, data);
		putmem_1(addr++, data>>8);
		putmem_1(addr++, data>>16);
		putmem_1(addr, data>>24);
	}
}

void
putmem_2(ulong addr, ushort data)
{
	if(isbigend){
		putmem_1(addr++, data>>8);
		putmem_1(addr, data);
	}else{
		putmem_1(addr++, data);
		putmem_1(addr, data>>8);
	}
}

void
putmem_1(ulong addr, uchar data)
{
	uchar *va;

	va = vaddr(addr, 0, 0);
	va += addr & (BY2PG-1);
	va[0] = data;
}

void
putmem_w(ulong addr, ulong data)
{
	uchar *va;

	if(addr & 3)
		error("address error (store) vaddr %.8lux", addr);

	va = vaddr(addr, 1, Store);
	va += addr&(BY2PG-1);

	if(isbigend){
		va[0] = data>>24;
		va[1] = data>>16;
		va[2] = data>>8;
		va[3] = data;
	}else{
		va[0] = data;
		va[1] = data>>8;
		va[2] = data>>16;
		va[3] = data>>24;
	}
	stores++;
	if(membpt)
		brkchk(addr, Write);
}

void
putmem_h(ulong addr, ushort data)
{
	uchar *va;

	if(addr & 1)
		error("address error (store) vaddr %.8lux", addr);

	va = vaddr(addr, 1, Store);
	va += addr&(BY2PG-1);
	if(isbigend){
		va[0] = data>>8;
		va[1] = data;
	}else{
		va[0] = data;
		va[1] = data>>8;
	}
	stores++;
	if(membpt)
		brkchk(addr, Write);
}

void
putmem_b(ulong addr, uchar data)
{
	uchar *va;


	va = vaddr(addr, 1, Store);
	va += addr&(BY2PG-1);
	va[0] = data;
	stores++;
	if(membpt)
		brkchk(addr, Write);
}

/*
 * return a pointer to the 1k piece of memory containing addr
 */
void *
vaddr(ulong addr, int running, int kind)
{
	Segment *seg;
	uchar **p;
	ulong base, n;

	base = dspmem;
	if(addr >= base && addr < base+0x10000){
		addr -= base;
		if(addr < 0x400)
			error("no boot memory");
		if(addr < 0x800)
			error("no memory io regs");
		if(addr < 0xe000)
			error("access to reserved memory");
		if(addr < 0xf000)
			seg = &segments[Ram1];
		else
			seg = &segments[Ram0];
		addr = 0;
	}else{
		if(addr < 0x60000000){
			if(addr >= base + 0x10000)
				addr -= 0x10000;
			seg = &segments[Mema];
		}else{
			addr -= 0x60000000;
			seg = &segments[Memb];
		}
	}
	if(addr / BY2PG >= seg->ntbl){
		n = addr / BY2PG + 1;
		seg->table = erealloc(seg->table, n*sizeof(uchar*));
		memset(&seg->table[seg->ntbl], 0, (n - seg->ntbl)*sizeof(uchar*));
		seg->ntbl = n;
	}
	p = &seg->table[addr / BY2PG];
	if(!*p){
		seg->rss++;
		*p = emalloc(BY2PG);
	}
	if(running){
		seg->refs++;
		mprof->refs[kind][seg-segments]++;
		if(lock)
			locked[kind]++;
	}
	return *p;
}
