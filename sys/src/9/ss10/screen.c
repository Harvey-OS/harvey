#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"io.h"
#include	"../port/error.h"

#include	<libg.h>
#include	<gnot.h>
#include	"screen.h"

#define	MINX	8

extern	GSubfont	defont0;
GSubfont		*defont;

struct{
	Point	pos;
	int	bwid;
}out;

static ulong	rep(ulong, int);
void		(*kprofp)(ulong);

typedef struct	Video Video;
struct Video
{
	/* Brooktree 458/451 */
	uchar	addr;		/* address register */
	uchar	pad0[3];
	uchar	color;		/* color palette */
	uchar	pad1[3];
	uchar	cntrl;		/* control register */
	uchar	pad2[3];
	uchar	ovrl;		/* overlay palette */
	uchar	pad3[3];
#ifdef notright
	/* Sun-4 video chip */
	uchar	mcr;		/* master control register */
	uchar	sr;		/* status register */
	uchar	csa;		/* cursor start address */
	uchar	cea;		/* cursor end address */
	uchar	hbs;		/* horizontal blank set */
	uchar	hbc;		/* horizontal blank clear */
	uchar	hss;		/* horizontal sync set */
	uchar	hsc;		/* horizontal sync clear */
	uchar	csc;		/* composite sync clear */
	uchar	vbsh;		/* vertical blank set high byte */
	uchar	vbsl;		/* vertical blank set low byte */
	uchar	vbc;		/* vertical blank clear */
	uchar	vss;		/* vertical sync set */
	uchar	vsc;		/* vertical sync clear */
	uchar	xcs;		/* transfer cycle hold off set */
	uchar	xcc;		/* transfer cycle hold off clear */
#endif
} *vid;

GBitmap gscreen;

struct screens
{
	char	*type;
	int	x;
	int	y;
	int	ld;
	ulong	vidaddr;
}screens[] = {
	{ "bwtwo", 1152, 900, 0, 0x400000 },
	{ "cgsix", 1152, 900, 3, 0x200000 },
	{ "cgthree", 1152, 900, 3, 0x400000 },	/* data from rwolff */
	0
};

Lock screenlock;

void
screeninit(char *str, int slot)
{
	struct screens *s;
	ulong r, g, b;
	int i;
	int havecol;
	ulong va;

	havecol = 1;
	for(s=screens; s->type; s++)
		if(strcmp(s->type, str) == 0)
			goto found;
	/* default is 0th element of table */
	if(conf.monitor){
		s = screens;
		goto found;
	}
	conf.monitor = 0;
	return;

    found:
	gscreen.zero = 0;
	gscreen.width = (s->x<<s->ld)/(8*sizeof(ulong));

	va = kmapsbus(slot);
	gscreen.base = (ulong*)(va+0x800000);

	gscreen.ldepth = s->ld;
	gscreen.r = Rect(0, 0, s->x, s->y);
	gscreen.clipr = gscreen.r;
	gscreen.cache = 0;
	defont = &defont0;
	gbitblt(&gscreen, Pt(0, 0), &gscreen, gscreen.r, 0);
	out.pos.x = MINX;
	out.pos.y = 0;
	out.bwid = defont0.info[' '].width;
	vid = (Video*)(va+s->vidaddr);

	if(gscreen.ldepth == 3){
		vid->addr = 4;
		vid->cntrl = 0xFF;	/* enable all planes */
		vid->addr = 5;
		vid->cntrl = 0x00;	/* no blinking */
		vid->addr = 6;
		vid->cntrl = 0x43;	/* enable palette ram and display */
		vid->addr = 7;
		vid->cntrl = 0x00;	/* no tests */
		if(havecol) {
		/*
		 * For now, just use a fixed colormap, where pixel i is
		 * regarded as 3 bits of red, 3 bits of green, and 2 bits of blue.
		 * Intensities are inverted so that 0 means white, 255 means black.
		 * Exception: pixels 85 and 170 are set to intermediate grey values
		 * so that 2-bit grey scale images will look ok on this screen.
		 */
			for(i = 0; i<256; i++) {
				r = ~rep((i>>5) & 7, 3);
				g = ~rep((i>>2) & 7, 3);
				b = ~rep(i & 3, 2);
				setcolor(i, r, g, b);
			}
			setcolor(85, 0xAAAAAAAA, 0xAAAAAAAA, 0xAAAAAAAA);
			setcolor(170, 0x55555555, 0x55555555, 0x55555555);
		} else {
			vid->addr = 0;
			for(i=255; i>=0; i--){
				vid->color = i;
				vid->color = i;
				vid->color = i;
			}
		}
	}
}


void
screenputnl(void)
{
	if(!conf.monitor)
		return;
	out.pos.x = MINX;
	out.pos.y += defont0.height;
	if(out.pos.y > gscreen.r.max.y-defont0.height)
		out.pos.y = gscreen.r.min.y;
	gbitblt(&gscreen, Pt(0, out.pos.y), &gscreen,
	    Rect(0, out.pos.y, gscreen.r.max.x, out.pos.y+2*defont0.height), 0);
}

void
screenputs(char *s, int n)
{
	Rune r;
	int i;
	char buf[4];

	if(!conf.monitor) {
		return;
	}

	if((getpsr()&SPL(15))){
		if(!canlock(&screenlock)) {
			return;	/* don't deadlock trying to print in interrupt */
		}
	}else
		lock(&screenlock);
	while(n > 0){
		i = chartorune(&r, s);
		if(i == 0){
			s++;
			--n;
			continue;
		}
		memmove(buf, s, i);
		buf[i] = 0;
		n -= i;
		s += i;
		if(r == '\n')
			screenputnl();
		else if(r == '\t'){
			out.pos.x += (8-((out.pos.x-MINX)/out.bwid&7))*out.bwid;
			if(out.pos.x >= gscreen.r.max.x)
				screenputnl();
		}else if(r == '\b'){
			if(out.pos.x >= out.bwid+MINX){
				out.pos.x -= out.bwid;
				gsubfstring(&gscreen, out.pos, defont, " ", S);
			}
		}else{
			if(out.pos.x >= gscreen.r.max.x-out.bwid)
				screenputnl();
			out.pos = gsubfstring(&gscreen, out.pos, defont, buf, S);
		}
	}
	unlock(&screenlock);
}

/*
 * Map is indexed by keyboard char, output is ASCII.
 * Plan 9-isms:
 * Return sends newline and Line Feed sends carriage return.
 * Del and power/on/off send Delete.
 * Help is an extra ESC.
 * arrow keys are VIEW (scroll).
 * Compose, Alt and Alt Graph build Unicode characters.
 */
uchar keymap[128] = {
/*	00    Stop  spkr  Again spkr! F1    F2    F10	*/
	0xFF, 0x80, 0xFF, 0x81, 0xFF, 0x82, 0x83, 0xFF,
/*	F3    F11   F4    F12   F5    AltGr F6    0f  	*/
	0x84, 0xFF, 0x85, 0xFF, 0x86, 0xb6, 0x87, 0xFF,
/*	F7    F8    F9    Alt   ↑      R1    R2    R3	*/
	0x88, 0x89, 0x8a, 0xb6, 0x80, 0x8c, 0x8d, 0x8e,
/*	←    Props Undo   ↓     →    Esc   1     2	*/
	0x80, 0x8f, 0x90, 0x80, 0x80, 0x1b, '1',  '2',
/*	3     4     5     6     7     8     9     0	*/
	'3',  '4',  '5',  '6',  '7',  '8',  '9',  '0',
/*	-     =     `     bs    Insert~spkr R/    R*	*/
	'-',  '=',  '`',  '\b', 0xFF, 0x91, 0x92, 0x93,
/*	onoff Front R.    Copy  Home  tab   q     w  	*/
	0x7F, 0x94, 0xFF, 0x95, 0xFF, '\t', 'q',  'w',
/*	e     r     t     y     u     i     o     p    	*/
	'e',  'r',  't',  'y',  'u',  'i',  'o',  'p',
/*	[     ]     Del   Comp  R7    R8    R9    R-	*/
	'[',  ']',  0x7f, 0xB6, 0x96, 0x97, 0x98, 0xFF,
/*	Open  Paste End   4b    ctrl  a     s     d	*/
	0x99, 0x9a, 0xFF, 0xFF, 0xF0, 'a',  's',  'd',
/*	f     g     h     j     k     l     ;     '   	*/
	'f',  'g',  'h',  'j',  'k',  'l',  ';',  '\'',
/*	\     ret   Enter R4    R5    R6    R0    Find	*/
	'\\', '\n', 0xFF, 0x9b, 0x9c, 0x9d, 0xFF, 0x9e, 
/*	PgUp  Cut   NumLk shift z     x     c     v	*/
	0xFF, 0x9f, 0x7F, 0xF1, 'z',  'x',  'c', 'v',
/*	b     n     m     ,     .     /     shift lf	*/
	'b',  'n',  'm',  ',',  '.',  '/',  0xF1, '\r',
/*	R1    R2    R3    73    74    75    Help  caps	*/
	0xA0, 0xA1, 0xA2, 0xFF, 0xFF, 0xFF, 0x1b, 0xF2,
/*	L⋄    79    R⋄    PgDn  7c    R+    7e    blank	*/
	0xA3, ' ',  0xA4, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
};

uchar keymapshift[128] = {
/*	00    Stop  spkr  Again spkr! F1    F2    F10	*/
	0xFF, 0x80, 0xFF, 0x81, 0xFF, 0x82, 0x83, 0xFF,
/*	F3    F11   F4    F12   F5    AltGr F6    0f  	*/
	0x84, 0xFF, 0x85, 0xFF, 0x86, 0x80, 0x87, 0xFF,
/*	F7    F8    F9    Alt   ↑     Pause PrScr ScrLk	*/
	0x88, 0x89, 0x8a, 0x8b, 0x80, 0x8c, 0x8d, 0x8e,
/*	←    Props Undo   ↓     →    Esc   1     2	*/
	0x80, 0x8f, 0x90, 0x80, 0x80, 0x1b, '!',  '@',
/*	3     4     5     6     7     8     9     0	*/
	'#',  '$',  '%',  '^',  '&',  '*',  '(',  ')',
/*	-     =     `     bs    Insert~spkr R/    R*	*/
	'_',  '+',  '~',  '\b', 0xFF, 0x91, 0x92, 0x93,
/*	onoff Front R.    Copy  Home  tab   q     w  	*/
	0x7F, 0x94, 0xFF, 0x95, 0xFF, '\t', 'Q',  'W',
/*	e     r     t     y     u     i     o     p    	*/
	'E',  'R',  'T',  'Y',  'U',  'I',  'O',  'P',
/*	[     ]     Del   Comp  R7    R8    R9    R-	*/
	'{',  '}',  0x7f, 0xB6, 0x96, 0x97, 0x98, 0xFF,
/*	Open  Paste End   4b    ctrl  a     s     d	*/
	0x99, 0x9a, 0xFF, 0xFF, 0xF0, 'A',  'S',  'D',
/*	f     g     h     j     k     l     ;     '   	*/
	'F',  'G',  'H',  'J',  'K',  'L',  ':',  '"',
/*	\     ret   Enter R4    R5    R6    R0    Find	*/
	'|', '\n',  0xFF, 0x9b, 0x9c, 0x9d, 0xFF, 0x9e, 
/*	PgUp  Cut   NumLk shift z     x     c     v	*/
	0xFF, 0x9f, 0x7F, 0xF1, 'Z',  'X',  'C', 'V',
/*	b     n     m     ,     .     /     shift lf	*/
	'B',  'N',  'M',  '<',  '>',  '?',  0xF1, '\r',
/*	R1    R2    R3    73    74    75    Help  caps	*/
	0xA0, 0xA1, 0xA2, 0xFF, 0xFF, 0xFF, 0x1b, 0xF2,
/*	L⋄    79    R⋄    PgDn  7c    R+    7e    blank	*/
	0xA3, ' ',  0xA4, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
};

uchar keymapctrl[128] = {
/*	00    Stop  spkr  Again spkr! F1    F2    F10	*/
	0xFF, 0x80, 0xFF, 0x81, 0xFF, 0x82, 0x83, 0xFF,
/*	F3    F11   F4    F12   F5    AltGr F6    0f  	*/
	0x84, 0xFF, 0x85, 0xFF, 0x86, 0x80, 0x87, 0xFF,
/*	F7    F8    F9    Alt   ↑     Pause PrScr ScrLk	*/
	0x88, 0x89, 0x8a, 0x8b, 0x80, 0x8c, 0x8d, 0x8e,
/*	←    Props Undo   ↓     →    Esc   1     2	*/
	0x80, 0x8f, 0x90, 0x80, 0x80, 0x1b, '!',  '@',
/*	3     4     5     6     7     8     9     0	*/
	'#',  '$',  '%',  '^',  '&',  '*',  '(',  ')',
/*	-     =     `     bs    Insert~spkr R/    R*	*/
	'_',  '+',  '~', '\b', 0xFF, 0x91, 0x92, 0x93,
/*	onoff Front R.    Copy  Home  tab   q     w  	*/
	0x7F, 0x94, 0xFF, 0x95, 0xFF, '\t', 0x11, 0x17,
/*	e     r     t     y     u     i     o     p    	*/
	0x05, 0x12, 0x14, 0x19, 0x15, 0x09, 0x0F, 0x10,
/*	[     ]     Del   Comp  R7    R8    R9    R-	*/
	0x1B, 0x1D, 0x7f, 0xB6, 0x96, 0x97, 0x98, 0xFF,
/*	Open  Paste End   4b    ctrl  a     s     d	*/
	0x99, 0x9a, 0xFF, 0xFF, 0xF0, 0x01, 0x13, 0x04,
/*	f     g     h     j     k     l     ;     '   	*/
	0x06, 0x07, 0x08, 0x0A, 0x0B, 0x0C,':',  '"',
/*	\     ret   Enter R4    R5    R6    R0    Find	*/
	0x1C, '\n',  0xFF, 0x9b, 0x9c, 0x9d, 0xFF, 0x9e, 
/*	PgUp  Cut   NumLk shift z     x     c     v	*/
	0xFF, 0x9f, 0x7F, 0xF1, 0x1A, 0x18, 0x03, 0x16,
/*	b     n     m     ,     .     /     shift lf	*/
	0x02, 0x0E, 0x0D, '<',  '>',  '?',  0xF1, '\r',
/*	R1    R2    R3    73    74    75    Help  caps	*/
	0xA0, 0xA1, 0xA2, 0xFF, 0xFF, 0xFF, 0x1b, 0xF2,
/*	L⋄    79    R⋄    PgDn  7c    R+    7e    blank	*/
	0xA3, ' ',  0xA4, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
};

uchar keymapshiftctrl[128] = {
/*	00    Stop  spkr  Again spkr! F1    F2    F10	*/
	0xFF, 0x80, 0xFF, 0x81, 0xFF, 0x82, 0x83, 0xFF,
/*	F3    F11   F4    F12   F5    AltGr F6    0f  	*/
	0x84, 0xFF, 0x85, 0xFF, 0x86, 0x80, 0x87, 0xFF,
/*	F7    F8    F9    Alt   ↑     Pause PrScr ScrLk	*/
	0x88, 0x89, 0x8a, 0x8b, 0x80, 0x8c, 0x8d, 0x8e,
/*	←    Props Undo   ↓     →    Esc   1     2	*/
	0x80, 0x8f, 0x90, 0x80, 0x80, 0x1b, '!',  0x00,
/*	3     4     5     6     7     8     9     0	*/
	'#',  '$',  '%',  0x1E, '&',  '*',  '(',  ')',
/*	-     =     `     bs    Insert~spkr R/    R*	*/
	0x1F, '+',  '~', '\b', 0xFF, 0x91, 0x92, 0x93,
/*	onoff Front R.    Copy  Home  tab   q     w  	*/
	0x7F, 0x94, 0xFF, 0x95, 0xFF, '\t', 0x11, 0x17,
/*	e     r     t     y     u     i     o     p    	*/
	0x05, 0x12, 0x14, 0x19, 0x15, 0x09, 0x0F, 0x10,
/*	[     ]     Del   Comp  R7    R8    R9    R-	*/
	0x1B, 0x1D, 0x7f, 0xB6, 0x96, 0x97, 0x98, 0xFF,
/*	Open  Paste End   4b    ctrl  a     s     d	*/
	0x99, 0x9a, 0xFF, 0xFF, 0xF0, 0x01, 0x13, 0x04,
/*	f     g     h     j     k     l     ;     '   	*/
	0x06, 0x07, 0x08, 0x0A, 0x0B, 0x0C,':',  '"',
/*	\     ret   Enter R4    R5    R6    R0    Find	*/
	0x1C, '\n', 0xFF, 0x9b, 0x9c, 0x9d, 0xFF, 0x9e, 
/*	PgUp  Cut   NumLk shift z     x     c     v	*/
	0xFF, 0x9f, 0x7F, 0xF1, 0x1A, 0x18, 0x03, 0x16,
/*	b     n     m     ,     .     /     shift lf	*/
	0x02, 0x0E, 0x0D, '<',  '>',  '?',  0xF1, '\r',
/*	R1    R2    R3    73    74    75    Help  caps	*/
	0xA0, 0xA1, 0xA2, 0xFF, 0xFF, 0xFF, 0x1b, 0xF2,
/*	L⋄    79    R⋄    PgDn  7c    R+    7e    blank	*/
	0xA3, ' ',  0xA4, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
};

static uchar *kbdmap[4] = {
	keymap,
	keymapshift,
	keymapctrl,
	keymapshiftctrl
};

int
kbdstate(IOQ *q, int c)
{
	static shift = 0x00;
	static caps = 0;
	static long startclick;
	static int repeatc;
	static int collecting, nk;
	static uchar kc[4];
	uchar ch;
	int i;

	USED(q);
	ch = kbdmap[shift][c&0x7F];
	if(c==0x7F){	/* all keys up */
    norepeat:
		kbdrepeat(0);
		return 0;
	}
	if(ch == 0xFF)	/* shouldn't happen; ignore */
		return 0;
	if(c & 0x80){	/* key went up */
		if(ch == 0xF0){		/* control */
			shift &= ~2;
			goto norepeat;
		}
		if(ch == 0xF1){	/* shift */
			shift &= ~1;
			goto norepeat;
		}
		if(ch == 0xF2){	/* caps */
			goto norepeat;
		}
		goto norepeat;
	}
	if(ch == 0xF0){		/* control */
		shift |= 2;
		goto norepeat;
	}
	if(ch==0xF1){	/* shift */
		shift |= 1;
		goto norepeat;
	}
	if(ch==0xF2){	/* caps */
		caps ^= 1;
		goto norepeat;
	}
	if(caps && 'a'<=ch && ch<='z')
		ch |= ' ';
	repeatc = ch;
	kbdrepeat(1);
	if(ch == 0xB6){	/* Compose */
		collecting = 1;
		nk = 0;
	}else{
		if(!collecting){
			kbdputc(&kbdq, ch);
			return 0;
		}
		kc[nk++] = ch;
		c = latin1(kc, nk);
		if(c < -1)	/* need more keystrokes */
			return 0;
		if(c != -1)	/* valid sequence */
			kbdputc(&kbdq, c);
		else	/* dump characters */
			for(i=0; i<nk; i++)
				kbdputc(&kbdq, kc[i]);
		nk = 0;
		collecting = 0;
	}
	return 0;
}

void
buzz(int freq, int dur)
{
	USED(freq, dur);
}

void
lights(int mask)
{
	USED(mask);
}

int
screenbits(void)
{
	return 1;	/* bits per pixel */
}

void
getcolor(ulong p, ulong *pr, ulong *pg, ulong *pb)
{
	uchar r, g, b;
	ulong ans;

	/*
	 * The slc monochrome says 0 is white (max intensity).
	 */
	if(gscreen.ldepth == 0) {
		if(p == 0)
			ans = ~0;
		else
			ans = 0;
		*pr = *pg = *pb = ans;
	} else {
		*(uchar *)&vid->addr = p & 0xFF;
		r = vid->color;
		g = vid->color;
		b = vid->color;
		*pr = (r<<24) | (r<<16) | (r<<8) | r;
		*pg = (g<<24) | (g<<16) | (g<<8) | g;
		*pb = (b<<24) | (b<<16) | (b<<8) | b;
	}
}


int
setcolor(ulong p, ulong r, ulong g, ulong b)
{
	if(gscreen.ldepth == 0)
		return 0;	/* can't change mono screen colormap */
	else{
		vid->addr = p & 0xFF;
		vid->color = r >> 24;
		vid->color = g >> 24;
		vid->color = b >> 24;
		return 1;
	}
}

int
hwgcmove(Point p)
{
	USED(p);
	return 0;
}

void
setcursor(Cursor *curs)
{
	uchar *p;
	int i;
	extern GBitmap set, clr;

	for(i = 0; i < 16; i++){
		p = (uchar*)&set.base[i];
		*p = curs->set[2*i];
		*(p+1) = curs->set[2*i+1];
		p = (uchar*)&clr.base[i];
		*p = curs->clr[2*i];
		*(p+1) = curs->clr[2*i+1];
	}
}

/* replicate (from top) value in v (n bits) until it fills a ulong */
static ulong
rep(ulong v, int n)
{
	int o;
	ulong rv;

	rv = 0;
	for(o = 32 - n; o >= 0; o -= n)
		rv |= (v << o);
	return rv;
}

/*
 *  set/change mouse configuration
 */
void
mousectl(char *arg)
{
	USED(arg);
}
