#include	"type.h"

typedef	struct	Fun	Fun;
struct	Fun
{
	void	(*init)(void);
	void	(*updat)(void);
};
Fun	fun[];

void
test(void)
{
}

void
ereshaped(Rectangle r)
{
	USED(r);
}

void
main(int argc, char *argv[])
{
	int i;
	long to, tn;

	USED(argc);
	USED(argv);
	binit(0, 0, 0);
	screen.r = inset(screen.r, 4);
	rectf(D, screen.r, F_CLR);
	einit(Emouse|Ekeyboard);
	plane.time = 0;
	for(i=0; fun[i].init; i++) {
		(*fun[i].init)();
	}
	test();
	bflush();
	to = realtime();

	for(;;) {
		tn = realtime();
		if(tn < to) {
			nap();
			continue;
		}
		to += 6;
		for(i=0; fun[i].init; i++) {
			(*fun[i].updat)();
		}
		plane.time++;
		if(button(2))
		if(!plane.nbut) {
			for(i=0; i<30; i++) {
				plupdat();
				plane.time++;
			}
			to = realtime();
		}
		bflush();
	}
	exits(0);
}

void
blinit(void)
{
	Rectangle o;
	int tx, ty, t;

	o = Drect;
	tx = o.max.x - o.min.x;
	ty = o.max.y - o.min.y;
	t = muldiv(tx, 1024, 800);
	if(t > ty) {
		tx -= muldiv(ty, 800, 1024);
		o.min.x += tx/2;
		o.max.x -= tx/2;
		tx = muldiv(ty, 800, 1024);
	}
	plane.side = muldiv(tx, 200, 800);
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
	int c;

	plane.obut = plane.nbut;
	while(ecankbd()) {
		c = ekbd();
		c &= 0177;
		if(c >= 'a' && c <= 'z')
			c += 'A'-'a';
		plane.key[0] = plane.key[1];
		plane.key[1] = plane.key[2];
		plane.key[2] = plane.key[3];
		plane.key[3] = c;
		if(c == '\n' || c == '\r')
			setxyz(plane.key[0], plane.key[1], plane.key[2]);
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
	rocinit, rocupdat,
	pwrinit, pwrupdat,
	clkinit, clkupdat,
	altinit, altupdat,
	attinit, attupdat,
	tabinit, tabupdat,
	asiinit, asiupdat,
	dgyinit, dgyupdat,
	inpinit, inpupdat,
	0
};
