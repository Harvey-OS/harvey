#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"

#include "../port/netif.h"

#include "ethermii.h"

static int
miiprobe(Mii* mii, int mask)
{
	MiiPhy *miiphy;
	int bit, oui, phyno, r, rmask;

	/*
	 * Probe through mii for PHYs in mask;
	 * return the mask of those found in the current probe.
	 * If the PHY has not already been probed, update
	 * the Mii information.
	 */
	rmask = 0;
	for(phyno = 0; phyno < NMiiPhy; phyno++){
		bit = 1<<phyno;
		if(!(mask & bit))
			continue;
		if(mii->mask & bit){
			rmask |= bit;
			continue;
		}
		if(mii->rw(mii, 0, phyno, Bmsr, 0) == -1)
			continue;
		r = mii->rw(mii, 0, phyno, Phyidr1, 0)<<16;
		r |= mii->rw(mii, 0, phyno, Phyidr2, 0);
		oui = (r>>10) & 0xffff;
		if(oui == 0xffff || oui == 0)
			continue;

		if((miiphy = malloc(sizeof(MiiPhy))) == nil)
			continue;

		miiphy->mii = mii;
		miiphy->phyno = phyno;
		miiphy->phyid = r;
		miiphy->oui = oui;

		miiphy->anar = ~0;
		miiphy->fc = ~0;
		miiphy->mscr = ~0;

		mii->phy[phyno] = miiphy;
		if(mii->curphy == nil)
			mii->curphy = miiphy;
		mii->mask |= bit;
		mii->nphy++;

		rmask |= bit;
	}
	return rmask;
}

int
miimir(Mii* mii, int r)
{
	if(mii == nil || mii->ctlr == nil || mii->curphy == nil)
		return -1;
	return mii->rw(mii, 0, mii->curphy->phyno, r, 0);
}

int
miimiw(Mii* mii, int r, int data)
{
	if(mii == nil || mii->ctlr == nil || mii->curphy == nil)
		return -1;
	return mii->rw(mii, 1, mii->curphy->phyno, r, data);
}

int
miireset(Mii* mii)
{
	int bmcr, timeo;

	if(mii == nil || mii->ctlr == nil || mii->curphy == nil)
		return -1;
	bmcr = mii->rw(mii, 0, mii->curphy->phyno, Bmcr, 0);
	mii->rw(mii, 1, mii->curphy->phyno, Bmcr, BmcrR|bmcr);
	for(timeo = 0; timeo < 1000; timeo++){
		bmcr = mii->rw(mii, 0, mii->curphy->phyno, Bmcr, 0);
		if(!(bmcr & BmcrR))
			break;
		microdelay(1);
	}
	if(bmcr & BmcrR)
		return -1;
	if(bmcr & BmcrI)
		mii->rw(mii, 1, mii->curphy->phyno, Bmcr, bmcr & ~BmcrI);
	return 0;
}

int
miiane(Mii* mii, int a, int p, int e)
{
	int anar, bmsr, mscr, r, phyno;

	if(mii == nil || mii->ctlr == nil || mii->curphy == nil)
		return -1;
	phyno = mii->curphy->phyno;

	mii->rw(mii, 1, phyno, Bmsr, 0);
	bmsr = mii->rw(mii, 0, phyno, Bmsr, 0);
	if(!(bmsr & BmsrAna))
		return -1;

	if(a != ~0)
		anar = (AnaTXFD|AnaTXHD|Ana10FD|Ana10HD) & a;
	else if(mii->curphy->anar != ~0)
		anar = mii->curphy->anar;
	else{
		anar = mii->rw(mii, 0, phyno, Anar, 0);
		anar &= ~(AnaAP|AnaP|AnaT4|AnaTXFD|AnaTXHD|Ana10FD|Ana10HD);
		if(bmsr & Bmsr10THD)
			anar |= Ana10HD;
		if(bmsr & Bmsr10TFD)
			anar |= Ana10FD;
		if(bmsr & Bmsr100TXHD)
			anar |= AnaTXHD;
		if(bmsr & Bmsr100TXFD)
			anar |= AnaTXFD;
	}
	mii->curphy->anar = anar;

	if(p != ~0)
		anar |= (AnaAP|AnaP) & p;
	else if(mii->curphy->fc != ~0)
		anar |= mii->curphy->fc;
	mii->curphy->fc = (AnaAP|AnaP) & anar;

	if(bmsr & BmsrEs){
		mscr = mii->rw(mii, 0, phyno, Mscr, 0);
		mscr &= ~(Mscr1000TFD|Mscr1000THD);
		if(e != ~0)
			mscr |= (Mscr1000TFD|Mscr1000THD) & e;
		else if(mii->curphy->mscr != ~0)
			mscr = mii->curphy->mscr;
		else{
			r = mii->rw(mii, 0, phyno, Esr, 0);
			if(r & Esr1000THD)
				mscr |= Mscr1000THD;
			if(r & Esr1000TFD)
				mscr |= Mscr1000TFD;
		}
		mii->curphy->mscr = mscr;
		mii->rw(mii, 1, phyno, Mscr, mscr);
	}
	else
		mii->curphy->mscr = 0;
	mii->rw(mii, 1, phyno, Anar, anar);

	r = mii->rw(mii, 0, phyno, Bmcr, 0);
	if(!(r & BmcrR)){
		r |= BmcrAne|BmcrRan;
		mii->rw(mii, 1, phyno, Bmcr, r);
	}

	return 0;
}

int
miistatus(Mii* mii)
{
	MiiPhy *phy;
	int anlpar, bmsr, p, r, phyno;

	if(mii == nil || mii->ctlr == nil || mii->curphy == nil)
		return -1;
	phy = mii->curphy;
	phyno = phy->phyno;

	/*
	 * Check Auto-Negotiation is complete and link is up.
	 * (Read status twice as the Ls bit is sticky).
	 */
	bmsr = mii->rw(mii, 0, phyno, Bmsr, 0);
	if(!(bmsr & (BmsrAnc|BmsrAna)))
		return -1;

	bmsr = mii->rw(mii, 0, phyno, Bmsr, 0);
	if(!(bmsr & BmsrLs)){
		phy->link = 0;
		return -1;
	}

	phy->speed = phy->fd = phy->rfc = phy->tfc = 0;
	if(phy->mscr){
		r = mii->rw(mii, 0, phyno, Mssr, 0);
		if((phy->mscr & Mscr1000TFD) && (r & Mssr1000TFD)){
			phy->speed = 1000;
			phy->fd = 1;
		}
		else if((phy->mscr & Mscr1000THD) && (r & Mssr1000THD))
			phy->speed = 1000;
	}

	anlpar = mii->rw(mii, 0, phyno, Anlpar, 0);
	if(phy->speed == 0){
		r = phy->anar & anlpar;
		if(r & AnaTXFD){
			phy->speed = 100;
			phy->fd = 1;
		}
		else if(r & AnaTXHD)
			phy->speed = 100;
		else if(r & Ana10FD){
			phy->speed = 10;
			phy->fd = 1;
		}
		else if(r & Ana10HD)
			phy->speed = 10;
	}
	if(phy->speed == 0)
		return -1;

	if(phy->fd){
		p = phy->fc;
		r = anlpar & (AnaAP|AnaP);
		if(p == AnaAP && r == (AnaAP|AnaP))
			phy->tfc = 1;
		else if(p == (AnaAP|AnaP) && r == AnaAP)
			phy->rfc = 1;
		else if((p & AnaP) && (r & AnaP))
			phy->rfc = phy->tfc = 1;
	}

	phy->link = 1;

	return 0;
}

char*
miidumpphy(Mii* mii, char* p, char* e)
{
	int i, r;

	if(mii == nil || mii->curphy == nil)
		return p;

	p = seprint(p, e, "phy:   ");
	for(i = 0; i < NMiiPhyr; i++){
		if(i && ((i & 0x07) == 0))
			p = seprint(p, e, "\n       ");
		r = mii->rw(mii, 0, mii->curphy->phyno, i, 0);
		p = seprint(p, e, " %4.4ux", r);
	}
	p = seprint(p, e, "\n");

	return p;
}

void
miidetach(Mii* mii)
{
	int i;

	for(i = 0; i < NMiiPhy; i++){
		if(mii->phy[i] == nil)
			continue;
		free(mii);
		mii->phy[i] = nil;
	}
	free(mii);
}

Mii*
miiattach(void* ctlr, int mask, int (*rw)(Mii*, int, int, int, int))
{
	Mii* mii;

	if((mii = malloc(sizeof(Mii))) == nil)
		return nil;
	mii->ctlr = ctlr;
	mii->rw = rw;

	if(miiprobe(mii, mask) == 0){
		free(mii);
		mii = nil;
	}

	return mii;
}
