/* Pci/pcmcia code for wavelan.c */

#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "io.h"
#include "../port/error.h"
#include "../port/netif.h"
#include "etherif.h"

#include "wavelan.h"

static int
wavelanpcmciareset(Ether *ether)
{
	int i;
	char *p;
	Ctlr *ctlr;

	if((ctlr = malloc(sizeof(Ctlr))) == nil)
		return -1;

	ilock(ctlr);
	ctlr->ctlrno = ether->ctlrno;

	if (ether->port==0)
		ether->port=WDfltIOB;
	ctlr->iob = ether->port;

	if (ether->irq==0)
		ether->irq=WDfltIRQ;

	if (ioalloc(ether->port,WIOLen,0,"wavelan")<0){
	//	print("#l%d: port 0x%lx in use\n",
	//			ether->ctlrno, ether->port);
		goto abort1;
	}

	/*
	 * If id= is specified, card must match.  Otherwise try generic.
	 */
	ctlr->slot = -1;
	for(i=0; i<ether->nopt; i++){
		if(cistrncmp(ether->opt[i], "id=", 3) == 0){
			if((ctlr->slot = pcmspecial(&ether->opt[i][3], ether)) < 0)
				goto abort;
			break;
		}
	}
	if(ctlr->slot == -1){
		for (i=0; wavenames[i]; i++)
			if((ctlr->slot = pcmspecial(wavenames[i], ether))>=0)
				break;
		if(!wavenames[i]){
			DEBUG("no wavelan found\n");
			goto abort;
		}
	}

	// DEBUG("#l%d: port=0x%lx irq=%ld\n",
	//		ether->ctlrno, ether->port, ether->irq);

	if(wavelanreset(ether, ctlr) < 0){
	abort:
		iofree(ether->port);
	abort1:
		iunlock(ctlr);
		free(ctlr);
		ether->ctlr = nil;
		return -1;
	}

	for(i = 0; i < ether->nopt; i++){
		if(p = strchr(ether->opt[i], '='))
			*p = ' ';
		w_option(ctlr, ether->opt[i], strlen(ether->opt[i]));
	}

	iunlock(ctlr);
	return 0;
}

static struct {
	int vid;
	int did;
} wavelanpci[] = {
	0x1260, 0x3873,	/* Intersil Prism2.5 */
};

static Ctlr *ctlrhead, *ctlrtail;

static void
wavelanpciscan(void)
{
	int i;
	ulong pa;
	Pcidev *p;
	Ctlr *ctlr;

	p = nil;
	while(p = pcimatch(p, 0, 0)){
		for(i=0; i<nelem(wavelanpci); i++)
			if(p->vid == wavelanpci[i].vid && p->did == wavelanpci[i].did)
				break;
		if(i==nelem(wavelanpci))
			continue;

		/*
		 * On the Prism, bar[0] is the memory-mapped register address (4KB),
		 */
		if(p->mem[0].size != 4096){
			print("wavelanpci: %.4ux %.4ux: unlikely mmio size\n", p->vid, p->did);
			continue;
		}

		ctlr = malloc(sizeof(Ctlr));
		ctlr->pcidev = p;
		pa = upamalloc(p->mem[0].bar&~0xF, p->mem[0].size, 0);
		if(pa == 0){
			print("wavelanpci: %.4ux %.4ux: upamalloc 0x%.8lux %d failed\n", p->vid, p->did, p->mem[0].bar&~0xF, p->mem[0].size);
			free(ctlr);
			continue;
		}
		ctlr->mmb = (ushort*)KADDR(pa);
		if(ctlrhead != nil)
			ctlrtail->next = ctlr;
		else
			ctlrhead = ctlr;
		ctlrtail = ctlr;
		pcisetbme(p);
	}
}

static int
wavelanpcireset(Ether *ether)
{
	int i;
	char *p;
	Ctlr *ctlr;

	if(ctlrhead == nil)
		wavelanpciscan();

	/*
	 * Allow plan9.ini to set vid, did?
	 */
	for(ctlr=ctlrhead; ctlr!=nil; ctlr=ctlr->next)
		if(ctlr->active == 0)
			break;
	if(ctlr == nil)
		return -1;

	ctlr->active = 1;
	ilock(ctlr);
	ether->irq = ctlr->pcidev->intl;
	ether->tbdf = ctlr->pcidev->tbdf;

	/*
	 * Really hard reset.
	 */
	csr_outs(ctlr, WR_PciCor, 0x0080);
	delay(250);
	csr_outs(ctlr, WR_PciCor, 0x0000);
	delay(500);
	for(i=0; i<2*10; i++){
		if(!(csr_ins(ctlr, WR_Cmd)&WCmdBusy))
			break;
		delay(100);
	}
	if(i >= 2*10)
		print("wavelan pci %.4ux %.4ux: reset timeout %.4ux\n",
			ctlr->pcidev->vid, ctlr->pcidev->did, csr_ins(ctlr, WR_Cmd));

	if(wavelanreset(ether, ctlr) < 0){
		iunlock(ctlr);
		ether->ctlr = nil;
		return -1;
	}

	for(i = 0; i < ether->nopt; i++){
		if(p = strchr(ether->opt[i], '='))
			*p = ' ';
		w_option(ctlr, ether->opt[i], strlen(ether->opt[i]));
	}
	iunlock(ctlr);
	return 0;
}
	
void
etherwavelanlink(void)
{
	addethercard("wavelan", wavelanpcmciareset);
	addethercard("wavelanpci", wavelanpcireset);
}
