#include <u.h>
#include <libc.h>
#include <bio.h>
#define Extern extern
#include <mach.h>
#include "3210.h"

#define percent(num, max)	(((double)(num)*100.)/(max))

Inst	sumtab[Ntype];

void
isum(void)
{
	Inst *i, *ie;
	Alu *a, *ae;
	Fspec *f, *fe;
	ulong total, arith, branch, fpu;
	ulong taken, ataken, syscall;
	ulong n, ninst;

	arith = 0;
	ataken = 0;
	branch = 0;
	taken = 0;
	syscall = 0;
	fpu = 0;

	memset(sumtab, 0, sizeof sumtab);
	total = icalc(itab);
	total += icalc(biginst);
	total += nopcount;
	ninst = total;
	if(!ninst)
		ninst = 1;

	ie = sumtab + Ntype;
	for(i = sumtab; i < ie; i++){
		switch(i->type){
		case Tcall:
		case Tcalli:
		case Tgoto:
			branch += i->count;
			break;
		case Tcgoto:
		case Tdecgoto:
		case Tdo:
			branch += i->count;
			taken += i->taken;
			break;
		case Tarith:
		case Taddi:
		case Tshiftor:
		case Tset:
			arith += i->count;
			break;
		case Tfloat:
		case Tfspec:
			fpu += i->count;
			break;
		case Tsys:
			syscall += i->count;
			break;
		case Tmem:
		case Tillegal:
			break;
		default:
			fatal("isum: bad type %d\n", i->type);
		}
	}

	Bprint(bioout, "\nInstruction summary.\n\n");

	for(i = sumtab; i < ie; i++)
		if(i->type != Tarith && i->type != Tfspec && i->type != Tmem)
			summary(i->count, ninst, i->name);
	summary(iloads, ninst, "load");
	summary(istores, ninst, "store");
	ae = alutab + 16;
	for(a = alutab; a < ae; a++){
		ataken += a->taken + a->icount;
		if(a->name)
			summary(a->rcount + a->icount, ninst, a->name);
	}
	fe = fputab+ 16;
	for(f = fputab; f < fe; f++)
		if(f->name)
			summary(f->count, ninst, f->name);

	Bprint(bioout, "%-8lud      Memory cycles\n", loads+stores+total);
	Bprint(bioout, "%-8lud      Locked cycles\n",
			locked[Fetch]+locked[Load]+locked[Store]);

	Bprint(bioout, "%-8lud %3.0f%% Instruction cycles\n",
			total, percent(total, loads+stores+ninst));
	Bprint(bioout,	"  %-8lud %3.0f%% Locked\n",
			locked[Fetch], percent(locked[Fetch], loads+stores+ninst));
	Bprint(bioout, "%-8lud %3.0f%% Data cycles\n\n",
			loads+stores, percent(loads+stores, loads+stores+ninst));
	
	Bprint(bioout, "%-8lud %3.0f%% Loads\n", loads, percent(loads, ninst));
	Bprint(bioout,	"  %-8lud %3.0f%% Locked\n",
			locked[Load], percent(locked[Load], loads+stores+ninst));
	Bprint(bioout, "%-8lud %3.0f%% Stores\n", stores, percent(stores, ninst));
	Bprint(bioout,	"  %-8lud %3.0f%% Locked\n",
			locked[Store], percent(locked[Store], loads+stores+ninst));
	Bprint(bioout, "%-8lud %3.0f%% Integer Loads\n", iloads, percent(iloads, ninst));
	Bprint(bioout, "%-8lud %3.0f%% Integer Stores\n", istores, percent(istores, ninst));
	n = loads - iloads;
	Bprint(bioout, "%-8lud %3.0f%% Floating Loads\n", n, percent(n, ninst));
	n = stores - istores;
	Bprint(bioout, "%-8lud %3.0f%% Floating Stores\n", n, percent(n, ninst));

	Bprint(bioout, "%-8lud %3.0f%% Arithmetic\n",
			arith, percent(arith, ninst));

	Bprint(bioout, "%-8lud %3.0f%% Arithmetic executed\n",
			ataken, percent(ataken, ninst));

	Bprint(bioout, "%-8lud %3.0f%% Floating point\n",
			fpu, percent(fpu, ninst));

	Bprint(bioout, "%-8lud %3.0f%% Branches\n",
			branch, percent(branch, ninst));

	if(branch == 0)
		branch = 1;
	Bprint(bioout, "   %-8lud %3.0f%% Branches taken\n",
			taken, percent(taken, branch));

	Bprint(bioout, "%-8lud %3.0f%% System calls\n",
			syscall, percent(syscall, ninst));

	Bprint(bioout, "%-8lud %3.0f%% Nops\n",
			nopcount, percent(nopcount, ninst));
}

void
summary(ulong n, ulong total, char *name)
{
	double pct;

	if(n == 0)
		return;
	pct = 0.;
	if(total)
		pct = percent(n, total);
	if(pct >= 1.0)
		Bprint(bioout, "%-8lud %3.0f%% %s\n", n, pct, name);
	else
		Bprint(bioout, "%-8lud      %s\n", n, name);
}

void
tlbsum(void)
{
}

void
cachesum(void)
{
}

ulong
icalc(Inst *inst)
{
	Inst *t;
	ulong n;
	int i;

	for(i = 0; i < Ntype; i++)
		sumtab[i].type = i;
	n = 0;
	for(; inst->func; inst++){
		n += inst->count;
		t = &sumtab[inst->type];
		t->count += inst->count;
		t->taken += inst->taken;
		t->name = inst->name;
	}
	return n;
}

void
segsum(void)
{
	Segment *s;
	int i;

	Bprint(bioout, "\n\nMemory Summary\n\n");
	Bprint(bioout, "      Resident References\n");
	for(i = 0; i < Nseg; i++){
		s = &segments[i];
		Bprint(bioout, "%-5s %-8d %-8d\n", s->name, s->rss*BY2PG, s->refs);
	}
}

typedef struct Prof Prof;
struct Prof
{
	Symbol	s;
	ulong	count;
	Refs	refs;
};
Prof	prof[5000];

int
profcmp(Prof *a, Prof *b)
{
	return b->count - a->count;
}

void
addrefs(Refs *sum, Refs *r)
{
	int i, j;

	for(i = 0; i < Nref; i++)
		for(j = 0; j < Nseg; j++)
			sum->refs[i][j] += r->refs[i][j];
	sum->nops += r->nops;
}

ulong
sumrefs(Refs *r, int kind)
{
	ulong *k;

	k = r->refs[kind];
	return k[Mema] + k[Memb] + k[Ram0] + k[Ram1];
}

ulong
sumsegs(Refs *r, int seg)
{
	return r->refs[Fetch][seg] + r->refs[Load][seg] + r->refs[Store][seg];
}

void
iprofile(void)
{
	Refs *r;
	Prof *p, *n;
	int i, b, e;
	ulong total;

	p = prof;
	for(i = 0; ; i++){
		if(textsym(&p->s, i) == 0)
			return;
		if(p->s.value >= Btext && p->s.value < Etext)
			break;
	}
	for(i++; ; i++){
		n = p+1;
		if(textsym(&n->s, i) == 0)
			break;
		if(n->s.value < Btext || n->s.value >= Etext)
			continue;
		b = (p->s.value-Btext)/PROFGRAN;
		e = (n->s.value-Btext)/PROFGRAN;
		for(; b < e; b++)
			addrefs(&p->refs, &iprof[b]);
		p = n;
	}
	b = 0;
	e = (Eram-Bram)/PROFGRAN;
	for(; b < e; b++)
		addrefs(&p->refs, &ramprof[b]);
	p->s.name = "ram";
	p->s.value = Bram;
	p++;
	n++;
	i++;

	p->refs = notext;
	p->s.name = "not in text";
	n++;
	i++;

	e = i;
	total = 0;
	for(p = prof; p < n; p++){
		r = &p->refs;
		p->count = sumrefs(r, Fetch) + sumrefs(r, Load) + sumrefs(r, Store);
		total += p->count;
	}

	qsort(prof, e, sizeof(Prof), profcmp);

	Bprint(bioout, "  cycles     inst     nops     load    store  fastram    %% symbol          file\n");
	for(i = 0; i < e; i++){
		if(prof[i].count == 0)
			continue;

		r = &prof[i].refs;
		Bprint(bioout, "%8d %8d %8d %8d %8d %8d %4.1f %-15s ",
			prof[i].count,
			sumrefs(r, Fetch), r->nops, sumrefs(r, Load), sumrefs(r, Store),
			sumsegs(r, Ram0) + sumsegs(r, Ram1),
			percent(prof[i].count, total),
			prof[i].s.name);

		printsource(prof[i].s.value);
		Bputc(bioout, '\n');
	}
	memset(prof, 0, e * sizeof *prof);
}

void
clearprof(void)
{
	Alu *a, *ae;
	Fspec *f, *fe;
	Inst *inst;
	Segment *s;
	int i;

	if(iprof)
		memset(iprof, 0, ((Etext-Btext)/PROFGRAN) * sizeof *iprof);
	memset(ramprof, 0, ((Eram-Bram)/PROFGRAN) * sizeof *ramprof);

	stores = 0;
	loads = 0;
	istores = 0;
	iloads = 0;
	nopcount = 0;
	for(i = 0; i < Nseg; i++){
		s = &segments[i];
		s->refs = 0;
	}
	for(inst = itab; inst->func; inst++)
		inst->count = inst->taken = 0;
	for(inst = biginst; inst->func; inst++)
		inst->count = inst->taken = 0;

	ae = alutab + 16;
	for(a = alutab; a < ae; a++)
		a->taken = a->rcount = a->icount = 0;
	fe = fputab+ 16;
	for(f = fputab; f < fe; f++)
		f->count = 0;
}
