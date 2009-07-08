#include <u.h>
#include <libc.h>
#include <bio.h>
#include <mach.h>
#define Extern extern
#include "power.h"

#define	prof profqi
#define Percent(num, max)	(int)(((vlong)(num)*100)/(max))

Inset *tables[] = { &ops0, &ops19, &ops31, &ops59, &ops63a, &ops63b, 0 };

void
isum(void)
{
	Inst *i;
	int pct, j, k;
	int total, loads, stores, arith, branch;
	int taken, powerreg, syscall, realarith, control;

	total = 0;
	loads = 0;
	stores = 0;
	arith = 0;
	branch = 0;
	taken = 0;
	powerreg = 0;
	syscall = 0;
	realarith = 0;
	control = 0;

	/* Compute the total so we can have percentages */
	for(j = 0; tables[j]; j++)
		for(k = tables[j]->nel; --k >= 0;) {
			i = &tables[j]->tab[k];
			if(i->name && i->func)
				total += i->count;
		}

	Bprint(bioout, "\nInstruction summary.\n\n");

	for(j = 0; tables[j]; j++) {
		for(k =tables[j]->nel; --k>=0; ) {
			i = &tables[j]->tab[k];
			if(i->name && i->func) {
				if(i->count == 0)
					continue;
				pct = Percent(i->count, total);
				if(pct != 0)
					Bprint(bioout, "%-8ud %3d%% %s\n",
					i->count, Percent(i->count, total), i->name);
				else
					Bprint(bioout, "%-8ud      %s\n",
					i->count, i->name);
	
				switch(i->type) {
				default:
					fatal(0, "isum bad stype %d\n", i->type);
				case Iload:
					loads += i->count;
					break;
				case Istore:
					stores += i->count;
					break;
				case Ilog:
				case Iarith:
					arith += i->count;
					break;
				case Ibranch:
					branch += i->count;
					taken += i->taken;
					break;
				case Ireg:
					powerreg += i->count;
					break;
				case Isyscall:
					syscall += i->count;
					break;
				case Ifloat:
					realarith += i->count;
					break;
				case Inop:
					arith += i->count;
					i->count -= nopcount;
					break;
				case Icontrol:
					control += i->count;
					break;
				}
			}
		}
	}

	Bprint(bioout, "\n%-8ud      Memory cycles\n", loads+stores+total);

	if(total == 0)
		return;

	Bprint(bioout, "%-8ud %3d%% Instruction cycles\n",
				total, Percent(total, loads+stores+total));

	Bprint(bioout, "%-8ud %3d%% Data cycles\n\n",
				loads+stores, Percent(loads+stores, loads+stores+total));	

	Bprint(bioout, "%-8ud %3d%% Stores\n", stores, Percent(stores, total));

	Bprint(bioout, "%-8ud %3d%% Loads\n", loads, Percent(loads, total));

	Bprint(bioout, "   %-8ud Store stall\n", stores*2);

	Bprint(bioout, "   %-8lud Load stall\n", loadlock);

	Bprint(bioout, "%-8ud %3d%% Arithmetic\n", arith, Percent(arith, total));

	Bprint(bioout, "%-8ud %3d%% Floating point\n",
					realarith, Percent(realarith, total));

	Bprint(bioout, "%-8ud %3d%% PowerPC special register load/stores\n",
					powerreg, Percent(powerreg, total));

	Bprint(bioout, "%-8ud %3d%% PowerPC control instructions\n",
					control, Percent(control, total));

	Bprint(bioout, "%-8ud %3d%% System calls\n", syscall, Percent(syscall, total));

	Bprint(bioout, "%-8ud %3d%% Branches\n", branch, Percent(branch, total));

	Bprint(bioout, "   %-8ud %3d%% Branches taken\n",
					taken, Percent(taken, branch));
}

char *stype[] = { "Stack", "Text", "Data", "Bss" };

void
segsum(void)
{
	Segment *s;
	int i;

	Bprint(bioout, "\n\nMemory Summary\n\n");
	Bprint(bioout, "      Base     End      Resident References\n");
	for(i = 0; i < Nseg; i++) {
		s = &memory.seg[i];
		Bprint(bioout, "%-5s %.8lux %.8lux %-8d %-8d\n",
				stype[i], s->base, s->end, s->rss*BY2PG, s->refs);
	}
}

typedef struct Prof Prof;
struct Prof
{
	Symbol	s;
	long	count;
};
Prof	prof[5000];

int
profcmp(void *a, void *b)
{
	return ((Prof*)b)->count - ((Prof*)a)->count;
}

void
iprofile(void)
{
	Prof *p, *n;
	int i, b, e;
	ulong total;
	extern ulong textbase;

	i = 0;
	p = prof;
	if(textsym(&p->s, i) == 0)
		return;
	i++;
	for(;;) {
		n = p+1;
		if(textsym(&n->s, i) == 0)
			break;
		b = (p->s.value-textbase)/PROFGRAN;
		e = (n->s.value-textbase)/PROFGRAN;
		while(b < e)
			p->count += iprof[b++];
		i++;
		p = n;
	}

	qsort(prof, i, sizeof(Prof), profcmp);

	total = 0;
	for(b = 0; b < i; b++)
		total += prof[b].count;

	Bprint(bioout, "  cycles     %% symbol          file\n");
	for(b = 0; b < i; b++) {
		if(prof[b].count == 0)
			continue;

		Bprint(bioout, "%8ld %3ld.%ld %-15s ",
			prof[b].count,
			100*prof[b].count/total,
			(1000*prof[b].count/total)%10,
			prof[b].s.name);

		printsource(prof[b].s.value);
		Bputc(bioout, '\n');
	}
	memset(prof, 0, sizeof(Prof)*i);
}
