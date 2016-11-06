/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "../port/error.h"
#include "io.h"

#define	Image	IMAGE
#include <draw.h>
#include <memdraw.h>
#include <cursor.h>
#include "screen.h"

/*
 *  mouse types
 */
enum
{
	Mouseother=	0,
	Mouseserial=	1,
	MousePS2=	2,
};

extern int mouseshifted;

static QLock mousectlqlock;
static int mousetype;
static int intellimouse;
static int packetsize;
static int resolution;
static int accelerated;
static int mousehwaccel;

enum
{
	CMaccelerated,
	CMhwaccel,
	CMintellimouse,
	CMlinear,
	CMps2,
	CMps2intellimouse,
	CMres,
	CMreset,
	CMserial,
};

static Cmdtab mousectlmsg[] =
{
	CMaccelerated,		"accelerated",		0,
	CMhwaccel,		"hwaccel",		2,
	CMintellimouse,		"intellimouse",		1,
	CMlinear,		"linear",		1,
	CMps2,			"ps2",			1,
	CMps2intellimouse,	"ps2intellimouse",	1,
	CMres,			"res",			0,
	CMreset,		"reset",		1,
	CMserial,		"serial",		0,
};

/*
 *  ps/2 mouse message is three bytes
 *
 *	byte 0 -	0 0 SDY SDX 1 M R L
 *	byte 1 -	DX
 *	byte 2 -	DY
 *
 *  shift & right button is the same as middle button
 *
 * Intellimouse and AccuPoint with extra buttons deliver
 *	byte 3 -	00 or 01 or FF according to extra button state.
 * extra buttons are mapped in this code to buttons 4 and 5.
 * AccuPoint generates repeated events for these buttons;
*  it and Intellimouse generate 'down' events only, so
 * user-level code is required to generate button 'up' events
 * if they are needed by the application.
 * Also on laptops with AccuPoint AND external mouse, the
 * controller may deliver 3 or 4 bytes according to the type
 * of the external mouse; code must adapt.
 *
 * On the NEC Versa series (and perhaps others?) we seem to
 * lose a byte from the packet every once in a while, which
 * means we lose where we are in the instruction stream.
 * To resynchronize, if we get a byte more than two seconds
 * after the previous byte, we assume it's the first in a packet.
 */

/*
 *  set up a ps2 mouse
 */
static void
ps2mouse(void)
{
	if(mousetype == MousePS2)
		return;

	if (0) /* For now, this is done in main.c */
		mouseenable();
	/* make mouse streaming, enabled */
	mousecmd(0xEA);
	mousecmd(0xF4);

	mousetype = MousePS2;
	packetsize = 3;
	mousehwaccel = 1;
}

/*
 * The PS/2 Trackpoint multiplexor on the IBM Thinkpad T23 ignores
 * acceleration commands.  It is supposed to pass them on
 * to the attached device, but my Logitech mouse is simply
 * not behaving any differently.  For such devices, we allow
 * the user to use "hwaccel off" to tell us to back off to
 * software acceleration even if we're using the PS/2 port.
 * (Serial mice are always software accelerated.)
 * For more information on the Thinkpad multiplexor, see
 * http://wwwcssrv.almaden.ibm.com/trackpoint/
 */
static void
setaccelerated(int x)
{
	accelerated = x;
	if(mousehwaccel){
		switch(mousetype){
		case MousePS2:
			mousecmd(0xE7);
			return;
		}
	}
	mouseaccelerate(x);
}

static void
setlinear(void)
{
	accelerated = 0;
	if(mousehwaccel){
		switch(mousetype){
		case MousePS2:
			mousecmd(0xE6);
			return;
		}
	}
	mouseaccelerate(0);
}

static void
setres(int n)
{
	resolution = n;
	switch(mousetype){
	case MousePS2:
		mousecmd(0xE8);
		mousecmd(n);
		break;
	}
}

static void
setintellimouse(void)
{
	intellimouse = 1;
	packetsize = 4;
	switch(mousetype){
	case MousePS2:
		mousecmd(0xF3);	/* set sample */
		mousecmd(0xC8);
		mousecmd(0xF3);	/* set sample */
		mousecmd(0x64);
		mousecmd(0xF3);	/* set sample */
		mousecmd(0x50);
		break;
	}
}

static void
resetmouse(void)
{
	packetsize = 3;
	switch(mousetype){
	case MousePS2:
		mousecmd(0xF6);
		mousecmd(0xEA);	/* streaming */
		mousecmd(0xE8);	/* set resolution */
		mousecmd(3);
		mousecmd(0xF4);	/* enabled */
		break;
	}
}

void
mousectl(Cmdbuf *cb)
{
	Proc *up = externup();
	Cmdtab *ct;

	qlock(&mousectlqlock);
	if(waserror()){
		qunlock(&mousectlqlock);
		nexterror();
	}

	ct = lookupcmd(cb, mousectlmsg, nelem(mousectlmsg));
	switch(ct->index){
	case CMaccelerated:
		setaccelerated(cb->nf == 1 ? 1 : atoi(cb->f[1]));
		break;
	case CMintellimouse:
		setintellimouse();
		break;
	case CMlinear:
		setlinear();
		break;
	case CMps2:
		intellimouse = 0;
		ps2mouse();
		break;
	case CMps2intellimouse:
		ps2mouse();
		setintellimouse();
		break;
	case CMres:
		if(cb->nf >= 2)
			setres(atoi(cb->f[1]));
		else
			setres(1);
		break;
	case CMreset:
		resetmouse();
		if(accelerated)
			setaccelerated(accelerated);
		if(resolution)
			setres(resolution);
		if(intellimouse)
			setintellimouse();
		break;
	case CMhwaccel:
		if(strcmp(cb->f[1], "on")==0)
			mousehwaccel = 1;
		else if(strcmp(cb->f[1], "off")==0)
			mousehwaccel = 0;
		else
			cmderror(cb, "bad mouse control message");
	}

	qunlock(&mousectlqlock);
	poperror();
}
