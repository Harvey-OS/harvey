/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

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
	Mouse Mouse;
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
	m->Mouse.xy.x =  atoi(buf+1+0*12);
	m->Mouse.xy.y =  atoi(buf+1+1*12);
	m->Mouse.buttons =  atoi(buf+1+2*12);
	m->Mouse.msec =  atoi(buf+1+3*12);
	return 1;
}

void
writemouse(M *m)
{
	print("%c%11d %11d %11d %11ld ",
		m->byte,
		m->Mouse.xy.x,
		m->Mouse.xy.y,
		m->Mouse.buttons&7,
		m->Mouse.msec);
}

void
notifyf(void *v, char *s)
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
			om.Mouse.buttons &= ~2;
			button2 = 0;
			writemouse(&om);
			continue;
		}
		if(n <= 0)
			break;
		/* avoid bounce caused by button 5 click */
		if((om.Mouse.buttons&16) && (m.Mouse.buttons&16)){
			om.Mouse.buttons &= ~16;
			continue;
		}
		if(m.Mouse.buttons & 2)
			button2 = 0;
		else{
			/* only check 4 and 5 if 2 isn't down of its own accord */
			if(m.Mouse.buttons & 16){
				/* generate quick button 2 click */
				button2 = 0;
				m.Mouse.buttons |= 2;
				writemouse(&m);
				m.Mouse.buttons &= ~2;
				/* fall through to generate up event */
			}else if(m.Mouse.buttons & 8){
				/* press and hold button 2 */
				button2 = 1;
			}
		}
		if(button2)
			m.Mouse.buttons |= 2;
		if(m.byte!=om.byte || m.Mouse.buttons!=om.Mouse.buttons || !eqpt(m.Mouse.xy, om.Mouse.xy))
			writemouse(&m);
		om = m;
	}
}
