#include <u.h>
#include <libc.h>
#include "dat.h"
#include "fns.h"

#define ause(cpu) (cpu->abuf + (cpu->iabuf++ % nelem(cpu->abuf)))

Iarg*
adup(Iarg *x)
{
	Iarg *a;

	a = ause(x->cpu);
	*a = *x;
	return a;
}

Iarg*
areg(Cpu *cpu, uchar len, uchar reg)
{
	Iarg *a;

	a = ause(cpu);
	a->cpu = cpu;
	a->tag = TREG;
	a->len = len;
	a->reg = reg;
	return a;
}

Iarg*
amem(Cpu *cpu, uchar len, uchar sreg, ulong off)
{
	Iarg *a;

	a = ause(cpu);
	a->cpu = cpu;
	a->tag = TMEM;
	a->len = len;
	a->sreg = sreg;
	a->seg = cpu->reg[sreg];
	a->off = off;
	return a;
}

Iarg*
afar(Iarg *mem, uchar len, uchar alen)
{
	Iarg *a, *p;

	p = adup(mem);
	p->len = alen;
	a = amem(mem->cpu, len, R0S, ar(p));
	p->off += alen;
	p->len = 2;
	a->seg = ar(p);
	return a;
}

Iarg*
acon(Cpu *cpu, uchar len, ulong val)
{
	Iarg *a;

	a = ause(cpu);
	a->cpu = cpu;
	a->tag = TCON;
	a->len = len;
	a->val = val;
	return a;
}

ulong
ar(Iarg *a)
{
	ulong w, o;
	Bus *io;

	switch(a->tag){
	default:
		abort();
	case TMEM:
		o = ((a->seg<<4) + (a->off & 0xFFFF)) & 0xFFFFF;
		io = a->cpu->mem + (o>>16);
		w = io->r(io->aux, o, a->len);
		break;
	case TREG:
		w = a->cpu->reg[a->reg];
		break;
	case TREG|TH:
		w = a->cpu->reg[a->reg] >> 8;
		break;
	case TCON:
		w = a->val;
		break;
	}
	switch(a->len){
	default:
		abort();
	case 1:
		w &= 0xFF;
		break;
	case 2:
		w &= 0xFFFF;
		break;
	case 4:
		break;
	}
	return w;
}

long
ars(Iarg *a)
{
	ulong w = ar(a);
	switch(a->len){
	default:
		abort();
	case 1:
		return (char)w;
	case 2:
		return (short)w;
	case 4:
		return (long)w;
	}
}

void
aw(Iarg *a, ulong w)
{
	ulong *p, o;
	Cpu *cpu;
	Bus *io;

	cpu = a->cpu;
	switch(a->tag){
	default:
		abort();
	case TMEM:
		o = ((a->seg<<4) + (a->off & 0xFFFF)) & 0xFFFFF;
		io = cpu->mem + (o>>16);
		io->w(io->aux, o, w, a->len);
		break;
	case TREG:
		p = cpu->reg + a->reg;
		switch(a->len){
		case 4:
			*p = w;
			break;
		case 2:
			*p = (*p & ~0xFFFF) | (w & 0xFFFF);
			break;
		case 1:
			*p = (*p & ~0xFF) | (w & 0xFF);
			break;
		}
		break;
	case TREG|TH:
		p = cpu->reg + a->reg;
		*p = (*p & ~0xFF00) | (w & 0xFF)<<8;
		break;
	}
}
