#include <u.h>
#include <libc.h>
#include <libg.h>
#include <frame.h>
#include "flayer.h"
#include "samterm.h"

int	cursorfd;
int	input;
int	got;
int	block;
int	kbdc;
int	reshaped;
uchar	*hostp;
uchar	*hoststop;
void	panic(char*);

void
initio(void){
	einit(Emouse|Ekeyboard);
	estart(Ehost, 0, 0);
}

void
frgetmouse(void)
{
	mouse = emouse();
}

void
mouseunblock(void)
{
	got &= ~Emouse;
}

void
kbdblock(void)
{		/* ca suffit */
	block = Ekeyboard;
}

int
button(int but)
{
	frgetmouse();
	return mouse.buttons&(1<<(but-1));
}

int
waitforio(void)
{
	ulong type;
	static Event e;

	if(got & ~block)
		return got & ~block;
	type = eread(~(got|block), &e);
	switch(type){
	case Ehost:
		hostp = e.data;
		hoststop = hostp + e.n;
		block = 0;
		break;
	case Ekeyboard:
		kbdc = e.kbdc;
		break;
	case Emouse:
		mouse = e.mouse;
		break;
	}
	got |= type;
	return got; 
}

int
rcvchar(void)
{
	int c;

	if(!(got & Ehost))
		return -1;
	c = *hostp++;
	if(hostp == hoststop)
		got &= ~Ehost;
	return c;
}

char*
rcvstring(void)
{
	*hoststop = 0;
	got &= ~Ehost;
	return (char*)hostp;
}

int
getch(void)
{
	int c;

	while((c = rcvchar()) == -1){
		block = ~Ehost;
		waitforio();
		block = 0;
	}
	return c;
}

int
kbdchar(void)
{
	int c;

	if(got & Ekeyboard){
		c = kbdc;
		kbdc = -1;
		got &= ~Ekeyboard;
		return c;
	}
	if(!ecankbd())
		return -1;
	return ekbd();
}

int
qpeekc(void)
{
	return kbdc;
}

void
ereshaped(Rectangle r)
{
	USED(r);

	reshaped = 1;
}

int
RESHAPED(void)
{
	if(reshaped){
		screen.r = bscreenrect(&screen.clipr);
		reshaped = 0;
		return 1;
	}
	return 0;
}

void
mouseexit(void)
{
	exits(0);
}
