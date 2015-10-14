/*
 * This file is part of Harvey.
 *
 * Copyright (C) 2015 Giacomo Tesio <giacomo@tesio.it>
 *
 * Harvey is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3 of the License.
 *
 * Harvey is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Harvey.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <u.h>
#include <libc.h>

#include "gconsole.h"

enum {
	Spec =	0xF800,		/* Unicode private space */
		PF =	Spec|0x20,	/* num pad function key */
		View =	Spec|0x00,	/* view (shift window up) */
		Shift =	Spec|0x60,
		Break =	Spec|0x61,
		Ctrl =	Spec|0x62,
		kbAlt =	Spec|0x63,
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
[0x38]	kbAlt,	' ',	Ctrl,	KF|1,	KF|2,	KF|3,	KF|4,	KF|5,
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
[0x38]	kbAlt,	' ',	Ctrl,	KF|1,	KF|2,	KF|3,	KF|4,	KF|5,
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
[0x38]	kbAlt,	No,	Ctrl,	'\x05',	'\x06',	'\x07',	'\x04',	'\x05',
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
keybscan(uint8_t code, uint8_t *out, int len)
{
	Rune c;
	int keyup;
	int off;

	debug("kbscan: code 0x%x out %p len %d\n", code, out, len);
	c = code;
	off = 0;
	if(len < 16){
		debug("keybscan: input buffer too short to be safe\n");
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
			debug("unknown key %ux\n", c);
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
		case kbAlt:
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
		off += runetochar((char*)out+off, &c);
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
		case kbAlt:
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

	off += runetochar((char*)out+off, &c);
	return off;

}

/* read from src, write to consreads (and conswrite,
 * if a visual feedback is required)
 */
void
readkeyboard(int src, int consreads, int conswrites)
{
	int32_t pid, r, w, fb, scan, consumed, produced;
	static uint8_t ibuf[InputBufferSize];
	static uint8_t obuf[OutputBufferSize];

	pid = getpid();

	debug("readkeyboard %d: started\n", pid);

	do
	{
		w = 1; /* do not exit after a partial key scan */
		produced = 0;
		consumed = 0;
		r = read(src, ibuf, InputBufferSize);
		debug("readkeyboard %d: read(%d) %d\n", pid, src, r);
		while(consumed < r){
			scan = keybscan(ibuf[consumed], obuf+produced, OutputBufferSize-produced);
			debug("readkeyboard %d: ibuf[%d]=0x%x '%c', scan=%d\n", pid, consumed, ibuf[consumed], ibuf[consumed], scan);
			if(scan == -1){
				debug("readkeyboard %d: keybscan returns -1\n", pid);
				consumed = r;	/* discard the rest, should we return here? */
			} else {
				produced += scan;
				consumed++;
			}
		}
		if(produced){
			w = write(consreads, obuf, produced);
			debug("readkeyboard %d: write(consreads) returns %d\n", pid, w);
			if(!rawmode) {
				/* give a feedback to the user
				 *
				 * note: conswrites is a pipe(2) preserving record boundaries
				 *       thus it should be safe to write it concurrently
				 */
				fb = write(conswrites, obuf, produced);
				debug("readkeyboard %d: write(conswrites) returns %d\n", pid, fb);
			}
		}
	}
	while(r > 0 && w > 0);

	debug("input %d: shut down (r = %d, w = %d)\n", pid, r, w);
	close(src);
	debug("input %d: close(%d)\n", pid, src);
	close(consreads);
	debug("input %d: close(%d)\n", pid, consreads);

	if(r < 0)
		exits("read");
	if(w < 0)
		exits("write");
	exits(nil);
}
