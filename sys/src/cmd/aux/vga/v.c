void
setupregs(VGAmode *v) {
	int i;
	char buf[200];
	int lock;

	srout(0x01, v->sequencer[1] | 0x20);	/*turn display off*/
	sleep(2*500);
	switch (v->type) {
	case tseng:
		unlocktseng();
		crout(0x11, crin(0x11) & 0x7f); /*unlock crt regs*/
		outb(0x3cd, 0x00);		/* segment select */
		break;
	case pvga1a:
		lock = grin(0xf);
		grout(0xf, 5);
		break;
	case ati:
		outb(0x3cc, 0);	/* unlock svga? */
		outb(0x3ca, 1);
		break;
	}
	srout(0x00, srin(0x00) & 0xFD);	/* synchronous reset*/
	outb(EMISCW, v->general[0]);
	outb(0x3ca, v->general[1]);

	for(i = 0; i < sizeof(v->sequencer); i++)
		if (i != 1) /* we do #1 later */
			srout(i, v->sequencer[i]);
	/*
	 * We don't change the palette values
	 */
	for(i = 16; i < sizeof(v->attribute); i++)
		arout(i, v->attribute[i]);
	crout(0x11, crin(0x11) & 0x7f); /*unlock crt regs*/
	for(i = 0; i < sizeof(v->crt); i++)
		crout(i, v->crt[i]);
	for(i = 0; i < sizeof(v->graphics); i++)
		grout(i, v->graphics[i]);
	outb(0x3C6,0xFF);	/* pel mask */
	outb(0x3C8,0x00);	/* pel write address */
	switch (v->type) {
	pvga1a:
		grout(0xf, lock);
		break;
	case tseng:
		unlocktseng();	/* unlock the key */
		crout(0x11, crin(0x11) & 0x7f);		/* unlock crt 0-7 and 35 */
		srout(0x06, v->specials.tseng.sr6);	/* state ctrl p 142 */
		srout(0x07, v->specials.tseng.sr7);	/* ditto, AuxillaryMode */
		i = inb(0x3da); /* reset flip-flop. inp stat 1*/
		arout(0x16, v->specials.tseng.ar16);	/* misc */
		arout(0x17, v->specials.tseng.ar17);	/* misc 1*/
		crout(0x31, v->specials.tseng.crt31);	/* extended start. */
		crout(0x32, v->specials.tseng.crt32);	/* extended start. */
		crout(0x33, v->specials.tseng.crt33);	/* extended start. */
		crout(0x34, v->specials.tseng.crt34);	/* stub: 46ee + other bits */
		crout(0x35, v->specials.tseng.crt35);	/* overflow bits */
		crout(0x36, v->specials.tseng.crt36);	/* overflow bits */
		crout(0x37, v->specials.tseng.crt37);	/* overflow bits */
		outb(0x3c3, v->specials.tseng.viden);
		break;
	case ati:
		for (i=0xa0; i<=0xbf; i++)
			extout(i, v->specials.ati.ext[i-0xa0]);
	}
	sleep(2*500);
	srout(0x01, v->sequencer[1]);	/* turn display back on */
}

