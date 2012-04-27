#include <u.h>
#include <libc.h>
#include <draw.h>
#include <event.h>

/*
 * Convert AccuPoint buttons 4 and 5 to a simulation of button 2.
 * The buttons generate down events, repeat, and have no up events,
 * so it's a struggle. This program turns the left button into a near-as-
 * possible simulation of a regular button 2, but it can only sense up
 * events by timeout, so it's sluggish.  Thus it also turns the right button
 * into a click on button 2, useful for acme and chords.
 */

typedef struct M M;

struct M
{
	Mouse;
	int	byte;
};

int	button2;
int	interrupted;

int
readmouse(M *m)
{
	char buf[1+4*12];
	int n;

	n = read(0, buf, sizeof buf);
	if(n < 0)
		return n;
	if(n != sizeof buf)
		return 0;
	m->byte = buf[0];
	m->xy.x =  atoi(buf+1+0*12);
	m->xy.y =  atoi(buf+1+1*12);
	m->buttons =  atoi(buf+1+2*12);
	m->msec =  atoi(buf+1+3*12);
	return 1;
}

void
writemouse(M *m)
{
	print("%c%11d %11d %11d %11ld ",
		m->byte,
		m->xy.x,
		m->xy.y,
		m->buttons&7,
		m->msec);
}

void
notifyf(void*, char *s)
{
	if(strcmp(s, "alarm") == 0)
		interrupted = 1;
	noted(NCONT);
}

void
main(void)
{
	M m, om;
	int n;

	notify(notifyf);
	memset(&m, 0, sizeof m);
	om = m;
	for(;;){
		interrupted = 0;
		/* first click waits 500ms before repeating; after that they're 150, but that's ok */
		if(button2)
			alarm(550);
		n = readmouse(&m);
		if(button2)
			alarm(0);
		if(interrupted){
			/* timed out; clear button 2 */
			om.buttons &= ~2;
			button2 = 0;
			writemouse(&om);
			continue;
		}
		if(n <= 0)
			break;
		/* avoid bounce caused by button 5 click */
		if((om.buttons&16) && (m.buttons&16)){
			om.buttons &= ~16;
			continue;
		}
		if(m.buttons & 2)
			button2 = 0;
		else{
			/* only check 4 and 5 if 2 isn't down of its own accord */
			if(m.buttons & 16){
				/* generate quick button 2 click */
				button2 = 0;
				m.buttons |= 2;
				writemouse(&m);
				m.buttons &= ~2;
				/* fall through to generate up event */
			}else if(m.buttons & 8){
				/* press and hold button 2 */
				button2 = 1;
			}
		}
		if(button2)
			m.buttons |= 2;
		if(m.byte!=om.byte || m.buttons!=om.buttons || !eqpt(m.xy, om.xy))
			writemouse(&m);
		om = m;
	}
}
