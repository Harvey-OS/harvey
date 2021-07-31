#ifdef FS
#include "all.h"
#include "io.h"
#include "mem.h"
#include "../ip/ip.h"
#else

#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "io.h"
#include "../port/error.h"
#include "../port/netif.h"
#endif			/* FS */

#include "etherif.h"
#include "ethermii.h"

#include "compat.h"

int
mii(Mii* mii, int mask)
{
	MiiPhy *miiphy;
	int bit, phy, r, rmask;

	/*
	 * Probe through mii for PHYs in mask;
	 * return the mask of those found in the current probe.
	 * If the PHY has not already been probed, update
	 * the Mii information.
	 */
	rmask = 0;
	for(phy = 0; phy < NMiiPhy; phy++){
		bit = 1<<phy;
		if(!(mask & bit))
			continue;
		if(mii->mask & bit){
			rmask |= bit;
			continue;
		}
		if(mii->mir(mii, phy, Bmsr) == -1)
			continue;
		if((miiphy = malloc(sizeof(MiiPhy))) == nil)
			continue;

		miiphy->mii = mii;
		r = mii->mir(mii, phy, Phyidr1);
		miiphy->oui = (r & 0x3FFF)<<6;
		r = mii->mir(mii, phy, Phyidr2);
		miiphy->oui |= r>>10;
		miiphy->phy = phy;

		for(r = 0; r < NMiiPhyr; r++)
			miiphy->r[r] = mii->mir(mii, phy, r);

		mii->phy[phy] = miiphy;
		if(mii->curphy == nil)
			mii->curphy = miiphy;
		mii->mask |= bit;
		mii->nphy++;

		rmask |= bit;
	}
	return rmask;
}
