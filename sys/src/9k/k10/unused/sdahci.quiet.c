static int
ahciquiet(Aport *port)			/* unused! */
{
	ulong *p, i;

	p = &port->cmd;
	*p &= ~Ast;
	coherence();
	for (i = 500; i > 0 && *p & Acr; i -= 50)
		asleep(50);
	if(*p & Acr)		/* still running? */
		return -1;
	if(port->task & (ASdrq|ASbsy)) {
		*p |= Aclo;
		coherence();
		for (i = 500; i > 0 && *p & Aclo; i -= 50)
			asleep(50);
		if(*p & Aclo)
			return -1;

		/* extra check */
		dprint("ahci: clo clear %#lx\n", port->task);
		if(port->task & ASbsy)
			return -1;
	}
	*p |= Ast;
	return 0;
}

static int
ahcicomreset(Aportc *pc)		/* unused! */
{
	uchar *cfis;

	dprint("ahcicomreset\n");
	dreg("ahci: comreset ", pc->port);
	if(ahciquiet(pc->port) == -1){
		dprint("ahciquiet failed\n");
		return -1;
	}
	dreg("comreset ", pc->port);

	cfis = cfissetup(pc);
	cfis[1] = 0;
	cfis[15] = 1<<2;		/* srst */
	listsetup(pc, Lclear | Lreset);
	if(ahciexec(pc, 500) == -1){
		dprint("ahcicomreset: first command failed\n");
		return -1;
	}
	microdelay(250);
	dreg("comreset ", pc->port);

	cfis = cfissetup(pc);
	cfis[1] = 0;
	listsetup(pc, Lwrite);
	if(ahciexec(pc, 150) == -1){
		dprint("ahcicomreset: second command failed\n");
		return -1;
	}
	dreg("comreset ", pc->port);
	return 0;
}
