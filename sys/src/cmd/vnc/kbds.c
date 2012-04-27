#include	<u.h>
#include	<libc.h>
#include	"compat.h"
#include	"kbd.h"
#include   "ksym2utf.h"

enum
{
	VKSpecial = 0xff00,

	/*
	 * plan 9 key mappings
	 */
	Spec=		0xF800,

	PF=		Spec|0x20,	/* num pad function key */
	View=		Spec|0x00,	/* view (shift window up) */
	KF=		0xF000,	/* function key (begin Unicode private space) */
	Shift=		Spec|0x60,
	Break=		Spec|0x61,
	Ctrl=		Spec|0x62,
	Latin=		Spec|0x63,
	Caps=		Spec|0x64,
	Num=		Spec|0x65,
	Middle=		Spec|0x66,
	No=		0x00,		/* peter */

	Home=		KF|13,
	Up=		KF|14,
	Pgup=		KF|15,
	Print=		KF|16,
	Left=		KF|17,
	Right=		KF|18,
	End=		'\r',
	Down=		View,
	Pgdown=		KF|19,
	Ins=		KF|20,
	Del=		0x7F,
	Scroll=		KF|21,

	Esc = 0x1b,
	Delete = 0x7f,
};

static Rune vnckeys[] =
{
[0x00]	No,	No,	No,	No,	No,	No,	No,	No,
[0x08]	'\b',	'\t',	'\r',	No,	No,	'\n',	No,	No,
[0x10]	No,	No,	No,	No,	Scroll,	No,	No,	No,
[0x18]	No,	No,	No,	Esc,	No,	No,	No,	No,
[0x20]	No,	No,	No,	No,	No,	No,	No,	No,
[0x28]	No,	No,	No,	No,	No,	No,	No,	No,
[0x30]	No,	No,	No,	No,	No,	No,	No,	No,
[0x38]	No,	No,	No,	No,	No,	No,	No,	No,
[0x40]	No,	No,	No,	No,	No,	No,	No,	No,
[0x48]	No,	No,	No,	No,	No,	No,	No,	No,
[0x50]	Home,	Left,	Up,	Right,	Down,	Pgup,	Pgdown,	No,
[0x58]	No,	No,	No,	No,	No,	No,	No,	No,
[0x60]	No,	Print,	No,	Ins,	No,	No,	No,	No,
[0x68]	No,	No,	No,	Break,	No,	No,	No,	No,
[0x70]	No,	No,	No,	No,	No,	No,	No,	No,
[0x78]	No,	No,	No,	No,	No,	No,	No,	Num,
[0x80]	No,	No,	No,	No,	No,	No,	No,	No,
[0x88]	No,	No,	No,	No,	No,	No,	No,	No,
[0x90]	No,	No,	No,	No,	No,	No,	No,	No,
[0x98]	No,	No,	No,	No,	No,	No,	No,	No,
[0xa0]	No,	No,	No,	No,	No,	No,	No,	No,
[0xa8]	No,	No,	'*',	'+',	No,	'-',	'.',	'/',
[0xb0]	'0',	'1',	'2',	'3',	'4',	'5',	'6',	'7',
[0xb8]	'8',	'9',	No,	No,	No,	'=',	No,	No,
[0xc0]	No,	No,	No,	No,	No,	No,	No,	No,
[0xc8]	No,	No,	No,	No,	No,	No,	No,	No,
[0xd0]	No,	No,	No,	No,	No,	No,	No,	No,
[0xd8]	No,	No,	No,	No,	No,	No,	No,	No,
[0xe0]	No,	Shift,	Shift,	Ctrl,	Ctrl,	Caps,	Caps,	No,
[0xe8]	No,	Latin,	Latin,	No,	No,	No,	No,	No,
[0xf0]	No,	No,	No,	No,	No,	No,	No,	No,
[0xf8]	No,	No,	No,	No,	No,	No,	No,	Delete,
};

/*
 *  keyboard interrupt
 */
void
vncputc(int keyup, int c)
{
	int i;
	static int esc1, esc2;
	static int alt, caps, ctl, num, shift;
	static int collecting, nk;
	static Rune kc[5];

	if(caps && c<='z' && c>='a')
		c += 'A' - 'a';

	/*
 	 *  character mapping
	 */
	if((c & VKSpecial) == VKSpecial){
		c = vnckeys[c & 0xff];
		if(c == No)
			return;
	}
	/*
	 * map an xkeysym onto a utf-8 char
	 */
	if((c & 0xff00) && c < nelem(ksym2utf) && ksym2utf[c] != 0)
			c = ksym2utf[c];

	/*
	 *  keyup only important for shifts
	 */
	if(keyup){
		switch(c){
		case Latin:
			alt = 0;
			break;
		case Shift:
			shift = 0;
			break;
		case Ctrl:
			ctl = 0;
			break;
		}
		return;
	}

	/*
 	 *  normal character
	 */
	if(!(c & (Spec|KF))){
		if(ctl){
			c &= 0x1f;
		}
		if(!collecting){
			kbdputc(c);
			return;
		}
		kc[nk++] = c;
		c = latin1(kc, nk);
		if(c < -1)	/* need more keystrokes */
			return;
		if(c != -1)	/* valid sequence */
			kbdputc(c);
		else	/* dump characters */
			for(i=0; i<nk; i++)
				kbdputc(kc[i]);
		nk = 0;
		collecting = 0;
		return;
	}else{
		switch(c){
		case Caps:
			caps ^= 1;
			return;
		case Num:
			num ^= 1;
			return;
		case Shift:
			shift = 1;
			return;
		case Latin:
			alt = 1;
			collecting = 1;
			nk = 0;
			return;
		case Ctrl:
			ctl = 1;
			return;
		}
	}
	kbdputc(c);
}
