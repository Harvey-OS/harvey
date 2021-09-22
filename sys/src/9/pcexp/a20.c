/*
 * enable the A20 address line
 */

enum {
	/* keyboard controller ports & cmds */
	Data=		0x60,		/* data port */
	Status=		0x64,		/* status port */
	 Inready=	0x01,		/*  input character ready */
	 Outbusy=	0x02,		/*  output busy */
	Cmd=		0x64,		/* command port (write only) */
};

static void
delay(uint ms)				/* very rough approximation */
{
	int i;

	while(ms-- > 0)
		for(i = 1000*1000; i > 0; i--)
			;
}

static int
isaliased(ulong mb, ulong off)
{
	int r;
	ulong oz, omb1;
	ulong *zp, *mb1p;

	zp = (ulong *)(mb*MB + off);
	mb1p = (ulong *)((mb+1)*MB + off);
	oz = *zp;
	omb1 = *mb1p;

	*zp = 0x1234;
	*mb1p = 0x8765;
	mb586();
	wbinvd();
	r = *zp == *mb1p;

	*zp = oz;
	*mb1p = omb1;
	return r;
}

static int
isa20on(void)
{
	if (isaliased(0, 0) || isaliased(1, 512*KB) ||
	    isaliased(2, 512*KB) || isaliased(3, 512*KB))
		return 0;
	return 1;
}

static int
kbdinit(void)
{
	int c, try;

	/* wait for a quiescent controller */
	try = 500;
	while(try-- > 0 && (c = inb(Status)) & (Outbusy | Inready)) {
		if(c & Inready)
			inb(Data);
		delay(1);
	}
	return try <= 0? -1: 0;
}

/*
 *  wait for output no longer busy (but not forever,
 *  there might not be a keyboard controller).
 */
static void
outready(void)
{
	int i;

	for (i = 1000; i > 0 && inb(Status) & Outbusy; i--)
		delay(1);
}

/*
 *  ask 8042 to enable the use of address bit 20
 */
int
i8042a20(void)
{
	if (kbdinit() < 0)
		return -1;
	outready();
	outb(Cmd, 0xD1);
	outready();
	outb(Data, 0xDF);
	outready();
	return 0;
}

int
a20init(void)
{
	int b;

	if (isa20on())
		return 0;		/* "bios"; */
	if (i8042a20() >= 0)		/* original method */
		return 0;		/* "kbd ctlr"; */

	/* newer methods, last resorts */
	b = inb(Sysctla);
	if (!(b & Sysctla20ena))
		outb(Sysctla, (b & ~Sysctlreset) | Sysctla20ena);
	if (isa20on())
		return 0;		/* "port 0x92"; */

	b = inb(0xee);
	USED(b);
	if (isa20on())
		return 0;		/* "port 0xee"; */
	return -1;
}

