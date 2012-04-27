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

enum {
	Button1 = 0x1;
	Button2 = 0x2;
	Button3 = 0x4;
};

int		buttons;
Point	position;

static void
mousevent(void) {
	static int		curbuttons;
	static Point	curposition;

	if (buttons == curbuttons && eqpt(position, curposition))
		return;

	/* generate a mouse event */
	curbuttons = buttons;
	curposition = position;
}

void
buttonevent(int event) {
	switch (event) {
	case 0x02:
		/* Button 2 down */
		buttons |= Button2;
		mousevent();
		break;
	case 0x82:
		/* Button 2 up */
		buttons &= ~Button2;
		mousevent();
		break;
	case 0x03:
		/* Button 3 down */
		buttons |= Button3;
		mousevent();
		break;
	case 0x83:
		/* Button 3 up */
		buttons &= ~Button3;
		mousevent();
		break;
	default:
		/* other buttons */
	}
}
