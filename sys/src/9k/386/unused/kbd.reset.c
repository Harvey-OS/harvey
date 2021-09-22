/*
 *  ask 8042 to reset the machine
 */
void
i8042reset(void)
{
	ushort *s = KADDR(0x472);
	int i, x;

	if(nokbd)
		return;

	*s = 0x1234;		/* BIOS warm-boot flag */

	/*
	 *  newer reset-the-machine command
	 */
	outready();
	outbyte(Cmd, 0xFE);

	/*
	 *  Pulse it by hand (old somewhat reliable)
	 */
	x = 0xDF;
	for(i = 0; i < 5; i++){
		x ^= 1;
		outready();
		outbyte(Cmd, 0xD1);
		outb(Data, x);	/* toggle reset */
		delay(100);
	}
}

int
i8042auxcmd(int cmd)
{
	unsigned int c;
	int tries;
	static int badkbd;

	if(badkbd)
		return -1;
	c = tries = 0;
	ilock(&i8042lock);
	do{
		if(tries++ > 2 ||
		    outready() < 0 ||
		    outbyte(Cmd, 0xD4) < 0 ||
		    outbyte(Data, cmd) < 0 ||
		    inready() < 0)
			break;
		c = inb(Data);
	} while(c == 0xFE || c == 0);
	iunlock(&i8042lock);

	if(c != 0xFA){
		print("i8042: %2.2ux returned to the %2.2ux command\n", c, cmd);
		badkbd = 1;	/* don't keep trying; there might not be one */
		return -1;
	}
	return 0;
}

int
i8042auxcmds(uchar *cmd, int ncmd)
{
	int i;

	ilock(&i8042lock);
	for(i=0; i<ncmd; i++){
		if(outready() < 0 ||
		    outbyte(Cmd, 0xD4) < 0)
			break;
		outb(Data, cmd[i]);
	}
	iunlock(&i8042lock);
	return i;
}

void
i8042auxenable(void (*putc)(int, int))
{
	static char err[] = "i8042: aux init failed\n";

	/* enable kbd/aux xfers and interrupts */
	ccc &= ~Cauxdis;
	ccc |= Cauxint;

	ilock(&i8042lock);
	if(outready() < 0 ||
	    outbyte(Cmd, 0x60) < 0 ||	/* write control register */
	    outbyte(Data, ccc) < 0)
		print(err);
	if(outbyte(Cmd, 0xA8) >= 0){	/* auxiliary device enabled ok? */
		auxputc = putc;
		intrenable(IrqAUX, i8042intr, 0, BUSUNKNOWN, "kbdaux");
	}
	iunlock(&i8042lock);
}
