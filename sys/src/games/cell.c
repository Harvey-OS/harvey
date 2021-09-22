/* cellular automata */
#include <u.h>
#include <libc.h>
#include <bio.h>
#include <draw.h>
#include <event.h>

enum {
	STATE_BITS = 32*1024,
	PX = 1, 		/* cell spacing */
	BX = 1, 		/* box size */
	MODE_NORMAL = 0, 	/* fixed boundary values on left and right */
	MODE_WRAP = 1, 		/* state[n] is same is state[0] */
	MODE_SAME = 2, 		/* left boundary is same as state[0] and */
};				/* right boundary same as state[STATE_BITS-1] */

char	left, right;		/* boundary values */

char	state[STATE_BITS];
char	next_state[STATE_BITS];

Point cen;
Image *box;
int	row;
int	max_row;
int	screen_left, screen_right;	/* extent of display's state */
int	needresize;
int	mode;
int	pause;

static void reshape(void);

static void
g9err(Display *, char *err)
{
	static int entered = 0;

	fprint(2, "%s: %s (%r)\n", argv0, err);
	exits(err);
}

void
init_state(void)
{
	int i;

	for (i = 0; i < STATE_BITS; i++)
		state[i] = 0;
	state[STATE_BITS/2] = 1;
}

static void
setbox(int i, int j)
{
	Point loc;

	loc = addpt(screen->r.min, Pt(j * PX, i * PX));
	draw(screen, Rpt(loc, addpt(loc, Pt(BX, BX))), box, nil, ZP);
}

void
display_state(void)
{
	int i;

	for (i = screen_left; i < screen_right; i++)
		if (state[i] & 1)
			setbox(row, i - screen_left);
}

void
clear_display(void)
{
	draw(screen, screen->r, display->white, nil, ZP);
}

void
redraw(void)
{
	clear_display();
	display_state();
}

int
gen_next(int rule)
{
	int i, s;

	s = 0;
	switch (mode) {
	case MODE_NORMAL:
		s = left << 1 | state[0];
		break;
	case MODE_WRAP:
		s = state[STATE_BITS-1] << 1 | state[0];
		break;
	case MODE_SAME:
		s = state[0] * 3;
		break;
	}

	for (i = 0; i < STATE_BITS - 1; i++) {
		s <<= 1;
		s &= 7;
		s |= state[i+1];
		next_state[i] = ((1 << s) & rule)? 1: 0;
	}
	s <<= 1;
	s &= 7;
	switch (mode) {
	case MODE_NORMAL:
		s |= right;
		break;
	case MODE_WRAP:
		s |= state[0];
		break;
	case MODE_SAME:
		s |= state[STATE_BITS-1];
		break;
	}
	next_state[STATE_BITS-1] = ((1 << s) & rule)? 1: 0;

	/* check for static or zero state	*/
	s = 0;
	for (i = 0; i < STATE_BITS; i++) {
		if (state[i] != next_state[i])
			s = 1;
		state[i] = next_state[i];
	}
	return s;
}

void
usage(void)
{
	fprint(2, "%s: [-g max_generations] [-d ms_delay] rule\n", argv0);
	exits("usage");
}

void
idle(void)
{
	int c;

	while (ecanmouse())
		emouse();			/* throw away mouse events */
	while (ecankbd() || pause) {
		if ((c = ekbd()) == 'q' || c == 0x7F)
			exits(nil);
		if (c == 'p')
			pause ^= 1;
		if (pause)
			sleep(1000);
	}
	if (needresize)
		reshape();
}

void
scroll(void)
{
	draw(screen, screen->r, screen, nil, addpt(screen->r.min, Pt(0, PX)));
	draw(screen, Rpt(Pt(screen->r.min.x, screen->r.max.y - PX),
		Pt(screen->r.max.x, screen->r.max.y)),
		display->white, nil, ZP);
	flushimage(display, 1);
}

void
main(int argc, char **argv)
{
	int rule, delay = 0;
	uint gen, max_gen = -1;

	mode = MODE_NORMAL;
	ARGBEGIN {
	case 'd':
		delay = atoi(EARGF(usage()));
		break;
	case 'g':
		max_gen = atoi(EARGF(usage()));
		break;
	case 's':
		mode = MODE_SAME;
		break;
	case 'w':
		mode = MODE_WRAP;
		break;
	default:
		fprint(2, " badflag('%c')", ARGC());
		break;
	} ARGEND

	if (argc != 1)
		usage();

	rule = atol(argv[0]);

	initdraw(g9err, 0, argv0);
	einit(Emouse | Ekeyboard);	/* implies rawon() */

	max_row = (screen->r.max.y - screen->r.min.y) / PX;
	cen = divpt(subpt(screen->r.max, screen->r.min), 2);
	screen_left = (STATE_BITS / 2) - (cen.x / PX) + (3 * PX);
	screen_right = (STATE_BITS / 2) + ((cen.x + PX - 1) / PX);
	box = allocimage(display, Rect(0, 0, BX, BX), RGB24, 0, DBlack);
	assert(box != nil);

	init_state();
	gen = 0;
	do {
		if (row >= max_row) {
			scroll();
			row--;
		}
		display_state();
		row++;
		flushimage(display, 1);
		if (delay)
			sleep(delay);
		idle();
	} while (gen < max_gen && gen_next(rule));
	for (; ; )
		idle();
}

static void
reshape(void)
{
	sleep(1000);
	needresize = 0;
	cen = divpt(subpt(addpt(screen->r.min, screen->r.max),
		Pt(STATE_BITS * PX, STATE_BITS * PX)), 2);
	redraw();
	flushimage(display, 1);
}

/* called from graphics library */
void
eresized(int callgetwin)
{
	needresize = callgetwin + 1;

	/* new window? */
	/* was using Refmesg */
	if (needresize > 1 && getwindow(display, Refnone) < 0)
		sysfatal("can't reattach to window: %r");

	/* destroyed window? */
	if (Dx(screen->r) == 0 || Dy(screen->r) == 0)
		exits("window gone");

	reshape();
}
