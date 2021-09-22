#include	"type.h"

typedef	struct	Fun	Fun;
struct	Fun
{
	void	(*init)(void);
	void	(*updat)(void);
};
Fun	fun[];

void
eresized(int new)
{
	int i;

	if(new && getwindow(display, Refmesg) < 0) {
		print("can't reattach to window\n");
		exits("reshap");
	}
	d_clr(D, screen->r);
//	screen->r = inset(screen->r, 4);
	for(i=0; fun[i].init; i++) {
		(*fun[i].init)();
	}
	d_bflush();
}

void
main(int argc, char *argv[])
{
	int i;
	long to, tn;

	ARGBEGIN {
	default:
		fprint(2, "usage: a.out [-x]\n");
		exits(0);
	case 'x':
		proxflag = 1;
		break;
	case 't':
		plane.trace = Bopen("/tmp/fsimtrace", OWRITE);
		break;
	} ARGEND

	datmain();
	d_binit();
	d_clr(D, screen->r);
	einit(Emouse|Ekeyboard);
	plstart();
	eresized(0);

	to = realtime();
	for(;;) {
		tn = realtime();
		if(tn < to) {
			nap();
			continue;
		}
		to += 6;
		for(i=0; fun[i].init; i++)
			(*fun[i].updat)();
		plane.time++;
		if(button(2))
		if(!plane.nbut) {
			for(i=0; i<30; i++) {
				plupdat();
				plane.time++;
			}
			to = realtime();
		}
		d_bflush();
	}
}

void
blinit(void)
{
	Rectangle o;
	int tx, ty, t;

	o = Drect;
	tx = o.max.x - o.min.x;
	ty = o.max.y - o.min.y;
	t = muldiv(tx, 1024, 1000);
	if(t > ty) {
		tx -= muldiv(ty, 1000, 1024);
		o.min.x += tx/2;
		o.max.x -= tx/2;
		tx = muldiv(ty, 1000, 1024);
	}
	plane.side = muldiv(tx, 200, 1000);
	plane.side14 = plane.side/4;
	plane.side24 = plane.side/2;
	plane.side34 = plane.side14+plane.side24;
	plane.origx = o.min.x;
	plane.origy = o.max.y - 3 * plane.side;
	plane.obut = 0;
	plane.obut = button(1);
	plane.nbut = plane.obut;
	plane.tempx = plane.origx + 0*plane.side + plane.side24 - 3*W;
	plane.tempy = plane.origy + 2*plane.side + plane.side24 + (3*H)/2;
}

void
blupdat(void)
{
	int c, i;

	plane.obut = plane.nbut;
	while(ecankbd()) {
		c = ekbd();
		c &= 0177;
		if(c >= 'a' && c <= 'z')
			c += 'A'-'a';
		if(c == '\n' || c == '\r')
			c = 0;
		for(i=1; i<nelem(plane.key); i++)
			plane.key[i-1] = plane.key[i];
		plane.key[i-1] = c;
		if(c == 0) {
			for(i=nelem(plane.key)-4; i>=0; i--)
			if(setxyz(plane.key+i)) {
				setorg();
				break;
			}
		}
	}
	plane.nbut = button(1);
	if(plane.nbut != plane.obut) {
		plane.butx = mouse.xy.x;
		plane.buty = mouse.xy.y;
	}
	if(plane.nbut)
	if(button(2))
	if(button(3))
		exits(0);
}

Fun	fun[] =
{
	blinit, blupdat,
	mginit, mgupdat,
	plinit, plupdat,
	apinit, apupdat,
	nvinit, nvupdat,
	vorinit, vorupdat,
	adfinit, adfupdat,
	mkbinit, mkbupdat,
	vsiinit, vsiupdat,
	pwrinit, pwrupdat,
	clkinit, clkupdat,
	altinit, altupdat,
	attinit, attupdat,
	tabinit, tabupdat,
	asiinit, asiupdat,
	dgyinit, dgyupdat,
	inpinit, inpupdat,
	gpsinit, gpsupdat,
	0
};
