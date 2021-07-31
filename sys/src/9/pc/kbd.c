/*
 * keyboard input
 */
#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"io.h"
#include	"../port/error.h"

enum {
	Data=		0x60,		/* data port */

	Status=		0x64,		/* status port */
	 Inready=	0x01,		/*  input character ready */
	 Outbusy=	0x02,		/*  output busy */
	 Sysflag=	0x04,		/*  system flag */
	 Cmddata=	0x08,		/*  cmd==0, data==1 */
	 Inhibit=	0x10,		/*  keyboard/mouse inhibited */
	 Minready=	0x20,		/*  mouse character ready */
	 Rtimeout=	0x40,		/*  general timeout */
	 Parity=	0x80,

	Cmd=		0x64,		/* command port (write only) */

	Spec=		0xF800,		/* Unicode private space */
	PF=		Spec|0x20,	/* num pad function key */
	View=		Spec|0x00,	/* view (shift window up) */
	KF=		0xF000,		/* function key (begin Unicode private space) */
	Shift=		Spec|0x60,
	Break=		Spec|0x61,
	Ctrl=		Spec|0x62,
	Latin=		Spec|0x63,
	Caps=		Spec|0x64,
	Num=		Spec|0x65,
	Middle=		Spec|0x66,
	Altgr=		Spec|0x67,
	Kmouse=		Spec|0x100,
	No=		0x00,		/* peter */

	Home=		KF|13,
	Up=		KF|14,
	Pgup=		KF|15,
	Print=		KF|16,
	Left=		KF|17,
	Right=		KF|18,
	End=		KF|24,
	Down=		View,
	Pgdown=		KF|19,
	Ins=		KF|20,
	Del=		0x7F,
	Scroll=		KF|21,

	Nscan=	128,

	Int=	0,			/* kbscans indices */
	Ext,
	Nscans,
};

/*
 * The codes at 0x79 and 0x7b are produced by the PFU Happy Hacking keyboard.
 * A 'standard' keyboard doesn't produce anything above 0x58.
 */
Rune kbtab[Nscan] = 
{
[0x00]	No,	0x1b,	'1',	'2',	'3',	'4',	'5',	'6',
[0x08]	'7',	'8',	'9',	'0',	'-',	'=',	'\b',	'\t',
[0x10]	'q',	'w',	'e',	'r',	't',	'y',	'u',	'i',
[0x18]	'o',	'p',	'[',	']',	'\n',	Ctrl,	'a',	's',
[0x20]	'd',	'f',	'g',	'h',	'j',	'k',	'l',	';',
[0x28]	'\'',	'`',	Shift,	'\\',	'z',	'x',	'c',	'v',
[0x30]	'b',	'n',	'm',	',',	'.',	'/',	Shift,	'*',
[0x38]	Latin,	' ',	Ctrl,	KF|1,	KF|2,	KF|3,	KF|4,	KF|5,
[0x40]	KF|6,	KF|7,	KF|8,	KF|9,	KF|10,	Num,	Scroll,	'7',
[0x48]	'8',	'9',	'-',	'4',	'5',	'6',	'+',	'1',
[0x50]	'2',	'3',	'0',	'.',	No,	No,	No,	KF|11,
[0x58]	KF|12,	No,	No,	No,	No,	No,	No,	No,
[0x60]	No,	No,	No,	No,	No,	No,	No,	No,
[0x68]	No,	No,	No,	No,	No,	No,	No,	No,
[0x70]	No,	No,	No,	No,	No,	No,	No,	No,
[0x78]	No,	View,	No,	Up,	No,	No,	No,	No,
};

Rune kbtabshift[Nscan] =
{
[0x00]	No,	0x1b,	'!',	'@',	'#',	'$',	'%',	'^',
[0x08]	'&',	'*',	'(',	')',	'_',	'+',	'\b',	'\t',
[0x10]	'Q',	'W',	'E',	'R',	'T',	'Y',	'U',	'I',
[0x18]	'O',	'P',	'{',	'}',	'\n',	Ctrl,	'A',	'S',
[0x20]	'D',	'F',	'G',	'H',	'J',	'K',	'L',	':',
[0x28]	'"',	'~',	Shift,	'|',	'Z',	'X',	'C',	'V',
[0x30]	'B',	'N',	'M',	'<',	'>',	'?',	Shift,	'*',
[0x38]	Latin,	' ',	Ctrl,	KF|1,	KF|2,	KF|3,	KF|4,	KF|5,
[0x40]	KF|6,	KF|7,	KF|8,	KF|9,	KF|10,	Num,	Scroll,	'7',
[0x48]	'8',	'9',	'-',	'4',	'5',	'6',	'+',	'1',
[0x50]	'2',	'3',	'0',	'.',	No,	No,	No,	KF|11,
[0x58]	KF|12,	No,	No,	No,	No,	No,	No,	No,
[0x60]	No,	No,	No,	No,	No,	No,	No,	No,
[0x68]	No,	No,	No,	No,	No,	No,	No,	No,
[0x70]	No,	No,	No,	No,	No,	No,	No,	No,
[0x78]	No,	Up,	No,	Up,	No,	No,	No,	No,
};

Rune kbtabesc1[Nscan] =
{
[0x00]	No,	No,	No,	No,	No,	No,	No,	No,
[0x08]	No,	No,	No,	No,	No,	No,	No,	No,
[0x10]	No,	No,	No,	No,	No,	No,	No,	No,
[0x18]	No,	No,	No,	No,	'\n',	Ctrl,	No,	No,
[0x20]	No,	No,	No,	No,	No,	No,	No,	No,
[0x28]	No,	No,	Shift,	No,	No,	No,	No,	No,
[0x30]	No,	No,	No,	No,	No,	'/',	No,	Print,
[0x38]	Altgr,	No,	No,	No,	No,	No,	No,	No,
[0x40]	No,	No,	No,	No,	No,	No,	Break,	Home,
[0x48]	Up,	Pgup,	No,	Left,	No,	Right,	No,	End,
[0x50]	Down,	Pgdown,	Ins,	Del,	No,	No,	No,	No,
[0x58]	No,	No,	No,	No,	No,	No,	No,	No,
[0x60]	No,	No,	No,	No,	No,	No,	No,	No,
[0x68]	No,	No,	No,	No,	No,	No,	No,	No,
[0x70]	No,	No,	No,	No,	No,	No,	No,	No,
[0x78]	No,	Up,	No,	No,	No,	No,	No,	No,
};

Rune kbtabaltgr[Nscan] =
{
[0x00]	No,	No,	No,	No,	No,	No,	No,	No,
[0x08]	No,	No,	No,	No,	No,	No,	No,	No,
[0x10]	No,	No,	No,	No,	No,	No,	No,	No,
[0x18]	No,	No,	No,	No,	'\n',	Ctrl,	No,	No,
[0x20]	No,	No,	No,	No,	No,	No,	No,	No,
[0x28]	No,	No,	Shift,	No,	No,	No,	No,	No,
[0x30]	No,	No,	No,	No,	No,	'/',	No,	Print,
[0x38]	Altgr,	No,	No,	No,	No,	No,	No,	No,
[0x40]	No,	No,	No,	No,	No,	No,	Break,	Home,
[0x48]	Up,	Pgup,	No,	Left,	No,	Right,	No,	End,
[0x50]	Down,	Pgdown,	Ins,	Del,	No,	No,	No,	No,
[0x58]	No,	No,	No,	No,	No,	No,	No,	No,
[0x60]	No,	No,	No,	No,	No,	No,	No,	No,
[0x68]	No,	No,	No,	No,	No,	No,	No,	No,
[0x70]	No,	No,	No,	No,	No,	No,	No,	No,
[0x78]	No,	Up,	No,	No,	No,	No,	No,	No,
};

Rune kbtabctrl[Nscan] =
{
[0x00]	No,	'', 	'', 	'', 	'', 	'', 	'', 	'', 
[0x08]	'', 	'', 	'', 	'', 	'', 	'', 	'\b',	'\t',
[0x10]	'', 	'', 	'', 	'', 	'', 	'', 	'', 	'\t',
[0x18]	'', 	'', 	'', 	'', 	'\n',	Ctrl,	'', 	'', 
[0x20]	'', 	'', 	'', 	'\b',	'\n',	'', 	'', 	'', 
[0x28]	'', 	No, 	Shift,	'', 	'', 	'', 	'', 	'', 
[0x30]	'', 	'', 	'', 	'', 	'', 	'', 	Shift,	'\n',
[0x38]	Latin,	No, 	Ctrl,	'', 	'', 	'', 	'', 	'', 
[0x40]	'', 	'', 	'', 	'', 	'', 	'', 	'', 	'', 
[0x48]	'', 	'', 	'', 	'', 	'', 	'', 	'', 	'', 
[0x50]	'', 	'', 	'', 	'', 	No,	No,	No,	'', 
[0x58]	'', 	No,	No,	No,	No,	No,	No,	No,
[0x60]	No,	No,	No,	No,	No,	No,	No,	No,
[0x68]	No,	No,	No,	No,	No,	No,	No,	No,
[0x70]	No,	No,	No,	No,	No,	No,	No,	No,
[0x78]	No,	'', 	No,	'\b',	No,	No,	No,	No,
};

enum
{
	/* controller command byte */
	Cscs1=		(1<<6),		/* scan code set 1 */
	Cauxdis=	(1<<5),		/* mouse disable */
	Ckbddis=	(1<<4),		/* kbd disable */
	Csf=		(1<<2),		/* system flag */
	Cauxint=	(1<<1),		/* mouse interrupt enable */
	Ckbdint=	(1<<0),		/* kbd interrupt enable */
};

int mouseshifted;
void (*kbdmouse)(int);

static Lock i8042lock;
static uchar ccc;
static void (*auxputc)(int, int);
static int nokbd = 1;			/* flag: no PS/2 keyboard */

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
 *  ask 8042 to reset the machine
 */
void
i8042reset(void)
{
	int i, x;

	if(nokbd)
		return;

	*((ushort*)KADDR(0x472)) = 0x1234;	/* BIOS warm-boot flag */

	/*
	 *  newer reset the machine command
	 */
	outready();
	outb(Cmd, 0xFE);
	outready();

	/*
	 *  Pulse it by hand (old somewhat reliable)
	 */
	x = 0xDF;
	for(i = 0; i < 5; i++){
		x ^= 1;
		outready();
		outb(Cmd, 0xD1);
		outready();
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
	c = 0;
	tries = 0;

	ilock(&i8042lock);
	do{
		if(tries++ > 2)
			break;
		if(outready() < 0)
			break;
		outb(Cmd, 0xD4);
		if(outready() < 0)
			break;
		outb(Data, cmd);
		if(outready() < 0)
			break;
		if(inready() < 0)
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
		if(outready() < 0)
			break;
		outb(Cmd, 0xD4);
		if(outready() < 0)
			break;
		outb(Data, cmd[i]);
	}
	iunlock(&i8042lock);
	return i;
}

typedef struct Kbscan Kbscan;
struct Kbscan {
	int	esc1;
	int	esc2;
	int	alt;
	int	altgr;
	int	caps;
	int	ctl;
	int	num;
	int	shift;
	int	collecting;
	int	nk;
	Rune	kc[5];
	int	buttons;
};

Kbscan kbscans[Nscans];	/* kernel and external scan code state */

static int kdebug;

/*
 * set keyboard's leds for lock states (scroll, numeric, caps).
 *
 * at least one keyboard (from Qtronics) also sets its numeric-lock
 * behaviour to match the led state, though it has no numeric keypad,
 * and some BIOSes bring the system up with numeric-lock set and no
 * setting to change that.  this combination steals the keys for these
 * characters and makes it impossible to generate them: uiolkjm&*().
 * thus we'd like to be able to force the numeric-lock led (and behaviour) off.
 */
static void
setleds(Kbscan *kbscan)
{
	int leds;

	if(nokbd || kbscan != &kbscans[Int])
		return;
	leds = 0;
	if(kbscan->num)
		leds |= 1<<1;
	if(0 && kbscan->caps)		/* we don't implement caps lock */
		leds |= 1<<2;
	ilock(&i8042lock);
	outready();
	outb(Data, 0xed);		/* `reset keyboard lock states' */
	outready();
	outb(Data, leds);
	outready();
	iunlock(&i8042lock);
}

/*
 * Scan code processing
 */
void
kbdputsc(int c, int external)
{
	int i, keyup;
	Kbscan *kbscan;

	if(external)
		kbscan = &kbscans[Ext];
	else
		kbscan = &kbscans[Int];

	if(kdebug)
		print("sc %x ms %d\n", c, mouseshifted);
	/*
	 *  e0's is the first of a 2 character sequence, e1 the first
	 *  of a 3 character sequence (on the safari)
	 */
	if(c == 0xe0){
		kbscan->esc1 = 1;
		return;
	} else if(c == 0xe1){
		kbscan->esc2 = 2;
		return;
	}

	keyup = c & 0x80;
	c &= 0x7f;
	if(c > sizeof kbtab){
		c |= keyup;
		if(c != 0xFF)	/* these come fairly often: CAPSLOCK U Y */
			print("unknown key %ux\n", c);
		return;
	}

	if(kbscan->esc1){
		c = kbtabesc1[c];
		kbscan->esc1 = 0;
	} else if(kbscan->esc2){
		kbscan->esc2--;
		return;
	} else if(kbscan->shift)
		c = kbtabshift[c];
	else if(kbscan->altgr)
		c = kbtabaltgr[c];
	else if(kbscan->ctl)
		c = kbtabctrl[c];
	else
		c = kbtab[c];

	if(kbscan->caps && c<='z' && c>='a')
		c += 'A' - 'a';

	/*
	 *  keyup only important for shifts
	 */
	if(keyup){
		switch(c){
		case Latin:
			kbscan->alt = 0;
			break;
		case Shift:
			kbscan->shift = 0;
			mouseshifted = 0;
			if(kdebug)
				print("shiftclr\n");
			break;
		case Ctrl:
			kbscan->ctl = 0;
			break;
		case Altgr:
			kbscan->altgr = 0;
			break;
		case Kmouse|1:
		case Kmouse|2:
		case Kmouse|3:
		case Kmouse|4:
		case Kmouse|5:
			kbscan->buttons &= ~(1<<(c-Kmouse-1));
			if(kbdmouse)
				kbdmouse(kbscan->buttons);
			break;
		}
		return;
	}

	/*
	 *  normal character
	 */
	if(!(c & (Spec|KF))){
		if(kbscan->ctl)
			if(kbscan->alt && c == Del)
				exit(0);
		if(!kbscan->collecting){
			kbdputc(kbdq, c);
			return;
		}
		kbscan->kc[kbscan->nk++] = c;
		c = latin1(kbscan->kc, kbscan->nk);
		if(c < -1)	/* need more keystrokes */
			return;
		if(c != -1)	/* valid sequence */
			kbdputc(kbdq, c);
		else	/* dump characters */
			for(i=0; i<kbscan->nk; i++)
				kbdputc(kbdq, kbscan->kc[i]);
		kbscan->nk = 0;
		kbscan->collecting = 0;
		return;
	} else {
		switch(c){
		case Caps:
			kbscan->caps ^= 1;
			return;
		case Num:
			kbscan->num ^= 1;
			if(!external)
				setleds(kbscan);
			return;
		case Shift:
			kbscan->shift = 1;
			if(kdebug)
				print("shift\n");
			mouseshifted = 1;
			return;
		case Latin:
			kbscan->alt = 1;
			/*
			 * VMware and Qemu use Ctl-Alt as the key combination
			 * to make the VM give up keyboard and mouse focus.
			 * This has the unfortunate side effect that when you
			 * come back into focus, Plan 9 thinks you want to type
			 * a compose sequence (you just typed alt). 
			 *
			 * As a clumsy hack around this, we look for ctl-alt
			 * and don't treat it as the start of a compose sequence.
			 */
			if(!kbscan->ctl){
				kbscan->collecting = 1;
				kbscan->nk = 0;
			}
			return;
		case Ctrl:
			kbscan->ctl = 1;
			return;
		case Altgr:
			kbscan->altgr = 1;
			return;
		case Kmouse|1:
		case Kmouse|2:
		case Kmouse|3:
		case Kmouse|4:
		case Kmouse|5:
			kbscan->buttons |= 1<<(c-Kmouse-1);
			if(kbdmouse)
				kbdmouse(kbscan->buttons);
			return;
		case KF|11:
			print("kbd debug on, F12 turns it off\n");
			kdebug = 1;
			break;
		case KF|12:
			kdebug = 0;
			break;
		}
	}
	kbdputc(kbdq, c);
}

/*
 *  keyboard interrupt
 */
static void
i8042intr(Ureg*, void*)
{
	int s, c;

	/*
	 *  get status
	 */
	ilock(&i8042lock);
	s = inb(Status);
	if(!(s&Inready)){
		iunlock(&i8042lock);
		return;
	}

	/*
	 *  get the character
	 */
	c = inb(Data);
	iunlock(&i8042lock);

	/*
	 *  if it's the aux port...
	 */
	if(s & Minready){
		if(auxputc != nil)
			auxputc(c, kbscans[Int].shift);
		return;
	}

	kbdputsc(c, Int);
}

void
i8042auxenable(void (*putc)(int, int))
{
	char *err = "i8042: aux init failed\n";

	/* enable kbd/aux xfers and interrupts */
	ccc &= ~Cauxdis;
	ccc |= Cauxint;

	ilock(&i8042lock);
	if(outready() < 0)
		print(err);
	outb(Cmd, 0x60);			/* write control register */
	if(outready() < 0)
		print(err);
	outb(Data, ccc);
	if(outready() < 0)
		print(err);
	outb(Cmd, 0xA8);			/* auxiliary device enable */
	if(outready() < 0){
		iunlock(&i8042lock);
		return;
	}
	auxputc = putc;
	intrenable(IrqAUX, i8042intr, 0, BUSUNKNOWN, "kbdaux");
	iunlock(&i8042lock);
}

static char *initfailed = "i8042: kbdinit failed\n";

static int
outbyte(int port, int c)
{
	outb(port, c);
	if(outready() < 0) {
		print(initfailed);
		return -1;
	}
	return 0;
}

void
kbdinit(void)
{
	int c, try;

	/* wait for a quiescent controller */
	try = 1000;
	while(try-- > 0 && (c = inb(Status)) & (Outbusy | Inready)) {
		if(c & Inready)
			inb(Data);
		delay(1);
	}
	if (try <= 0) {
		print(initfailed);
		return;
	}

	/* get current controller command byte */
	outb(Cmd, 0x20);
	if(inready() < 0){
		print("i8042: kbdinit can't read ccc\n");
		ccc = 0;
	} else
		ccc = inb(Data);

	/* enable kbd xfers and interrupts */
	ccc &= ~Ckbddis;
	ccc |= Csf | Ckbdint | Cscs1;
	if(outready() < 0) {
		print(initfailed);
		return;
	}

	nokbd = 0;

	/* disable mouse */
	if (outbyte(Cmd, 0x60) < 0 || outbyte(Data, ccc) < 0)
		print("i8042: kbdinit mouse disable failed\n");
}

void
kbdenable(void)
{
	kbdq = qopen(4*1024, 0, 0, 0);
	if(kbdq == nil)
		panic("kbdinit");
	qnoblock(kbdq, 1);

	ioalloc(Data, 1, 0, "kbd");
	ioalloc(Cmd, 1, 0, "kbd");

	intrenable(IrqKBD, i8042intr, 0, BUSUNKNOWN, "kbd");

	kbscans[Int].num = 0;
	setleds(&kbscans[Int]);
}

void
kbdputmap(ushort m, ushort scanc, Rune r)
{
	if(scanc >= Nscan)
		error(Ebadarg);
	switch(m) {
	default:
		error(Ebadarg);
	case 0:
		kbtab[scanc] = r;
		break;
	case 1:
		kbtabshift[scanc] = r;
		break;
	case 2:
		kbtabesc1[scanc] = r;
		break;
	case 3:
		kbtabaltgr[scanc] = r;
		break;
	case 4:	
		kbtabctrl[scanc] = r;
		break;
	}
}

int
kbdgetmap(uint offset, int *t, int *sc, Rune *r)
{
	if ((int)offset < 0)
		error(Ebadarg);
	*t = offset/Nscan;
	*sc = offset%Nscan;
	switch(*t) {
	default:
		return 0;
	case 0:
		*r = kbtab[*sc];
		return 1;
	case 1:
		*r = kbtabshift[*sc];
		return 1;
	case 2:
		*r = kbtabesc1[*sc];
		return 1;
	case 3:
		*r = kbtabaltgr[*sc];
		return 1;
	case 4:
		*r = kbtabctrl[*sc];
		return 1;
	}
}
