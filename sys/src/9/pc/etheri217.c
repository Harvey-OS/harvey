/*
 * i217 or 218 specific gunk.
 * optionally included by etherigbe.c
 */

enum {
	Smbctlpage	= 769,
	Smbctlreg	= 23,
	Smbfrcsmbus	= 1,
};

/* prevent hardware and firmware from changing registers */
static void
takeswflag(Ctlr *ctlr)
{
	int i;

	if (ctlr->type != i217)
		return;
	if (ctlr->regs[Extcnf] & Swflag)
		return;
	iprint("%æ: waiting for Swflag\n", ctlr);
	for (i = 10000; --i > 0 && !(ctlr->regs[Extcnf] & Swflag); ) {
		ctlr->regs[Extcnf] |= Swflag;
		regwrbarr(ctlr);
		microdelay(100);
	}
	if (i <= 0)
		iprint("%æ: can't get sw control\n", ctlr);
}

/* let hardware and firmware change registers */
static void
relswflag(Ctlr *ctlr)
{
	if (ctlr->type != i217)
		return;
	if (ctlr->regs[Extcnf] & Swflag) {
		ctlr->regs[Extcnf] &= Swflag;
		regwrbarr(ctlr);
	} else
		iprint("%æ: relswflag: swflag not set\n", ctlr);
}

/*
 *  try to appease the (cranky) 217 to transmit.  hasn't worked yet.
 */
static uint
coddle217tx(Ctlr *ctlr)
{
	uint r, tarc;
	uint *regs;

	if (ctlr->type != i217)
		return 0;

	regs = ctlr->regs;

	/* use only transmit queue 0 */
	tarc = regs[Tarc0] & Tarcpreserve;
	tarc |= 1<<27 | 1<<26 | 1<<24 | 1<<23;	/* netbsd undoc magic */
	tarc |= 1<<29 | 1<<28;			/* ich8 magic */
	regs[Tarc0] = tarc | Tarcount | Tarcmbo | Tarcqena;

	tarc = regs[Tarc1] & Tarcpreserve;
	tarc &= ~Tarcqena;
	tarc |= 1<<30 | 1<<28 | 1<<26 | 1<<24;	/* netbsd undoc magic */
	regs[Tarc1] = tarc | Tarcount | Tarcmbo;
	regs[Tdbah1] = regs[Tdbal1] = regs[Tdlen1] = 0;

//	r = 0;				/* manual suggests this */
	r = Mbo57 | Gran | 1<<HthreshSHIFT | 1<<LwthreshSHIFT |
		1<<WthreshSHIFT | 1<<PthreshSHIFT;
//	r = Gran;			/* works for pxe boot rom */
	regs[Txdctl1] = r;		/* erratum workaround */
	coherence();
	return r;
}

/* give offerings to the 217; hasn't made it work yet. */
static void
coddle217(Ctlr *ctlr)
{
	uint *regs;

	if (ctlr->type != i217)
		return;

	regs = ctlr->regs;
	regs[Itr] = 0;
	regs[Fextnvm4] &= ~MASK(3);
	regs[Fextnvm4] |= 7;	/* 8µs beacon duration; nvram gets it wrong */
	takeswflag(ctlr);
	regs[Ctrlext] &= ~(Frcmacsmbus|Iame|Inttmrsclrena|Phypden);
	regs[Ctrlext] |= Extmbo57;	/* netbsd undoc magic */
	regs[Ctrlext] |= Drvload;	/* from pxe boot rom */
	regs[Ctrl] &= ~(Tfce|Rfce);	/* no flow control */
	regs[Tkabgtxd] = 0x50000;	/* utter intel magic for glci */

	/* enabling ecc eliminates some strange interrupt causes */
	regs[Pbeccsts] |= Pbeccena;
	regs[Ctrl] |= Mehe;
	relswflag(ctlr);

	/* set wake-up regs to known values */
	regs[Wuc] &= ~(Apme|Pme_en|Apmpme);
	regs[Wufc] = 0;			/* should be no change from reset */
	regs[Lpic] = 0;			/* no energy-efficient ethernet (eee) */
	regs[Status] &= ~(1<<31);	/* magic */
	regwrbarr(ctlr);

	/* unforce SMBus mode in PHY */
	phypage217(ctlr, Smbctlpage);
	phywrite(ctlr, Smbctlreg, phyread(ctlr, Smbctlreg) & ~Smbfrcsmbus);
	phypage217(ctlr, 0);
	delay(1000);
	/*
	 * we have verified that the EEE-enabling bits in page 772 regs 18 &
	 * 20 are off.
	 */
}

/* alleged link stall fix via undoc phy reg. */
static void
unstall217(Ctlr *ctlr)
{
	if (ctlr->type != i217)
		return;
	phypage217(ctlr, I217icpage);
	phywrite(ctlr, 19, 0x0100);
	phypage217(ctlr, 0);		/* reset page to usual 0 */
}

static ulong
eccerr217(Ctlr *ctlr, ulong icr, ulong im)
{
	static ulong oddities;

	if (ctlr->type != i217)
		return im;
	if (icr & Parityeccrst) {
		if (oddities++ == 0)
			iprint("%æ: parity or ecc error, or me reset; "
				"icr %#lux pbeccsts %#ux\n", ctlr, icr,
				ctlr->regs[Pbeccsts]);
		im &= ~Parityeccrst;
	}
	return im;
}
