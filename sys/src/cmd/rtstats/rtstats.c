#include <u.h>
#include <libc.h>
#include <thread.h>
#include <ip.h>
#include <bio.h>
#include <stdio.h>
#include <draw.h>
#include <mouse.h>
#include <cursor.h>
#include <keyboard.h>
#include "time.h"
#include "devrealtime.h"

typedef uvlong	Ticks;

char *T = "200ms";
char *Dl = "80ms";
char *C = "40ms";

#define numblocks(a, b)	(((a) + (b) - 1) / (b))
#define roundup(a, b)	(numblocks((a), (b)) * (b))

typedef struct {
	ulong		tid;
	char			*settings;
	int			nevents;	
	Schedevent	*events;
} Task;

enum {
	Nevents = 1000,
	Ncolor = 6,
	K = 1024,
	STACKSIZE = 8 * K,
};

char	*taskdir = "#R/realtime/task";
char	*clonedev = "#R/realtime/clone";
char	*profdev = "#R/realtime/nblog";
char	*timedev = "#R/realtime/time";
Time	prevts;

int	newwin;
int	Width = 1000;		
int	Height = 100;		// Per task
int	topmargin = 8;
int	bottommargin = 4;
int	lineht = 12;

Schedevent *event;

void drawrt(void);
int schedparse(char*, char*, char*);

static char *schedstatename[] = {
	[SRelease] =	"Release",
	[SRun] =		"Run",
	[SPreempt] =	"Preempt",
	[SBlock] =		"Block",
	[SResume] =	"Resume",
	[SDeadline] =	"Deadline",
	[SYield] =		"Yield",
	[SSlice] =		"Slice",
	[SExpel] =		"Expel",
};

static int ntasks, besteffort, verbose;
static Task *tasks;
static Image *cols[Ncolor][3];
static Font *mediumfont, *tinyfont;
static Image *grey, *red, *green, *bg, *fg;

int schedfd;

int
rthandler(void*, char*)
{
	/* On any note */
	if (fprint(schedfd, "remove") < 0)
		sysfatal("Could not remove task: %r");
	sysfatal("Removed task");
	return 1;
}

static void
usage(void)
{
	fprint(2, "Usage: %s [-d profdev] [-t timedev] [-b] [-v]\n", argv0);
	exits(nil);
}

void
threadmain(int argc, char **argv)
{
	ARGBEGIN {
	case 'T':
		T = EARGF(usage());
		break;
	case 'D':
		Dl = EARGF(usage());
		break;
	case 'C':
		C = EARGF(usage());
		break;
	case 'd':
		profdev = EARGF(usage());
		break;
	case 't':
		timedev = EARGF(usage());
		break;
	case 'b':
		besteffort++;
		break;
	case 'v':
		verbose++;
		break;
	case 'w':
		newwin++;
		break;
	default:
		usage();
	}
	ARGEND;

	if (argc != 0)
		usage();

	fmtinstall('T', timeconv);

	drawrt();
}

static void
mkcol(int i, int c0, int c1, int c2)
{
	cols[i][0] = allocimagemix(display, c0, DWhite);
	cols[i][1] = allocimage(display, Rect(0,0,1,1), screen->chan, 1, c1);
	cols[i][2] = allocimage(display, Rect(0,0,1,1), screen->chan, 1, c2);
}

static void
colinit(void)
{
	mediumfont = openfont(display, "/lib/font/bit/lucidasans/unicode.10.font");
	if(mediumfont == nil)
		mediumfont = font;
	tinyfont = openfont(display, "/lib/font/bit/lucidasans/unicode.7.font");
	if(tinyfont == nil)
		tinyfont = font;
	topmargin = mediumfont->height+2;
	bottommargin = tinyfont->height+2;

	/* Peach */
	mkcol(0, 0xFFAAAAFF, 0xFFAAAAFF, 0xBB5D5DFF);
	/* Aqua */
	mkcol(1, DPalebluegreen, DPalegreygreen, DPurpleblue);
	/* Yellow */
	mkcol(2, DPaleyellow, DDarkyellow, DYellowgreen);
	/* Green */
	mkcol(3, DPalegreen, DMedgreen, DDarkgreen);
	/* Blue */
	mkcol(4, 0x00AAFFFF, 0x00AAFFFF, 0x0088CCFF);
	/* Grey */
	cols[5][0] = allocimage(display, Rect(0,0,1,1), screen->chan, 1, 0xEEEEEEFF);
	cols[5][1] = allocimage(display, Rect(0,0,1,1), screen->chan, 1, 0xCCCCCCFF);
	cols[5][2] = allocimage(display, Rect(0,0,1,1), screen->chan, 1, 0x888888FF);
	grey = cols[5][2];
	red = allocimage(display, Rect(0,0,1,1), screen->chan, 1, 0xFF0000FF);
	green = allocimage(display, Rect(0,0,1,1), screen->chan, 1, 0x00FF00FF);
	bg = display->white;
	fg = display->black;
}

static void
redraw(int timefd, float scale)
{
	int n, i, x;
	char buf[256];
	Time ts, now, period, ppp, oldestts;
	Point p, q;
	Rectangle r, rtime;
	Task *t;

#	define time2x(t)	((int)(((t) - oldestts) / ppp))

	if ((n = read(timefd, &now, sizeof(Time))) < 0)
		sysfatal("redraw: Cannot get time: %r\n");
	assert(n == sizeof(Time));

	period = (Time)(Onesecond * scale);
	ppp = (Time)(Onesecond * scale) / Width;	// period per pixel.
	oldestts = now - period;

	/* white out time */
	rtime = screen->r;
	rtime.min.x = rtime.max.x - stringwidth(mediumfont, "0000000.000s");
	rtime.max.y = rtime.min.y + mediumfont->height;
	draw(screen, rtime, bg, nil, ZP);

	if (prevts < oldestts){
		prevts = oldestts;
		draw(screen, screen->r, bg, nil, ZP);
		p = screen->r.min;
		for (n = 0; n != ntasks; n++) {
			t = &tasks[n];
			/* p is upper left corner for this task */
			snprint(buf, sizeof(buf), "%ld ", t->tid);
			q = string(screen, p, fg, ZP, mediumfont, buf);
			snprint(buf, sizeof(buf), " %s", t->settings);
			string(screen, q, fg, ZP, tinyfont, buf);
			p.y += Height;
		}
	}
	
	x = time2x(prevts);

	p = screen->r.min;
	for (n = 0; n != ntasks; n++) {
		t = &tasks[n];

		/* p is upper left corner for this task */

		/* Move part already drawn */
		r = Rect(p.x, p.y + topmargin, p.x + x, p.y+Height);
		draw(screen, r, screen, nil, Pt(p.x + Width - x, p.y + topmargin));

		r.max.x = screen->r.max.x;
		r.min.x += x;
		draw(screen, r, bg, nil, ZP);

		line(screen, addpt(p, Pt(x, Height - lineht)), Pt(screen->r.max.x, p.y + Height - lineht),
			Endsquare, Endsquare, 0, cols[n % Ncolor][1], ZP);

		for (i = 0; i < t->nevents-1; i++)
			if (prevts < t->events[i + 1].ts)
				break;
			
		if (i > 0) {
			memmove(t->events, t->events + i,
				  (t->nevents - i) * sizeof(Schedevent));
			t->nevents -= i;
		}

		for (i = 0; i != t->nevents; i++) {
			Schedevent *e = &t->events[i];
			int sx, ex, j;

			switch (e->etype) {
			case SRelease:

				if (e->ts > prevts && e->ts <= now) {
					sx = time2x(e->ts);
					line(screen, addpt(p, Pt(sx, topmargin)), addpt(p, Pt(sx, Height - bottommargin)), 
						Endarrow, Endsquare, 1, fg, ZP);
				}
				break;
			case SDeadline:
				if (e->ts > prevts && e->ts <= now) {
					sx = time2x(e->ts);
					line(screen, addpt(p, Pt(sx, topmargin)), addpt(p, Pt(sx, Height - bottommargin)), 
						Endsquare, Endarrow, 1, fg, ZP);
				}
				break;

			case SYield:
				if (e->ts > prevts && e->ts <= now) {
					sx = time2x(e->ts);
					line(screen, addpt(p, Pt(sx, topmargin)), addpt(p, Pt(sx, Height - bottommargin)), 
						Endsquare, Endarrow, 0, green, ZP);
				}
				break;

			case SSlice:
				if (e->ts > prevts && e->ts <= now) {
					sx = time2x(e->ts);
					line(screen, addpt(p, Pt(sx, topmargin)), addpt(p, Pt(sx, Height - bottommargin)), 
						Endsquare, Endarrow, 0, red, ZP);
				}
				break;

			case SBlock:
			case SExpel:
				break;

			case SPreempt:
				for (j = i + 1; j < t->nevents; j++) {
					if (t->events[j].etype == SRun)
						break;
				}
				if (j >= t->nevents)
					break;
				if (t->events[j].ts < prevts)
					break;

				sx = time2x(e->ts);
				ex = time2x(t->events[j].ts);

				r = Rect(sx, Height - lineht - 8, ex, Height - lineht);
				r = rectaddpt(r, p);

				draw(screen, r, cols[n % Ncolor][1], nil, ZP);
				break;

			case SRun:
			case SResume:

				for (j = i + 1; j < t->nevents; j++) {
					if (t->events[j].etype == SBlock
					  || t->events[j].etype == SPreempt
					  || t->events[j].etype == SYield
					  || t->events[j].etype == SSlice)
						break;
				}

				if (j >= t->nevents)
					break;
				if (t->events[j].ts < prevts)
					break;

				sx = time2x(e->ts);
				ex = time2x(t->events[j].ts);

				r = Rect(sx, topmargin + 8, ex, Height - lineht);
				r = rectaddpt(r, p);

				draw(screen, r, cols[n % Ncolor][1], nil, ZP);
				break;
			}
		}
		p.y += Height;
	}

	ts = roundup(prevts, period / 10);
	x = time2x(ts);

	while (x < Dx(screen->r)) {

		p = screen->r.min;
		for (n = 0; n < ntasks; n++) {
			int height, width;

			/* p is upper left corner for this task */

			if ((ts % (period / 2)) == 0) {
				height = Height / 2;
				width = 1;
			}
			else {
				height = 3 * Height / 4;
				width = 0;
			}

			line(screen, addpt(p, Pt(x, height)), addpt(p, Pt(x, Height - lineht)),
				Endsquare, Endsquare, width, cols[n % Ncolor][2], ZP);

			p.y += Height;
		}

		if ((ts % (period / 2)) == 0) {
			snprint(buf, sizeof(buf), "%T", ts);
			string(screen, addpt(p, Pt(x - stringwidth(tinyfont, buf)/2, - tinyfont->height - 1)), 
				fg, ZP, tinyfont, buf);
		}

		ts += period / 10;
		x += Width / 10;
	}

	snprint(buf, sizeof(buf), "%T ", now);
	string(screen, Pt(screen->r.max.x - stringwidth(mediumfont, buf), screen->r.min.y), fg, ZP, mediumfont, buf);
	
	flushimage(display, 1);
	prevts = now;
}

void
drawrt(void)
{
	char *wsys, line[256];
	int wfd, wctlfd, timefd, logfd, fd;
	Mousectl *mousectl;
	Keyboardctl *keyboardctl;
	int paused;
	float scale;
	Rune r;
	int i, m, n;
	Task *t;

	event = malloc(sizeof(Schedevent));
	assert(event);

	if ((logfd = open(profdev, OREAD)) < 0)
		sysfatal("%s: Cannot open %s: %r\n", argv0, profdev);

	if ((timefd = open(timedev, OREAD)) < 0)
		sysfatal("%s: Cannot open %s: %r\n", argv0, timedev);

	if (newwin){
		if ((wsys = getenv("wsys")) == nil)
			sysfatal("drawrt: Cannot find windowing system: %r\n");
	
		if ((wfd = open(wsys, ORDWR)) < 0)
			sysfatal("drawrt: Cannot open windowing system: %r\n");
	
		snprint(line, sizeof(line), "new -pid %d -dx %d -dy %d", getpid(), Width + 20, Height + 5);
		line[sizeof(line) - 1] = '\0';
		rfork(RFNAMEG);
	
		if (mount(wfd, -1, "/mnt/wsys", MREPL, line) < 0) 
			sysfatal("drawrt: Cannot mount %s under /mnt/wsys: %r\n", line);
	
		if (bind("/mnt/wsys", "/dev", MBEFORE) < 0) 
			sysfatal("drawrt: Cannot bind /mnt/wsys in /dev: %r\n");
	
	}
	if ((wctlfd = open("/dev/wctl", OWRITE)) < 0)
		sysfatal("drawrt: Cannot open /dev/wctl: %r\n");
	if (initdraw(nil, nil, "rtprof") < 0)
		sysfatal("drawrt: initdraw failure: %r\n");

	Width = Dx(screen->r);
	Height = Dy(screen->r);

	if ((mousectl = initmouse(nil, screen)) == nil)
		sysfatal("drawrt: cannot initialize mouse: %r\n");

	if ((keyboardctl = initkeyboard(nil)) == nil)
		sysfatal("drawrt: cannot initialize keyboard: %r\n");

	colinit();

	SET(schedfd);
	if (!besteffort){
		schedfd = schedparse(T, Dl, C);
 		atnotify(rthandler, 1);
	}

	paused = 0;
	scale = 1.;
	while (1) {
		Alt a[] = {
			{ mousectl->c,			nil,		CHANRCV		},
			{ mousectl->resizec,	nil,		CHANRCV		},
			{ keyboardctl->c,		&r,		CHANRCV		},
			{ nil,					nil,		CHANNOBLK	},
		};

		switch (alt(a)) {
		case 0:
			continue;

		case 1:
			if (getwindow(display, Refnone) < 0)
				sysfatal("drawrt: Cannot re-attach window\n");
			if (newwin){
				if (Dx(screen->r) != Width || Dy(screen->r) != (ntasks * Height)){
					fprint(2, "resize: x: have %d, need %d; y: have %d, need %d\n",
						Dx(screen->r), Width + 8, Dy(screen->r), (ntasks * Height) + 8);
					fprint(wctlfd, "resize -dx %d -dy %d\n", Width + 8, (ntasks * Height) + 8);
				}
			}else{
				Width = Dx(screen->r);
				Height = Dy(screen->r)/ntasks;
			}
			break;

		case 2:

			switch (r) {
			case 'p':
				if (!paused)
					paused = 1;
				else
					paused = 0;
				prevts = 0;
				break;

			case '+':
				scale /= 2.;
				prevts = 0;
				break;

			case '-':
				scale *= 2.;
				prevts = 0;
				break;

			case 'q':
				rthandler(nil, nil);
				threadexitsall(nil);

			default:
				break;
			}
			break;
			
		case 3:
			while((n = read(logfd, event, sizeof(Schedevent))) > 0){

				assert(n == sizeof(Schedevent));

				if (verbose)
					print("%ud %T %s\n", event->tid, event->ts, schedstatename[event->etype]);
				for (i = 0; i < ntasks; i++)
					if (tasks[i].tid == event->tid)
						break;

				if (i == ntasks) {
					tasks = realloc(tasks, (ntasks + 1) * sizeof(Task));
					assert(tasks);

					t = &tasks[ntasks++];
					t->nevents = 0;
					t->events = nil;
					t->tid = event->tid;
					snprint(line, sizeof line, "%s/%ud", taskdir, event->tid);
					if ((fd = open(line, OREAD)) < 0)
						sysfatal("%s: %r", line);
					m = read(fd, line, sizeof line - 1);
					close(fd);
					if (m < 0)
						sysfatal("%s/%ud: %r", taskdir, event->tid);
					line[m] = 0;
					if (m && line[--m] == '\n')
						line[m] = 0;
					t->settings = strdup(line);
					prevts = 0;
					if (newwin){
						fprint(wctlfd, "resize -dx %d -dy %d\n",
							Width + 20, (ntasks * Height) + 5);
					}else
						Height = Dy(screen->r)/ntasks;
				} else
					t = &tasks[i];
				
				if (event->etype == SExpel){
					free(t->events);
					free(t->settings);
					ntasks--;
					memmove(t, t+1, sizeof(Task)*(ntasks-i));
					if (newwin)
						fprint(wctlfd, "resize -dx %d -dy %d\n",
							Width + 20, (ntasks * Height) + 5);
					else
						Height = Dy(screen->r)/ntasks;
					prevts = 0;
				}else{
					t->events = realloc(t->events, (t->nevents+1)*sizeof(Schedevent));
					assert(t->events);
					memcpy(&t->events[t->nevents], event, sizeof(Schedevent));
					t->nevents++;
				}
			}
			if (!paused)
				redraw(timefd, scale);
		}
		if (besteffort)
			sleep(100);
		else if (fprint(schedfd, "yield") < 0)
			sysfatal("yield: %r");
	}
}

int
schedparse(char *period, char *deadline, char *slice)
{
	int schedfd;

	if ((schedfd = open(clonedev, ORDWR)) < 0) 
		sysfatal("%s: %r", clonedev);
	
	fprint(2, "T=%s D=%s C=%s procs=%d admit\n", period, deadline, slice, getpid());
	if (fprint(schedfd, "T=%s D=%s C=%s procs=%d admit", period, deadline, slice, getpid()) < 0)
		sysfatal("%s: %r", clonedev);

	return schedfd;
}
