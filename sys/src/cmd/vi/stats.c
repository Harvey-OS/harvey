#include <u.h>
#include <libc.h>
#include <bio.h>
#include <mach.h>
#define Extern extern
#include "mips.h"

#define prof profvi
#define Percent(num, max)	(((num)*100)/(max))

extern Inst itab[], ispec[], cop1[];
Inst *tables[] = { itab, ispec, cop1, 0 };

void
isum(void)
{
	Inst *i;
	int total, loads, stores, arith, branch, realarith;
	int useddelay, taken, mipreg, syscall;
	int ldsunused, ldsused, ltotal;
	int pct, j;

	total = 0;
	loads = 0;
	stores = 0;
	arith = 0;
	branch = 0;
	useddelay = 0;
	taken = 0;
	mipreg = 0;
	syscall = 0;
	realarith = 0;

	/* Compute the total so we can have percentages */
	for(i = itab; i->func; i++)
		if(i->name && i->count)
			total += i->count;

	for(i = ispec; i->func; i++) {
		if(i->name && i->count) {
		}
	}
	/* Compute the total so we can have percentages */
	for(j = 0; tables[j]; j++) {
		for(i = tables[j]; i->func; i++) {
			if(i->name && i->count) {
				/* Dont count floating point twice */
				if(strcmp(i->name, "cop1") == 0)	
					i->count = 0;
				else
					total += i->count;
			}
		}
	}

	Bprint(bioout, "\nInstruction summary.\n\n");

	for(j = 0; tables[j]; j++) {
		for(i =tables[j]; i->func; i++) {
			if(i->name) {
				/* This is gross */
				if(strcmp(i->name, INOPINST) == 0)
					i->count -= nopcount;
				if(i->count == 0)
					continue;
				pct = Percent(i->count, total);
				if(pct != 0)
					Bprint(bioout, "%-8ud %3d%% %s\n",
						i->count, Percent(i->count,
						total), i->name);
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
				case Iarith:
					arith += i->count;
					break;
				case Ibranch:
					branch += i->count;
					taken += i->taken;
					useddelay += i->useddelay;
					break;
				case Ireg:
					mipreg += i->count;
					break;
				case Isyscall:
					syscall += i->count;
					break;
				case Ifloat:
					realarith += i->count;
					break;
				}
		
			}
		}
	}

	Bprint(bioout, "\n%-8ud      Memory cycles\n", loads+stores+total);	
	Bprint(bioout, "%-8ud %3d%% Instruction cycles\n",
			total, Percent(total, loads+stores+total));
	Bprint(bioout, "%-8ud %3d%% Data cycles\n\n",
			loads+stores, Percent(loads+stores, loads+stores+total));	

	Bprint(bioout, "%-8ud %3d%% Stores\n", stores, Percent(stores, total));
	Bprint(bioout, "%-8ud %3d%% Loads\n", loads, Percent(loads, total));

	/* Delay slots for loads/stores */
	ldsunused = nopcount-(branch-useddelay);
	ldsused = loads-ldsunused;
	ltotal = ldsused + ldsunused;
	Bprint(bioout, "   %-8ud %3d%% Delay slots\n",
			ldsused, Percent(ldsused, ltotal));

	Bprint(bioout, "   %-8ud %3d%% Unused delay slots\n", 
			ldsunused, Percent(ldsunused, ltotal));

	Bprint(bioout, "%-8ud %3d%% Arithmetic\n",
			arith, Percent(arith, total));

	Bprint(bioout, "%-8ud %3d%% Floating point\n",
			realarith, Percent(realarith, total));

	Bprint(bioout, "%-8ud %3d%% Mips special register load/stores\n",
			mipreg, Percent(mipreg, total));

	Bprint(bioout, "%-8ud %3d%% System calls\n",
			syscall, Percent(syscall, total));

	Bprint(bioout, "%-8ud %3d%% Branches\n",
			branch, Percent(branch, total));

	Bprint(bioout, "   %-8ud %3d%% Branches taken\n",
			taken, Percent(taken, branch));

	Bprint(bioout, "   %-8ud %3d%% Delay slots\n",
			useddelay, Percent(useddelay, branch));

	Bprint(bioout, "   %-8ud %3d%% Unused delay slots\n", 
			branch-useddelay, Percent(branch-useddelay, branch));

	Bprint(bioout, "%-8ud %3d%% Program total delay slots\n",
			nopcount, Percent(nopcount, total));
}

void
tlbsum(void)
{
	if(tlb.on == 0)
		return;

	Bprint(bioout, "\n\nTlb summary\n");

	Bprint(bioout, "\n%-8d User entries\n", tlb.tlbsize);
	Bprint(bioout, "%-8d Accesses\n", tlb.hit+tlb.miss);
	Bprint(bioout, "%-8d Tlb hits\n", tlb.hit);
	Bprint(bioout, "%-8d Tlb misses\n", tlb.miss);
	Bprint(bioout, "%7d%% Hit rate\n", Percent(tlb.hit, tlb.hit+tlb.miss));
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
profcmp(void *va, void *vb)
{
	Prof *a, *b;

	a = va;
	b = vb;
	return b->count - a->count;
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
