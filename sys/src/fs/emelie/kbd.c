#include	"all.h"
#include	"mem.h"
#include	"io.h"
#include	"ureg.h"

enum {
	Data=		0x60,	/* data port */

	Status=		0x64,	/* status port */
	 Inready=	0x01,	/*  input character ready */
	 Outbusy=	0x02,	/*  output busy */
	 Sysflag=	0x04,	/*  system flag */
	 Cmddata=	0x08,	/*  cmd==0, data==1 */
	 Inhibit=	0x10,	/*  keyboard/mouse inhibited */
	 Minready=	0x20,	/*  mouse character ready */
	 Rtimeout=	0x40,	/*  general timeout */
	 Parity=	0x80,

	Cmd=		0x64,	/* command port (write only) */

	CTdata=		0x0,	/* chips & Technologies ps2 data port */
	CTstatus=	0x1,	/* chips & Technologies ps2 status port */
	 Enable=	1<<7,
	 Clear=		1<<6,
	 Error=		1<<5,
	 Intenable=	1<<4,
	 Reset=		1<<3,
	 Tready=	1<<2,
	 Rready=	1<<1,
	 Idle=		1<<0,

	Spec=	0x80,

	PF=	Spec|0x20,	/* num pad function key */
	View=	Spec|0x00,	/* view (shift window up) */
	KF=	Spec|0x40,	/* function key */
	Shift=	Spec|0x60,
	Break=	Spec|0x61,
	Ctrl=	Spec|0x62,
	Latin=	Spec|0x63,
	Caps=	Spec|0x64,
	Num=	Spec|0x65,
	Middle=	Spec|0x66,
	No=	0x00,		/* peter */

	Home=	KF|13,
	Up=	KF|14,
	Pgup=	KF|15,
	Print=	KF|16,
	Left=	View,
	Right=	View,
	End=	'\r',
	Down=	View,
	Pgdown=	View,
	Ins=	KF|20,
	Del=	0x7F,

	Rbutton=4,
	Mbutton=2,
	Lbutton=1,
};

uchar kbtab[] = 
{
[0x00]	No,	0x1b,	'1',	'2',	'3',	'4',	'5',	'6',
[0x08]	'7',	'8',	'9',	'0',	'-',	'=',	'\b',	'\t',
[0x10]	'q',	'w',	'e',	'r',	't',	'y',	'u',	'i',
[0x18]	'o',	'p',	'[',	']',	'\n',	Ctrl,	'a',	's',
[0x20]	'd',	'f',	'g',	'h',	'j',	'k',	'l',	';',
[0x28]	'\'',	'`',	Shift,	'\\',	'z',	'x',	'c',	'v',
[0x30]	'b',	'n',	'm',	',',	'.',	'/',	Shift,	'*',
[0x38]	Latin,	' ',	Ctrl,	KF|1,	KF|2,	KF|3,	KF|4,	KF|5,
[0x40]	KF|6,	KF|7,	KF|8,	KF|9,	KF|10,	Num,	KF|12,	'7',
[0x48]	'8',	'9',	'-',	'4',	'5',	'6',	'+',	'1',
[0x50]	'2',	'3',	'0',	'.',	Del,	No,	No,	KF|11,
[0x58]	KF|12,	No,	No,	No,	No,	No,	No,	No,
};

uchar kbtabshift[] =
{
[0x00]	No,	0x1b,	'!',	'@',	'#',	'$',	'%',	'^',
[0x08]	'&',	'*',	'(',	')',	'_',	'+',	'\b',	'\t',
[0x10]	'Q',	'W',	'E',	'R',	'T',	'Y',	'U',	'I',
[0x18]	'O',	'P',	'{',	'}',	'\n',	Ctrl,	'A',	'S',
[0x20]	'D',	'F',	'G',	'H',	'J',	'K',	'L',	':',
[0x28]	'"',	'~',	Shift,	'|',	'Z',	'X',	'C',	'V',
[0x30]	'B',	'N',	'M',	'<',	'>',	'?',	Shift,	'*',
[0x38]	Latin,	' ',	Ctrl,	KF|1,	KF|2,	KF|3,	KF|4,	KF|5,
[0x40]	KF|6,	KF|7,	KF|8,	KF|9,	KF|10,	Num,	KF|12,	'7',
[0x48]	'8',	'9',	'-',	'4',	'5',	'6',	'+',	'1',
[0x50]	'2',	'3',	'0',	'.',	No,	No,	No,	KF|11,
[0x58]	KF|12,	No,	No,	No,	No,	No,	No,	No,
};

uchar kbtabesc1[] =
{
[0x00]	No,	No,	No,	No,	No,	No,	No,	No,
[0x08]	No,	No,	No,	No,	No,	No,	No,	No,
[0x10]	No,	No,	No,	No,	No,	No,	No,	No,
[0x18]	No,	No,	No,	No,	'\n',	Ctrl,	No,	No,
[0x20]	No,	No,	No,	No,	No,	No,	No,	No,
[0x28]	No,	No,	Shift,	No,	No,	No,	No,	No,
[0x30]	No,	No,	No,	No,	No,	'/',	No,	Print,
[0x38]	Latin,	No,	No,	No,	No,	No,	No,	No,
[0x40]	No,	No,	No,	No,	No,	No,	Break,	Home,
[0x48]	Up,	Pgup,	No,	Left,	No,	Right,	No,	End,
[0x50]	Down,	Pgdown,	Ins,	Del,	No,	No,	No,	No,
[0x58]	No,	No,	No,	No,	No,	No,	No,	No,
};

static uchar ccc;
static int shift;

enum
{
	/* controller command byte */
	Cscs1=		(1<<6),		/* scan code set 1 */
	Cmousedis=	(1<<5),		/* mouse disable */
	Ckbddis=	(1<<4),		/* kbd disable */
	Csf=		(1<<2),		/* system flag */
	Cmouseint=	(1<<1),		/* mouse interrupt enable */
	Ckbdint=	(1<<0),		/* kbd interrupt enable */
};

/*
 *  wait for output no longer busy
 */
static int
outready(void)
{
	int tries;

	for(tries = 0; (inb(Status) & Outbusy); tries++){
		if(tries > 500)
			return -1;
		delay(2);
	}
	return 0;
}

/*
 *  wait for input
 */
static int
inready(void)
{
	int tries;

	for(tries = 0; !(inb(Status) & Inready); tries++){
		if(tries > 500)
			return -1;
		delay(2);
	}
	return 0;
}

/*
 *  ask 8042 to enable the use of address bit 20
 */
void
i8042a20(void)
{
	outready();
	outb(Cmd, 0xD1);
	outready();
	outb(Data, 0xDF);
	outready();
}

/*
 *  ask 8042 to reset the machine
 */
void
i8042reset(void)
{
	ushort *s = (ushort*)(KZERO|0x472);

	*s = 0x1234;		/* BIOS warm-boot flag */

	outready();
	outb(Cmd, 0xFE);	/* pulse reset line (means resend on AT&T machines) */
	outready();
}

/*
 *  keyboard processing
 */
int
kbdintr0(void)
{
	int s, c;
	static int esc1, esc2;
	static int caps;
	static int ctl;
	static int num;
	int keyup;

	/*
	 *  get status
	 */
	s = inb(Status);
	if(!(s&Inready))
		return -1;

	/*
	 *  get the character
	 */
	c = inb(Data);

	/*
	 *  e0's is the first of a 2 character sequence
	 */
	if(c == 0xe0){
		esc1 = 1;
		return -1;
	} else if(c == 0xe1){
		esc2 = 2;
		return -1;
	}

	keyup = c&0x80;
	c &= 0x7f;
	if(c > sizeof kbtab){
		print("unknown key %ux\n", c|keyup);
		return -1;
	}

	if(esc1){
		c = kbtabesc1[c];
		esc1 = 0;
	} else if(esc2){
		esc2--;
		return -1;
	} else if(shift)
		c = kbtabshift[c];
	else
		c = kbtab[c];

	if(caps && c<='z' && c>='a')
		c += 'A' - 'a';

	/*
	 *  keyup only important for shifts
	 */
	if(keyup){
		switch(c){
		case Shift:
			shift = 0;
			break;
		case Ctrl:
			ctl = 0;
			break;
		}
		return -1;
	}

	/*
 	 *  normal character
	 */
	if(!(c & Spec)){
		if(ctl)
			c &= 0x1f;
		return c;
	} else {
		switch(c){
		case Caps:
			caps ^= 1;
			return -1;
		case Num:
			num ^= 1;
			return -1;
		case Shift:
			shift = 1;
			return -1;
		case Ctrl:
			ctl = 1;
			return -1;
		}
	}
	return c;
}

static void
kbdintr(Ureg *ur, void *v)
{
	int c;

	USED(ur, v);
	if((c = kbdintr0()) >= 0)
		kbdchar(c);
}

int
kbdgetc(void)
{
	int c;

	if((c = kbdintr0()) < 0)
		return 0;
	return c;
}

void
kbdinit(void)
{
	int c;

	setvec(Kbdvec, kbdintr, 0);

	/* wait for a quiescent controller */
	while((c = inb(Status)) & (Outbusy | Inready))
		if(c & Inready)
			inb(Data);

	/* get current controller command byte */
	outb(Cmd, 0x20);
	if(inready() < 0){
		print("kbdinit: can't read ccc\n");
		ccc = 0;
	} else
		ccc = inb(Data);

	/* enable kbd xfers and interrupts */
	ccc &= ~Ckbddis;
	ccc |= Csf | Ckbdint | Cscs1;
	if(outready() < 0)
		print("kbd init failed\n");
	outb(Cmd, 0x60);
	if(outready() < 0)
		print("kbd init failed\n");
	outb(Data, ccc);
	outready();
}
