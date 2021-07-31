#include	<alef.h>
#include	"run.h"

intern Lock	block;

aggr Parrend
{
	int	nproc;
	Rendez;
};

int
ALEF_pfork(int n, byte **stack, Parrend *b)
{
	Task *t;
	Tdb *tdb;
	byte *st;
	int *semap;
	int id, nid, sema;

	memset(b, 0, sizeof(Parrend));
	b->nproc = n;

	id = 0;
	while(n) {
		st = ALEFalloc(ALEFstack+sizeof(Task)+sizeof(Tdb), 0);
		*stack++ = st+ALEFstack;

		memset(st, 0, sizeof(Task)+sizeof(Tdb));
		t = (Task*)st;
		t->stack = st;
		st += sizeof(Task);

		tdb = (Tdb*)st;
		tdb->ntask = 1;
		tdb->runhd = nil;
		tdb->ctask = t;
		t->tdb = tdb;

		sema = 1;
		/*
		 * Using &sema is required to prevent the optimiser
		 * from removing the assignment to sema
		 */
		semap = &sema;
		nid = id;
		if(rfork(RFNOWAIT|RFMEM|RFPROC) == 0) {
			PROC.tdb = tdb;
			*semap = 0;
			return nid;
		}
		while(*semap)
			;
		n--;
		id++;
	}
	*stack = nil;
	return id;
}

void
ALEF_pdone(Parrend *b, byte **stv)
{
	byte *p;

	while(b->nproc)
		b->Sleep(&(void*)b->nproc, 0);

	while(*stv) {
		p = *stv++;
		p -= ALEFstack;
		free(p);
	}
}

void
ALEF_pexit(Parrend *b)
{
	int x;

	block.lock();
	x = --b->nproc;
	block.unlock();
	if(x == 0)
		b->Wakeup();
	_exits(nil);
}
