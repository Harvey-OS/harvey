/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"../port/error.h"

#include	"io.h"

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
};

/*
 * The codes at 0x79 and 0x81 are produed by the PFU Happy Hacking keyboard.
 * A 'standard' keyboard doesn't produce anything above 0x58.
 */
Rune kbtab[Nscan] =
{
[0x00] =	No,	0x1b,	'1',	'2',	'3',	'4',	'5',	'6',
[0x08] =	'7',	'8',	'9',	'0',	'-',	'=',	'\b',	'\t',
[0x10] =	'q',	'w',	'e',	'r',	't',	'y',	'u',	'i',
[0x18] =	'o',	'p',	'[',	']',	'\n',	Ctrl,	'a',	's',
[0x20] =	'd',	'f',	'g',	'h',	'j',	'k',	'l',	';',
[0x28] =	'\'',	'`',	Shift,	'\\',	'z',	'x',	'c',	'v',
[0x30] =	'b',	'n',	'm',	',',	'.',	'/',	Shift,	'*',
[0x38] =	Latin,	' ',	Ctrl,	KF|1,	KF|2,	KF|3,	KF|4,	KF|5,
[0x40] =	KF|6,	KF|7,	KF|8,	KF|9,	KF|10,	Num,	Scroll,	'7',
[0x48] =	'8',	'9',	'-',	'4',	'5',	'6',	'+',	'1',
[0x50] =	'2',	'3',	'0',	'.',	No,	No,	No,	KF|11,
[0x58] =	KF|12,	No,	No,	No,	No,	No,	No,	No,
[0x60] =	No,	No,	No,	No,	No,	No,	No,	No,
[0x68] =	No,	No,	No,	No,	No,	No,	No,	No,
[0x70] =	No,	No,	No,	No,	No,	No,	No,	No,
[0x78] =	No,	View,	No,	Up,	No,	No,	No,	No,
};

Rune kbtabshift[Nscan] =
{
[0x00] =	No,	0x1b,	'!',	'@',	'#',	'$',	'%',	'^',
[0x08] =	'&',	'*',	'(',	')',	'_',	'+',	'\b',	'\t',
[0x10] =	'Q',	'W',	'E',	'R',	'T',	'Y',	'U',	'I',
[0x18] =	'O',	'P',	'{',	'}',	'\n',	Ctrl,	'A',	'S',
[0x20] =	'D',	'F',	'G',	'H',	'J',	'K',	'L',	':',
[0x28] =	'"',	'~',	Shift,	'|',	'Z',	'X',	'C',	'V',
[0x30] =	'B',	'N',	'M',	'<',	'>',	'?',	Shift,	'*',
[0x38] =	Latin,	' ',	Ctrl,	KF|1,	KF|2,	KF|3,	KF|4,	KF|5,
[0x40] =	KF|6,	KF|7,	KF|8,	KF|9,	KF|10,	Num,	Scroll,	'7',
[0x48] =	'8',	'9',	'-',	'4',	'5',	'6',	'+',	'1',
[0x50] =	'2',	'3',	'0',	'.',	No,	No,	No,	KF|11,
[0x58] =	KF|12,	No,	No,	No,	No,	No,	No,	No,
[0x60] =	No,	No,	No,	No,	No,	No,	No,	No,
[0x68] =	No,	No,	No,	No,	No,	No,	No,	No,
[0x70] =	No,	No,	No,	No,	No,	No,	No,	No,
[0x78] =	No,	Up,	No,	Up,	No,	No,	No,	No,
};

Rune kbtabesc1[Nscan] =
{
[0x00] =	No,	No,	No,	No,	No,	No,	No,	No,
[0x08] =	No,	No,	No,	No,	No,	No,	No,	No,
[0x10] =	No,	No,	No,	No,	No,	No,	No,	No,
[0x18] =	No,	No,	No,	No,	'\n',	Ctrl,	No,	No,
[0x20] =	No,	No,	No,	No,	No,	No,	No,	No,
[0x28] =	No,	No,	Shift,	No,	No,	No,	No,	No,
[0x30] =	No,	No,	No,	No,	No,	'/',	No,	Print,
[0x38] =	Altgr,	No,	No,	No,	No,	No,	No,	No,
[0x40] =	No,	No,	No,	No,	No,	No,	Break,	Home,
[0x48] =	Up,	Pgup,	No,	Left,	No,	Right,	No,	End,
[0x50] =	Down,	Pgdown,	Ins,	Del,	No,	No,	No,	No,
[0x58] =	No,	No,	No,	No,	No,	No,	No,	No,
[0x60] =	No,	No,	No,	No,	No,	No,	No,	No,
[0x68] =	No,	No,	No,	No,	No,	No,	No,	No,
[0x70] =	No,	No,	No,	No,	No,	No,	No,	No,
[0x78] =	No,	Up,	No,	No,	No,	No,	No,	No,
};

Rune kbtabaltgr[Nscan] =
{
[0x00] =	No,	No,	No,	No,	No,	No,	No,	No,
[0x08] =	No,	No,	No,	No,	No,	No,	No,	No,
[0x10] =	No,	No,	No,	No,	No,	No,	No,	No,
[0x18] =	No,	No,	No,	No,	'\n',	Ctrl,	No,	No,
[0x20] =	No,	No,	No,	No,	No,	No,	No,	No,
[0x28] =	No,	No,	Shift,	No,	No,	No,	No,	No,
[0x30] =	No,	No,	No,	No,	No,	'/',	No,	Print,
[0x38] =	Altgr,	No,	No,	No,	No,	No,	No,	No,
[0x40] =	No,	No,	No,	No,	No,	No,	Break,	Home,
[0x48] =	Up,	Pgup,	No,	Left,	No,	Right,	No,	End,
[0x50] =	Down,	Pgdown,	Ins,	Del,	No,	No,	No,	No,
[0x58] =	No,	No,	No,	No,	No,	No,	No,	No,
[0x60] =	No,	No,	No,	No,	No,	No,	No,	No,
[0x68] =	No,	No,	No,	No,	No,	No,	No,	No,
[0x70] =	No,	No,	No,	No,	No,	No,	No,	No,
[0x78] =	No,	Up,	No,	No,	No,	No,	No,	No,
};

/*
Rune kbtabctrl[] =
{
[0x00]	No,	'', 	'', 	'', 	'', 	'', 	'', 	'',
[0x08]	'', 	'', 	'', 	'', 	'
', 	'', 	'\b',	'\t',
[0x10]	'', 	'', 	'', 	'', 	'', 	'', 	'', 	'\t',
[0x18]	'', 	'', 	'', 	'', 	'\n',	Ctrl,	'', 	'',
[0x20]	'', 	'', 	'', 	'\b',	'\n',	'', 	'', 	'',
[0x28]	'', 	No, 	Shift,	'', 	'', 	'', 	'', 	'',
[0x30]	'', 	'', 	'
', 	'', 	'', 	'', 	Shift,	'\n',
[0x38]	Latin,	No, 	Ctrl,	'', 	'', 	'', 	'', 	'',
[0x40]	'', 	'', 	'', 	'
', 	'', 	'', 	'', 	'',
[0x48]	'', 	'', 	'
', 	'', 	'', 	'', 	'', 	'',
[0x50]	'', 	'', 	'', 	'', 	No,	No,	No,	'',
[0x58]	'', 	No,	No,	No,	No,	No,	No,	No,
[0x60]	No,	No,	No,	No,	No,	No,	No,	No,
[0x68]	No,	No,	No,	No,	No,	No,	No,	No,
[0x70]	No,	No,	No,	No,	No,	No,	No,	No,
[0x78]	No,	'', 	No,	'\b',	No,	No,	No,	No,
};
*/

static Rune kbtabctrl[256] =
{
[0x00] =	No,	'\x1b',	'\x11',	'\x12',	'\x13',	'\x14',	'\x15',	'\x16',
[0x08] =	'\x17',	'\x18',	'\x19',	'\x10',	'\n',	'\x1d',	'\b',	'\t',
[0x10] =	'\x11',	'\x17',	'\x05',	'\x12',	'\x14',	'\x19',	'\x15',	'\t',
[0x18] =	'\x0f',	'\x10',	'\x1b',	'\x1d',	'\n',	Ctrl,	'\x01',	'\x13',
[0x20] =	'\x04',	'\x06',	'\x07',	'\b',	'\n',	'\x0b',	'\x0c',	'\x1b',
[0x28] =	'\x07',	No,	Shift,	'\x1c',	'\x1a',	'\x18',	'\x03',	'\x16',
[0x30] =	'\x02',	'\x0e',	'\n',	'\x0c',	'\x0e',	'\x0f',	Shift,	'\n',
[0x38] =	Latin,	No,	Ctrl,	'\x05',	'\x06',	'\x07',	'\x04',	'\x05',
[0x40] =	'\x06',	'\x07',	'\x0c',	'\n',	'\x0e',	'\x05',	'\x06',	'\x17',
[0x48] =	'\x18',	'\x19',	'\n',	'\x14',	'\x15',	'\x16',	'\x0b',	'\x11',
[0x50] =	'\x12',	'\x13',	'\x10',	'\x0e',	No,	No,	No,	'\x0f',
[0x58] =	'\x0c',	No,	No,	No,	No,	No,	No,	No,
[0x60] =	No,	No,	No,	No,	No,	No,	No,	No,
[0x68] =	No,	No,	No,	No,	No,	No,	No,	No,
[0x70] =	No,	No,	No,	No,	No,	No,	No,	No,
[0x78] =	No,	'\x07',	No,	'\b',	No,	No,	No,	No,
};

enum
{
	/* controller command byte */
	Cscs1=		(1<<6),		/* scan code set 1 */
	Cauxdis=	(1<<5),		/* mouse disable */
	Ckeybdis=	(1<<4),		/* kbd disable */
	Csf=		(1<<2),		/* system flag */
	Cauxint=	(1<<1),		/* mouse interrupt enable */
	Ckeybint=	(1<<0),		/* kbd interrupt enable */
};

static Queue *keybq;
static Queue *mouseq;

int mouseshifted;
void (*kbdmouse)(int);
static int nokeyb = 1;

static Lock i8042lock;
static uint8_t ccc;
static void (*auxputc)(int, int);

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
i8042systemreset(void)
{
	uint16_t *s = KADDR(0x472);
	int i, x;

	if(nokeyb)
		return;

	*s = 0x1234;		/* BIOS warm-boot flag */

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
		return -1;
	}
	return 0;
}

int
i8042auxcmds(uint8_t *cmd, int ncmd)
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

struct {
	int esc1;
	int esc2;
	int alt;
	int altgr;
	int caps;
	int ctl;
	int num;
	int shift;
	int collecting;
	int nk;
	Rune kc[5];
	int buttons;
} kbscan;

/*
 *  keyboard interrupt
 */
static void
i8042intr(Ureg* u, void* v)
{
	int s, c, i;
	int keyup;

	/*
	 *  get status
	 */
	ilock(&i8042lock);
	s = inb(Status);
	if((s&Inready) == 0){
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
		if(mouseq != nil)
			qiwrite(mouseq, &c, 1);
	}

	/*
	 *  e0's is the first of a 2 character sequence, e1 the first
	 *  of a 3 character sequence (on the safari)
	 */
	if(c == 0xe0){
		kbscan.esc1 = 1;
		return;
	} else if(c == 0xe1){
		kbscan.esc2 = 2;
		return;
	}

	keyup = c&0x80;
	c &= 0x7f;
	if(c > sizeof kbtab){
		c |= keyup;
		if(c != 0xFF)	/* these come fairly often: CAPSLOCK U Y */
			print("unknown key %ux\n", c);
		return;
	}

	if(kbscan.esc1){
		c = kbtabesc1[c];
		kbscan.esc1 = 0;
	} else if(kbscan.esc2){
		kbscan.esc2--;
		return;
	} else if(kbscan.shift)
		c = kbtabshift[c];
	else if(kbscan.altgr)
		c = kbtabaltgr[c];
	else if(kbscan.ctl)
		c = kbtabctrl[c];
	else
		c = kbtab[c];

	if(kbscan.caps && c<='z' && c>='a')
		c += 'A' - 'a';

	/*
	 *  keyup only important for shifts
	 */
	if(keyup){
		switch(c){
		case Latin:
			kbscan.alt = 0;
			break;
		case Shift:
			kbscan.shift = 0;
			mouseshifted = 0;
			break;
		case Ctrl:
			kbscan.ctl = 0;
			break;
		case Altgr:
			kbscan.altgr = 0;
			break;
		case Kmouse|1:
		case Kmouse|2:
		case Kmouse|3:
		case Kmouse|4:
		case Kmouse|5:
			kbscan.buttons &= ~(1<<(c-Kmouse-1));
			if(kbdmouse)
				kbdmouse(kbscan.buttons);
			break;
		}
		return;
	}

	/*
 	 *  normal character
	 */
	if(!(c & (Spec|KF))){
		if(kbscan.ctl)
			if(kbscan.alt && c == Del)
				exit(0);
		if(!kbscan.collecting){
			kbdputc(keybq, c);
			return;
		}
		kbscan.kc[kbscan.nk++] = c;
		c = latin1(kbscan.kc, kbscan.nk);
		if(c < -1)	/* need more keystrokes */
			return;
		if(c != -1)	/* valid sequence */
			kbdputc(keybq, c);
		else	/* dump characters */
			for(i=0; i<kbscan.nk; i++)
				kbdputc(keybq, kbscan.kc[i]);
		kbscan.nk = 0;
		kbscan.collecting = 0;
		return;
	} else {
		switch(c){
		case Caps:
			kbscan.caps ^= 1;
			return;
		case Num:
			kbscan.num ^= 1;
			return;
		case Shift:
			kbscan.shift = 1;
			mouseshifted = 1;
			return;
		case Latin:
			kbscan.alt = 1;
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
			if(!kbscan.ctl){
				kbscan.collecting = 1;
				kbscan.nk = 0;
			}
			return;
		case Ctrl:
			kbscan.ctl = 1;
			return;
		case Altgr:
			kbscan.altgr = 1;
			return;
		case Kmouse|1:
		case Kmouse|2:
		case Kmouse|3:
		case Kmouse|4:
		case Kmouse|5:
			kbscan.buttons |= 1<<(c-Kmouse-1);
			if(kbdmouse)
				kbdmouse(kbscan.buttons);
			return;
		}
	}
	kbdputc(keybq, c);
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
	outb(Cmd, 0xA8);			/* auxilliary device enable */
	if(outready() < 0){
		iunlock(&i8042lock);
		return;
	}
	auxputc = putc;
	intrenable(IrqAUX, i8042intr, 0, BUSUNKNOWN, "kbdaux");
	iunlock(&i8042lock);
}

int
mousecmd(int cmd)
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
		print("mousecmd: %2.2x returned to the %2.2x command\n", c, cmd);
		badkbd = 1;	/* don't keep trying; there might not be one */
		return -1;
	}
	return 0;
}

static int
mousecmds(uint8_t *cmd, int ncmd)
{
	int i;

	ilock(&i8042lock);
	for(i=0; i<ncmd; i++){
		if(outready() == -1)
			break;
		outb(Cmd, 0xD4);
		if(outready() == -1)
			break;
		outb(Data, cmd[i]);
	}
	iunlock(&i8042lock);
	return i;
}

void
kbdputsc(int data, int _)
{
	qiwrite(keybq, &data, 1);
}

static char *initfailed = "i8042: keybinit failed\n";

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

static int32_t
mouserwrite(Chan* c, void *vbuf, int32_t len, int64_t off64)
{
	return mousecmds(vbuf, len);
}

static int32_t
mouseread(Chan* c, void *vbuf, int32_t len, int64_t off64)
{
	return qread(mouseq, vbuf, len);
}

void
mouseenable(void)
{
	mouseq = qopen(32, 0, 0, 0);
	if(mouseq == nil)
		panic("mouseenable");
	qnoblock(mouseq, 1);

	ccc &= ~Cauxdis;
	ccc |= Cauxint;

	ilock(&i8042lock);
	if(outready() == -1)
		iprint("mouseenable: failed 0\n");
	outb(Cmd, 0x60);	/* write control register */
	if(outready() == -1)
		iprint("mouseenable: failed 1\n");
	outb(Data, ccc);
	if(outready() == -1)
		iprint("mouseenable: failed 2\n");
	outb(Cmd, 0xA8);	/* auxilliary device enable */
	if(outready() == -1){
		iprint("mouseenable: failed 3\n");
		iunlock(&i8042lock);
		return;
	}
	iunlock(&i8042lock);

	intrenable(IrqAUX, i8042intr, 0, BUSUNKNOWN, "mouse");
	addarchfile("ps2mouse", 0666, mouseread, mouserwrite);
}

void
keybinit(void)
{
	int c, try;

	/* wait for a quiescent controller */
	ilock(&i8042lock);

	try = 1000;
	while(try-- > 0 && (c = inb(Status)) & (Outbusy | Inready)) {
		if(c & Inready)
			inb(Data);
		delay(1);
	}
	if (try <= 0) {
		iunlock(&i8042lock);
		print("keybinit failed 0\n");
		return;
	}

	/* get current controller command byte */
	outb(Cmd, 0x20);
	if(inready() == -1){
		iunlock(&i8042lock);
		print("keybinit failed 1\n");
		ccc = 0;
	} else
		ccc = inb(Data);

	/* enable keyb xfers and interrupts */
	ccc &= ~(Ckeybdis);
	ccc |= Csf | Ckeybint | Cscs1;
	if(outready() == -1) {
		iunlock(&i8042lock);
		print("keybinit failed 2\n");
		return;
	}

	if (outbyte(Cmd, 0x60) == -1){
		iunlock(&i8042lock);
		print("keybinit failed 3\n");
		return;
	}
	if (outbyte(Data, ccc) == -1){
		iunlock(&i8042lock);
		print("keybinit failed 4\n");
		return;
	}

	nokeyb = 0;

	iunlock(&i8042lock);
}

static int32_t
keybread(Chan* c, void *vbuf, int32_t len, int64_t off64)
{
	return qread(keybq, vbuf, len);
}

void
keybenable(void)
{
	keybq = qopen(32, 0, 0, 0);
	if(keybq == nil)
		panic("keybinit");
	qnoblock(keybq, 1);
	addkbdq(keybq, -1);

	ioalloc(Data, 1, 0, "keyb");
	ioalloc(Cmd, 1, 0, "keyb");

	intrenable(IrqKBD, i8042intr, 0, MKBUS(BusISA, 0xff, 0xff, 0xff), "keyb");

	addarchfile("ps2keyb", 0666, keybread, nil);
}

void
kbdputmap(uint16_t m, uint16_t scanc, Rune r)
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
kbdgetmap(int offset, int *t, int *sc, Rune *r)
{
	*t = offset/Nscan;
	*sc = offset%Nscan;
	if(*t < 0 || *sc < 0)
		error(Ebadarg);
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
