#include	"u.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"lib.h"

static int	debug;

enum {
	Maxbank = 2,
};

Bank	bank[Maxbank];
int	nbank;

void
meminit(void)
{
	Memdsc *mem;
	Memclust *c;
	int i;
	uvlong npage, p0, p1;
	extern ulong _main[], edata[];

	mem = (Memdsc*)((ulong)hwrpb + hwrpb->memoff);
	if (debug)
		print("\nnumber of clusters: %lld\n", mem->nclust);
	npage = 0;
	conf.maxphys = 0;
	for (i = 0; i < mem->nclust; i++) {
		c = &mem->clust[i];
		p0 = c->pfn*hwrpb->by2pg;
		p1 = (c->pfn+c->npages)*hwrpb->by2pg;
		if (debug) {
			print("clust%d: %llux-%llux, tested %llud/%llud vabitm %llux usage %llux\n",
				i, p0, p1, c->ntest, c->npages, c->vabitm, c->usage);
			if (c->vabitm)
				print("\tfirst 64 pages: %llux\n", *(uvlong*)c->vabitm);
		}
		npage += c->npages;
		if (p1 > conf.maxphys)
			conf.maxphys = p1;
		switch ((ulong)c->usage&3) {
		case 0:
			if (nbank >= Maxbank) {
				print("increase Maxbank; lost %lldMB\n", c->npages*hwrpb->by2pg/MB);
				break;
			}
			bank[nbank].min = p0;
			bank[nbank].max = p1;
			nbank++;
			break;
		case 2:
			print("nvram skipped\n");
			break;
		}
	}
	if (debug)
		print("\n");

	print("Memory size: %lludMB\n", npage*hwrpb->by2pg/MB);
	print("\n");

	/* kernel virtual space = 2G.  leave room for kmapio */
	if (conf.maxphys > 1024*MB) {
		print("meminit: too much physical memory; only first gigabyte mapped\n\n");
		conf.maxphys = 1024*MB;
	}

	conf.nbank = nbank;
	conf.bank = bank;
}

int
validrgn(ulong min, ulong max)
{
	int i;

	min &= ~KZERO;
	max &= ~KZERO;
	for (i = 0; i < nbank; i++)
		if (bank[i].min <= min && max <= bank[i].max)
			return 1;
	return 0;
}

uvlong
allocate(int pages)
{
	uvlong top, len;
	int from, i;

	top = 0;
	len = pages*hwrpb->by2pg;
	from = -1;
	for (i = 0; i < nbank; i++)
		if (bank[i].max - bank[i].min >= len && bank[i].max > top) {
			top = bank[i].max;
			from = i;
		}
	if (from < 0)
		return 0;
	bank[from].max -= len;
	conf.bank[from].max = bank[from].max;
	for (i = 0; i < len>>3; i++)
		stqp(bank[from].max+8*i, 0);
	return bank[from].max;
}
