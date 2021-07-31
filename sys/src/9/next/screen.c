#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"io.h"
#include	"ureg.h"
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

Lock	screenlock;

void	duartinit(void);
int	duartacr;
int	duartimr;

void	(*kprofp)(ulong);

GBitmap	gscreen =
{
	(ulong*)DISPLAYRAM,	/* BUG */
	0,
	2*1152/32,
	1,
	{ 0, 0, 1120, 832, },
	{ 0, 0, 1120, 832, },
	0
};

void
screeninit(void)
{
	void bigcursor(void);
	bigcursor();

	defont = &defont0;	/* save space; let bitblt do the conversion work */
	gbitblt(&gscreen, Pt(0, 0), &gscreen, gscreen.r, 0); /**/
	out.pos.x = MINX;
	out.pos.y = 0;
	out.bwid = defont0.info[' '].width;
}

void
screenputs(char *s, int n)
{
	Rune r;
	int i;
	char buf[4];

	if(getsr() & 0x0700){
		if(!canlock(&screenlock))
			return;	/* don't deadlock trying to print in interrupt */
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
		if(r == '\n'){
			out.pos.x = MINX;
			out.pos.y += defont0.height;
			if(out.pos.y > gscreen.r.max.y-defont0.height)
				out.pos.y = gscreen.r.min.y;
			gbitblt(&gscreen, Pt(0, out.pos.y), &gscreen,
			    Rect(0, out.pos.y, gscreen.r.max.x, out.pos.y+2*defont0.height), 0);
		}else if(r == '\t'){
			out.pos.x += (8-((out.pos.x-MINX)/out.bwid&7))*out.bwid;
			if(out.pos.x >= gscreen.r.max.x)
				screenputs("\n", 1);
		}else if(r == '\b'){
			if(out.pos.x >= out.bwid+MINX){
				out.pos.x -= out.bwid;
				screenputs(" ", 1);
				out.pos.x -= out.bwid;
			}
		}else{
			if(out.pos.x >= gscreen.r.max.x-out.bwid)
				screenputs("\n", 1);
			out.pos = gsubfstring(&gscreen, out.pos, defont, buf, S);
		}
	}
	unlock(&screenlock);
}

#include	"keys.h"

void
kbdstate(int ch)
{
	static int lstate, k1, k2;
	static uchar kc[5];
	int c, i, nk;

	if(ch == 0xff){
		lstate = 0;
		return;
	}
	if(ch == -1){
		lstate = 1;
		return;
	}
	switch(lstate){
	case 1:
		kc[0] = ch;
		lstate = 2;
		if(ch == 'X')
			lstate = 3;
		break;
	case 2:
		kc[1] = ch;
		c = latin1(kc);
		nk = 2;
	putit:
		lstate = 0;
		if(c != -1)
			kbdputc(&kbdq, c);
		else for(i=0; i<nk; i++)
			kbdputc(&kbdq, kc[i]);
		break;
	case 3:
	case 4:
	case 5:
		kc[lstate-2] = ch;
		lstate++;
		break;
	case 6:
		kc[4] = ch;
		c = unicode(kc);
		nk = 5;
		goto putit;
	default:
		kbdrepeat(1);
		kbdputc(&kbdq, ch);
	}
}

int altcmd;	/* state of left alt and command keys */

void
kbdmouseintr(void)
{
	int c;
	ulong d;
	int dx, dy;
	static int b;

	if(*(ulong*)MONCSR & (1<<23)){
		if(*(ulong*)MONCSR & (1<<22)){
			d = *(ulong*)KBDDATA;
			if(d & (0xF<<24)){
				if((d&0x01) == 0)
					b |= 1;
				else
					b &= ~1;
				if((d&0x100) == 0)
					b |= mouseshifted ? 2 : 4;
				else
					b &= ~6;
				dx = (d>>1) & 0x7F;
				dy = (d>>9) & 0x7F;
				if(dx & 0x40)
					dx |= ~0x7F;
				if(dy & 0x40)
					dy |= ~0x7F;
				mousedelta(b, 3*-dx, 3*-dy);
			}else{
				/* alt key + right mouse button == middle mouse button */
				if((d&0x2000))
					altcmd |= 1;
				else if(!(d&0x2000))
					altcmd &= ~1;
				mouseshifted = altcmd & 1;
				if((d&0x800))		/* left command */
					altcmd |= 2;
				else if(!(d&0x800))
					altcmd &= ~2;
				if(d & 0x4000){	/* right alt */
					kbdstate(-1);
					goto out;
				}
				if(d & 0x8000){	/* valid data */
					if(d & 0x80){	/* up */
						kbdrepeat(0);
						goto out;
					}
					if(d & 0x100)
						c = keyc[d&0x7f];
					else if(d & 0x600)
						c = keys[d&0x7f];
					else
						c = key0[d&0x7F];
					kbdstate(c);
				}
			}
		}
    out:
		if(*(ulong*)MONCSR & (1<<21)){
			/* keyboard overrun */
			*(ulong*)MONCSR |= (1<<21);
		}
	}
	if(*(ulong*)MONCSR & (1<<19)){
		print("kms interrupt\n");
		if(*(ulong*)MONCSR & (1<<17)){
			print("kms overrun\n");
			*(ulong*)MONCSR |= (1<<17);
		}
		if(*(ulong*)MONCSR & (1<<18))
			print("kmsdata %lux\n", *(ulong*)KMSDATA);
	}
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
	return 2;	/* bits per pixel */
}

void
getcolor(ulong p, ulong *pr, ulong *pg, ulong *pb)
{
	ulong ans;

	/*
	 * The next says 0 is white (max intensity)
	 */
	switch(p){
	case 0:		ans = ~0;		break;
	case 1:		ans = 0xAAAAAAAA;	break;
	case 2:		ans = 0x55555555;	break;
	default:	ans = 0;		break;
	}
	*pr = *pg = *pb = ans;
}

int
setcolor(ulong p, ulong r, ulong g, ulong b)
{
	USED(p, r, g, b);
	return 0;	/* can't change mono screen colormap */
}

void
mouseclock(void)	/* called splhi */
{
	mouseupdate(1);
}

/*
 *  a fatter than usual cursor for the safari
 */
Cursor fatarrow = {
	{ -1, -1 },
	{
		0xff, 0xff, 0x80, 0x01, 0x80, 0x02, 0x80, 0x0c, 
		0x80, 0x10, 0x80, 0x10, 0x80, 0x08, 0x80, 0x04, 
		0x80, 0x02, 0x80, 0x01, 0x80, 0x02, 0x8c, 0x04, 
		0x92, 0x08, 0x91, 0x10, 0xa0, 0xa0, 0xc0, 0x40, 
	},
	{
		0x00, 0x00, 0x7f, 0xfe, 0x7f, 0xfc, 0x7f, 0xf0, 
		0x7f, 0xe0, 0x7f, 0xe0, 0x7f, 0xf0, 0x7f, 0xf8, 
		0x7f, 0xfc, 0x7f, 0xfe, 0x7f, 0xfc, 0x73, 0xf8, 
		0x61, 0xf0, 0x60, 0xe0, 0x40, 0x40, 0x00, 0x00, 
	},
};
void
bigcursor(void)
{
	extern Cursor arrow;

	memmove(&arrow, &fatarrow, sizeof(fatarrow));
}

/*
 *  setup a serial mouse
 */
static void
serialmouse(int port, char *type, int setspeed)
{
	if(mousetype)
		error(Emouseset);

	if(port >= 2 || port < 0)
		error(Ebadarg);

	/* set up /dev/eia0 as the mouse */
	sccspecial(port, 0, &mouseq, setspeed ? 1200 : 0);
	if(type && *type == 'M')
		mouseq.putc = m3mouseputc;
	mousetype = Mouseserial;
}

/*
 *  set/change mouse configuration
 */
void
mousectl(char *arg)
{
	int n;
	char *field[3];

	n = getfields(arg, field, 3, ' ');
	if(strncmp(field[0], "serial", 6) == 0){
		switch(n){
		case 1:
			serialmouse(atoi(field[0]+6), 0, 1);
			break;
		case 2:
			serialmouse(atoi(field[1]), 0, 0);
			break;
		case 3:
		default:
			serialmouse(atoi(field[1]), field[2], 0);
			break;
		}
	}
}
