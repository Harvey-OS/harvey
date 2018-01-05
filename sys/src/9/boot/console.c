#include <u.h>
#include <libc.h>

enum {
	Spec =	0xF800,		/* Unicode private space */
		PF =	Spec|0x20,	/* num pad function key */
		View =	Spec|0x00,	/* view (shift window up) */
		Shift =	Spec|0x60,
		Break =	Spec|0x61,
		Ctrl =	Spec|0x62,
		Alt =	Spec|0x63,
		Caps =	Spec|0x64,
		Num =	Spec|0x65,
		Middle =	Spec|0x66,
		Altgr =	Spec|0x67,
		Kmouse =	Spec|0x100,
	No =	0x00,		/* peter */

	KF =	0xF000,		/* function key (begin Unicode private space) */
		Home =	KF|13,
		Up =		KF|14,
		Pgup =		KF|15,
		Print =	KF|16,
		Left =	KF|17,
		Right =	KF|18,
		End =	KF|24,
		Down =	View,
		Pgdown =	KF|19,
		Ins =	KF|20,
		Del =	0x7F,
		Scroll =	KF|21,
};

typedef struct Keybscan Keybscan;
struct Keybscan {
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
};

static Keybscan kbscan;

static Rune kbtab[256] =
{
[0x00]	No,	'\x1b',	'1',	'2',	'3',	'4',	'5',	'6',
[0x08]	'7',	'8',	'9',	'0',	'-',	'=',	'\b',	'\t',
[0x10]	'q',	'w',	'e',	'r',	't',	'y',	'u',	'i',
[0x18]	'o',	'p',	'[',	']',	'\n',	Ctrl,	'a',	's',
[0x20]	'd',	'f',	'g',	'h',	'j',	'k',	'l',	';',
[0x28]	'\'',	'`',	Shift,	'\\',	'z',	'x',	'c',	'v',
[0x30]	'b',	'n',	'm',	',',	'.',	'/',	Shift,	'*',
[0x38]	Alt,	' ',	Ctrl,	KF|1,	KF|2,	KF|3,	KF|4,	KF|5,
[0x40]	KF|6,	KF|7,	KF|8,	KF|9,	KF|10,	Num,	Scroll,	'7',
[0x48]	'8',	'9',	'-',	'4',	'5',	'6',	'+',	'1',
[0x50]	'2',	'3',	'0',	'.',	No,	No,	No,	KF|11,
[0x58]	KF|12,	No,	No,	No,	No,	No,	No,	No,
[0x60]	No,	No,	No,	No,	No,	No,	No,	No,
[0x68]	No,	No,	No,	No,	No,	No,	No,	No,
[0x70]	No,	No,	No,	No,	No,	No,	No,	No,
[0x78]	No,	View,	No,	Up,	No,	No,	No,	No,
};

static Rune kbtabshift[256] =
{
[0x00]	No,	'\x1b',	'!',	'@',	'#',	'$',	'%',	'^',
[0x08]	'&',	'*',	'(',	')',	'_',	'+',	'\b',	'\t',
[0x10]	'Q',	'W',	'E',	'R',	'T',	'Y',	'U',	'I',
[0x18]	'O',	'P',	'{',	'}',	'\n',	Ctrl,	'A',	'S',
[0x20]	'D',	'F',	'G',	'H',	'J',	'K',	'L',	':',
[0x28]	'"',	'~',	Shift,	'|',	'Z',	'X',	'C',	'V',
[0x30]	'B',	'N',	'M',	'<',	'>',	'?',	Shift,	'*',
[0x38]	Alt,	' ',	Ctrl,	KF|1,	KF|2,	KF|3,	KF|4,	KF|5,
[0x40]	KF|6,	KF|7,	KF|8,	KF|9,	KF|10,	Num,	Scroll,	'7',
[0x48]	'8',	'9',	'-',	'4',	'5',	'6',	'+',	'1',
[0x50]	'2',	'3',	'0',	'.',	No,	No,	No,	KF|11,
[0x58]	KF|12,	No,	No,	No,	No,	No,	No,	No,
[0x60]	No,	No,	No,	No,	No,	No,	No,	No,
[0x68]	No,	No,	No,	No,	No,	No,	No,	No,
[0x70]	No,	No,	No,	No,	No,	No,	No,	No,
[0x78]	No,	Up,	No,	Up,	No,	No,	No,	No,
};

static Rune kbtabesc1[256] =
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

static Rune kbtabaltgr[256] =
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

static Rune kbtabctrl[256] =
{
[0x00]	No,	'\x1b',	'\x11',	'\x12',	'\x13',	'\x14',	'\x15',	'\x16',
[0x08]	'\x17',	'\x18',	'\x19',	'\x10',	'\n',	'\x1d',	'\b',	'\t',
[0x10]	'\x11',	'\x17',	'\x05',	'\x12',	'\x14',	'\x19',	'\x15',	'\t',
[0x18]	'\x0f',	'\x10',	'\x1b',	'\x1d',	'\n',	Ctrl,	'\x01',	'\x13',
[0x20]	'\x04',	'\x06',	'\x07',	'\b',	'\n',	'\x0b',	'\x0c',	'\x1b',
[0x28]	'\x07',	No,	Shift,	'\x1c',	'\x1a',	'\x18',	'\x03',	'\x16',
[0x30]	'\x02',	'\x0e',	'\n',	'\x0c',	'\x0e',	'\x0f',	Shift,	'\n',
[0x38]	Alt,	No,	Ctrl,	'\x05',	'\x06',	'\x07',	'\x04',	'\x05',
[0x40]	'\x06',	'\x07',	'\x0c',	'\n',	'\x0e',	'\x05',	'\x06',	'\x17',
[0x48]	'\x18',	'\x19',	'\n',	'\x14',	'\x15',	'\x16',	'\x0b',	'\x11',
[0x50]	'\x12',	'\x13',	'\x10',	'\x0e',	No,	No,	No,	'\x0f',
[0x58]	'\x0c',	No,	No,	No,	No,	No,	No,	No,
[0x60]	No,	No,	No,	No,	No,	No,	No,	No,
[0x68]	No,	No,	No,	No,	No,	No,	No,	No,
[0x70]	No,	No,	No,	No,	No,	No,	No,	No,
[0x78]	No,	'\x07',	No,	'\b',	No,	No,	No,	No,
};

int
keybscan(uint8_t code, char *out, int len)
{
	Rune c;
	int keyup;
	int off;

	c = code;
	off = 0;

	if(len < 16){
		fprint(2, "keybscan: input buffer too short to be safe\n");
		return -1;
	}
	if(c == 0xe0){
		kbscan.esc1 = 1;
		return off;
	} else if(c == 0xe1){
		kbscan.esc2 = 2;
		return off;
	}

	keyup = c&0x80;
	c &= 0x7f;
	if(c > sizeof kbtab){
		c |= keyup;
		if(c != 0xFF)	/* these come fairly often: CAPSLOCK U Y */
			fprint(2, "unknown key %ux\n", c);
		return off;
	}

	if(kbscan.esc1){
		c = kbtabesc1[c];
		kbscan.esc1 = 0;
	} else if(kbscan.esc2){
		kbscan.esc2--;
		return off;
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
		case Alt:
			kbscan.alt = 0;
			break;
		case Shift:
			kbscan.shift = 0;
			/*mouseshifted = 0;*/
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
			/*if(kbdmouse)kbdmouse(kbscan.buttons);*/
			break;
		}
		return off;
	}

	/*
	 *  normal character
	 */
	if(!(c & (Spec|KF))){
		off += runetochar(out+off, &c);
		return off;
	} else {
		switch(c){
		case Caps:
			kbscan.caps ^= 1;
			return off;
		case Num:
			kbscan.num ^= 1;
			return off;
		case Shift:
			kbscan.shift = 1;
			/*mouseshifted = 1;*/
			return off;
		case Alt:
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
			return off;
		case Ctrl:
			kbscan.ctl = 1;
			return off;
		case Altgr:
			kbscan.altgr = 1;
			return off;
		case Kmouse|1:
		case Kmouse|2:
		case Kmouse|3:
		case Kmouse|4:
		case Kmouse|5:
			kbscan.buttons |= 1<<(c-Kmouse-1);
			/*if(kbdmouse)kbdmouse(kbscan.buttons);*/
			return off;
		}
	}

	off += runetochar(out+off, &c);
	return off;

}

enum {
	Black		= 0x00,
	Blue		= 0x01,
	Green		= 0x02,
	Cyan		= 0x03,
	Red		= 0x04,
	Magenta		= 0x05,
	Brown		= 0x06,
	Grey		= 0x07,

	Bright 		= 0x08,
	Blinking	= 0x80,

	Attr		= (Black<<4)|Grey,	/* (background<<4)|foreground */
	Cursor = (Grey<<4)|Black,
};

enum {
	Width		= 80*2,
	Height		= 25,
};

int
readline(char* buf, int buflen)
{
	int ps2fd;
	int i, n, off, lastoff, len;
	char* scan;
	static uint8_t ibuf[32];

	if((ps2fd = open("#P/ps2keyb", OREAD)) == -1){
		errstr(buf, sizeof buf);
		fprint(2, "open #P/ps2keyb: %s\n", buf);
		return -1;
	}

	off = 0;
	for (;;){
		while((len = read(ps2fd, ibuf, sizeof ibuf)) > 0){
			lastoff = off;
			for(i = 0; i < len; i++){
				if((n = keybscan(ibuf[i], buf+off, buflen - off)) == -1){
					fprint(2, "keybscan -1\n");
					return -1;
				}
				off += n;
			}
			if(buf[off-1] == '\n')
				goto GotLine;
			if(off > 0){
				for(scan = buf + lastoff; scan < buf + off; scan++){
					write(1, scan, 1);
					if(*scan == 0x8)
						off -= 2;
				}
			}
		}
	}
GotLine:
	close(ps2fd);
	write(1, "\n", 1);
	buf[off] = '\0';
	return off - 1;
}
