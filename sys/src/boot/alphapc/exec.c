#include	"u.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"lib.h"

uchar	pcbpage[64*1024+sizeof(PCB)];
PCB		*pcb;

void	(*kentry)(Bootconf*);

void
gokernel(void)
{
	(*kentry)(&conf);
}

void
kexec(ulong entry)
{
	uvlong pcbb, paltype;

	pcb = (PCB*)(((ulong)pcbpage+0xffff) & ~0xffff);	/* page align, even on 64K page Alphas */
	memset(pcb, 0, sizeof(PCB));
	pcb->ksp = (uvlong)&entry;
	pcb->ptbr = getptbr();
	pcb->fen = 1;
	conf.pcb = pcb;
	pcbb = paddr((uvlong)pcb);
	kentry = (void(*)(Bootconf*))entry;
	paltype = 2;	/* OSF/1 please */
	switch (swppal(paltype, (uvlong)gokernel, pcbb, hwrpb->vptb, pcb->ksp)) {
	case 1:
		panic("unknown PALcode variant");
	case 2:
		panic("PALcode variant not loaded");
	default:
		panic("weird return status from swppal");
	}
}
