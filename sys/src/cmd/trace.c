#include <u.h>
#include <tos.h>
#include <libc.h>
#include <thread.h>
#include <ip.h>
#include <bio.h>
#include <draw.h>
#include <mouse.h>
#include <cursor.h>
#include <keyboard.h>
#include "trace.h"

#pragma	varargck	type	"t"		vlong
#pragma	varargck	type	"U"		uvlong

#define NS(x)	((vlong)x)
#define US(x)	(NS(x) * 1000ULL)
#define MS(x)	(US(x) * 1000ULL)
#define S(x)	(MS(x) * 1000ULL)

#define numblocks(a, b)	(((a) + (b) - 1) / (b))
#define roundup(a, b)	(numblocks((a), (b)) * (b))

enum {
	OneRound = MS(1)/2LL,
	MilliRound = US(1)/2LL,
};

typedef struct Event	Event;
typedef struct Task	Task;
struct Event {
	Traceevent;
	vlong	etime;	/* length of block to draw */
};

struct Task {
	int	pid;
	char	*name;
	int	nevents;	
	Event	*events;
	vlong	tstart;
	vlong	total;
	vlong	runtime;
	vlong	runmax;
	vlong	runthis;
	long	runs;
	ulong	tevents[Nevent];
};

enum {
	Nevents = 1024,
	Ncolor = 6,
	K = 1024,
};

vlong	now, prevts;

int	newwin;
int	Width = 1000;		
int	Height = 100;		// Per task
int	topmargin = 8;
int	bottommargin = 4;
int	lineht = 12;
int	wctlfd;
int	nevents;
Traceevent *eventbuf;
Event	*event;

void drawtrace(void);
int schedparse(char*, char*, char*);
int timeconv(Fmt*);

char *schedstatename[] = {
	[SAdmit] =	"Admit",
	[SSleep] =	"Sleep",
	[SDead] =	"Dead",
	[SDeadline] =	"Deadline",
	[SEdf] =	"Edf",
	[SExpel] =	"Expel",
	[SReady] =	"Ready",
	[SRelease] =	"Release",
	[SRun] =	"Run",
	[SSlice] =	"Slice",
	[SInts] =	"Ints",
	[SInte] =	"Inte",
	[SUser] = 	"User",
	[SYield] =	"Yield",
};

struct {
	vlong	scale;
	vlong	bigtics;
	vlong	littletics;
	int	sleep;
} scales[] = {
	{	US(500),	US(100),	US(50),		  0},
	{	US(1000),	US(500),	US(100),	  0},
	{	US(2000),	US(1000),	US(200),	  0},
	{	US(5000),	US(1000),	US(500),	  0},
	{	MS(10),		MS(5),		MS(1),		 20},
	{	MS(20),		MS(10),		MS(2),		 20},
	{	MS(50),		MS(10),		MS(5),		 20},
	{	MS(100),	MS(50),		MS(10),		 20},	/* starting scaleno */
	{	MS(200),	MS(100),	MS(20),		 20},
	{	MS(500),	MS(100),	MS(50),		 50},
	{	MS(1000),	MS(500),	MS(100),	100},
	{	MS(2000),	MS(1000),	MS(200),	100},
	{	MS(5000),	MS(1000),	MS(500),	100},
	{	S(10),		S(50),		S(1),		100},
	{	S(20),		S(10),		S(2),		100},
	{	S(50),		S(10),		S(5),		100},
	{	S(100),		S(50),		S(10),		100},
	{	S(200),		S(100),		S(20),		100},
	{	S(500),		S(100),		S(50),		100},
	{	S(1000),	S(500),		S(100),		100},
};

int ntasks, verbose, triggerproc, paused;
Task *tasks;
Image *cols[Ncolor][4];
Font *mediumfont, *tinyfont;
Image *grey, *red, *green, *blue, *bg, *fg;
char*profdev = "/proc/trace";

static void
usage(void)
{
	fprint(2, "Usage: %s [-d profdev] [-w] [-v] [-t triggerproc] [processes]\n", argv0);
	exits(nil);
}

void
threadmain(int argc, char **argv)
{
	int fd, i;
	char fname[80];

	fmtinstall('t', timeconv);
	ARGBEGIN {
	case 'd':
		profdev = EARGF(usage());
		break;
	case 'v':
		verbose = 1;
		break;
	case 'w':
		newwin++;
		break;
	case 't':
		triggerproc = (int)strtol(EARGF(usage()), nil, 0);
		break;
	default:
		usage();
	}
	ARGEND;

	fname[sizeof fname - 1] = 0;
	for(i = 0; i < argc; i++){
		snprint(fname, sizeof fname - 2, "/proc/%s/ctl", 
					argv[i]);
		if((fd = open(fname, OWRITE)) < 0){
			fprint(2, "%s: cannot open %s: %r\n",
						argv[0], fname);
			continue;
		}

		if(fprint(fd, "trace 1") < 0)
			fprint(2, "%s: cannot enable tracing on %s: %r\n",
						argv[0], fname);
		close(fd);
	}

	drawtrace();
}

static void
mkcol(int i, int c0, int c1, int c2)
{
	cols[i][0] = allocimagemix(display, c0, DWhite);
	cols[i][1] = allocimage(display, Rect(0,0,1,1), screen->chan, 1, c1);
	cols[i][2] = allocimage(display, Rect(0,0,1,1), screen->chan, 1, c2);
	cols[i][3] = allocimage(display, Rect(0,0,1,1), screen->chan, 1, c0);
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
	cols[5][3] = allocimage(display, Rect(0,0,1,1), screen->chan, 1, 0xAAAAAAFF);
	grey = cols[5][2];
	red = allocimage(display, Rect(0,0,1,1), screen->chan, 1, 0xFF0000FF);
	green = allocimage(display, Rect(0,0,1,1), screen->chan, 1, 0x00FF00FF);
	blue = allocimage(display, Rect(0,0,1,1), screen->chan, 1, 0x0000FFFF);
	bg = display->white;
	fg = display->black;
}

static void
redraw(int scaleno)
{
	int n, i, j, x;
	char buf[256];
	Point p, q;
	Rectangle r, rtime;
	Task *t;
	vlong ts, oldestts, newestts, period, ppp, scale, s, ss;

#	define time2x(t)	((int)(((t) - oldestts) / ppp))

	scale = scales[scaleno].scale;
	period = scale + scales[scaleno].littletics;
	ppp = period / Width;	// period per pixel.

	/* Round `now' to a nice number */
	newestts = now - (now % scales[scaleno].bigtics) + 
			(scales[scaleno].littletics>>1);

	oldestts = newestts - period;

//print("newestts %t, period %t, %d-%d\n", newestts, period, time2x(oldestts), time2x(newestts));
	if (prevts < oldestts){
		oldestts = newestts - period;

		prevts = oldestts;
		draw(screen, screen->r, bg, nil, ZP);
	}else{
		/* just white out time */
		rtime = screen->r;
		rtime.min.x = rtime.max.x - stringwidth(mediumfont, "00000000000.000s");
		rtime.max.y = rtime.min.y + mediumfont->height;
		draw(screen, rtime, bg, nil, ZP);
	}
	p = screen->r.min;
	for (n = 0; n != ntasks; n++) {
		t = &tasks[n];
		/* p is upper left corner for this task */
		rtime = Rpt(p, addpt(p, Pt(500, mediumfont->height)));
		draw(screen, rtime, bg, nil, ZP);
		snprint(buf, sizeof(buf), "%d %s", t->pid, t->name);
		q = string(screen, p, fg, ZP, mediumfont, buf);
		s = now - t->tstart;
		if(t->tevents[SRelease])
			snprint(buf, sizeof(buf), " per %t — avg: %t max: %t",
				(vlong)(s/t->tevents[SRelease]),
				(vlong)(t->runtime/t->tevents[SRelease]),
				t->runmax);
		else if((s /=1000000000LL) != 0)
			snprint(buf, sizeof(buf), " per 1s — avg: %t total: %t",
				t->total/s,
				t->total);
		else
			snprint(buf, sizeof(buf), " total: %t", t->total);
		string(screen, q, fg, ZP, tinyfont, buf);
		p.y += Height;
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
			if (prevts < t->events[i + 1].time)
				break;
			
		if (i > 0) {
			memmove(t->events, t->events + i, (t->nevents - i) * sizeof(Event));
			t->nevents -= i;
		}

		for (i = 0; i != t->nevents; i++) {
			Event *e = &t->events[i], *_e;
			int sx, ex;

			switch (e->etype & 0xffff) {
			case SAdmit:
				if (e->time > prevts && e->time <= newestts) {
					sx = time2x(e->time);
					line(screen, addpt(p, Pt(sx, topmargin)), 
						addpt(p, Pt(sx, Height - bottommargin)), 
						Endarrow, Endsquare, 1, green, ZP);
				}
				break;
			case SExpel:
				if (e->time > prevts && e->time <= newestts) {
					sx = time2x(e->time);
					line(screen, addpt(p, Pt(sx, topmargin)), 
						addpt(p, Pt(sx, Height - bottommargin)), 
						Endsquare, Endarrow, 1, red, ZP);
				}
				break;
			case SRelease:
				if (e->time > prevts && e->time <= newestts) {
					sx = time2x(e->time);
					line(screen, addpt(p, Pt(sx, topmargin)), 
						addpt(p, Pt(sx, Height - bottommargin)), 
						Endarrow, Endsquare, 1, fg, ZP);
				}
				break;
			case SDeadline:
				if (e->time > prevts && e->time <= newestts) {
					sx = time2x(e->time);
					line(screen, addpt(p, Pt(sx, topmargin)), 
						addpt(p, Pt(sx, Height - bottommargin)), 
						Endsquare, Endarrow, 1, fg, ZP);
				}
				break;

			case SYield:
			case SUser:
				if (e->time > prevts && e->time <= newestts) {
					sx = time2x(e->time);
					line(screen, addpt(p, Pt(sx, topmargin)), 
						addpt(p, Pt(sx, Height - bottommargin)), 
						Endsquare, Endarrow, 0, 
						(e->etype == SYield)? green: blue, ZP);
				}
				break;
			case SSlice:
				if (e->time > prevts && e->time <= newestts) {
					sx = time2x(e->time);
					line(screen, addpt(p, Pt(sx, topmargin)), 
						addpt(p, Pt(sx, Height - bottommargin)), 
						Endsquare, Endarrow, 0, red, ZP);
				}
				break;

			case SRun:
			case SEdf:
				sx = time2x(e->time);
				ex = time2x(e->etime);
				if(ex == sx)
					ex++;

				r = Rect(sx, topmargin + 8, ex, Height - lineht);
				r = rectaddpt(r, p);

				draw(screen, r, cols[n % Ncolor][e->etype==SRun?1:3], nil, ZP);

				if(t->pid == triggerproc && ex < Width)
					paused ^= 1;

				for(j = 0; j < t->nevents; j++){
					_e = &t->events[j];
					switch(_e->etype & 0xffff){
					case SInts:
						if (_e->time > prevts && _e->time <= newestts){
							sx = time2x(_e->time);
							line(screen, addpt(p, Pt(sx, topmargin)), 
												addpt(p, Pt(sx, Height / 2 - bottommargin)), 	
												Endsquare, Endsquare, 0, 
												green, ZP);
						}
						break;
					case SInte:
						if (_e->time > prevts && _e->time <= newestts) {
							sx = time2x(_e->time);
							line(screen, addpt(p, Pt(sx, Height / 2 - bottommargin)), 
												addpt(p, Pt(sx, Height - bottommargin)), 
												Endsquare, Endsquare, 0, 
												blue, ZP);
						}
						break;
					}
				}
				break;
			}
		}
		p.y += Height;
	}

	ts = prevts + scales[scaleno].littletics - (prevts % scales[scaleno].littletics);
	x = time2x(ts);

	while(x < Width){
		p = screen->r.min;
		for(n = 0; n < ntasks; n++){
			int height, width;

			/* p is upper left corner for this task */
			if ((ts % scales[scaleno].scale) == 0){
				height = 10 * Height;
				width = 1;
			}else if ((ts % scales[scaleno].bigtics) == 0){
				height = 12 * Height;
				width = 0;
			}else{
				height = 13 * Height;
				width = 0;
			}
			height >>= 4;

			line(screen, addpt(p, Pt(x, height)), addpt(p, Pt(x, Height - lineht)),
				Endsquare, Endsquare, width, cols[n % Ncolor][2], ZP);

			p.y += Height;
		}
		ts += scales[scaleno].littletics;
		x = time2x(ts);
	}

	rtime = screen->r;
	rtime.min.y = rtime.max.y - tinyfont->height + 2;
	draw(screen, rtime, bg, nil, ZP);
	ts = oldestts + scales[scaleno].bigtics - (oldestts % scales[scaleno].bigtics);
	x = time2x(ts);
	ss = 0;
	while(x < Width){
		snprint(buf, sizeof(buf), "%t", ss);
		string(screen, addpt(p, Pt(x - stringwidth(tinyfont, buf)/2, - tinyfont->height - 1)), 
			fg, ZP, tinyfont, buf);
		ts += scales[scaleno].bigtics;
		ss += scales[scaleno].bigtics;
		x = time2x(ts);
	}

	snprint(buf, sizeof(buf), "%t", now);
	string(screen, Pt(screen->r.max.x - stringwidth(mediumfont, buf), screen->r.min.y), 
		fg, ZP, mediumfont, buf);
	
	flushimage(display, 1);
	prevts = newestts;
}

Task*
newtask(ulong pid)
{
	Task *t;
	char buf[64], *p;
	int fd,n;

	tasks = realloc(tasks, (ntasks + 1) * sizeof(Task));
	assert(tasks);

	t = &tasks[ntasks++];
	memset(t, 0, sizeof(Task));
	t->events = nil;
	snprint(buf, sizeof buf, "/proc/%ld/status", pid);
	t->name = nil;
	fd = open(buf, OREAD);
	if (fd >= 0){
		n = read(fd, buf, sizeof buf);
		if(n > 0){
			p = buf + sizeof buf - 1;
			*p = 0;
			p = strchr(buf, ' ');
			if (p) *p = 0;
			t->name = strdup(buf);
		}else
			print("%s: %r\n", buf);
		close(fd);
	}else
		print("%s: %r\n", buf);
	t->pid = pid;
	prevts = 0;
	if (newwin){
		fprint(wctlfd, "resize -dx %d -dy %d\n",
			Width + 20, (ntasks * Height) + 5);
	}else
		Height = ntasks ? Dy(screen->r)/ntasks : Dy(screen->r);
	return t;
}

void
doevent(Task *t, Traceevent *ep)
{
	int i, n;
	Event *event;
	vlong runt;

	t->tevents[ep->etype & 0xffff]++;
	n = t->nevents++;
	t->events = realloc(t->events, t->nevents*sizeof(Event));
	assert(t->events);
	event = &t->events[n];
	memmove(event, ep, sizeof(Traceevent));
	event->etime = 0;

	switch(event->etype & 0xffff){
	case SRelease:
		if (t->runthis > t->runmax)
			t->runmax = t->runthis;
		t->runthis = 0;
		break;

	case SSleep:
	case SYield:
	case SReady:
	case SSlice:
		for(i = n-1; i >= 0; i--)
			if (t->events[i].etype == SRun || 
				t->events[i].etype == SEdf)
				break;
		if(i < 0 || t->events[i].etime != 0)
			break;
		runt = event->time - t->events[i].time;
		if(runt > 0){
			t->events[i].etime = event->time;
			t->runtime += runt;
			t->total += runt;
			t->runthis += runt;
			t->runs++;
		}
		break;
	case SDead:
print("task died %ld %t %s\n", event->pid, event->time, schedstatename[event->etype & 0xffff]);
		free(t->events);
		free(t->name);
		ntasks--;
		memmove(t, t+1, sizeof(Task)*(&tasks[ntasks]-t));
		if (newwin)
			fprint(wctlfd, "resize -dx %d -dy %d\n",
				Width + 20, (ntasks * Height) + 5);
		else
			Height = ntasks ? Dy(screen->r)/ntasks : Dy(screen->r);
		prevts = 0;
	}
}

void
drawtrace(void)
{
	char *wsys, line[256];
	int wfd, logfd;
	Mousectl *mousectl;
	Keyboardctl *keyboardctl;
	int scaleno;
	Rune r;
	int i, n;
	Task *t;
	Traceevent *ep;

	eventbuf = malloc(Nevents*sizeof(Traceevent));
	assert(eventbuf);

	if((logfd = open(profdev, OREAD)) < 0)
		sysfatal("%s: Cannot open %s: %r", argv0, profdev);

	if(newwin){
		if((wsys = getenv("wsys")) == nil)
			sysfatal("%s: Cannot find windowing system: %r",
						argv0);
	
		if((wfd = open(wsys, ORDWR)) < 0)
			sysfatal("%s: Cannot open windowing system: %r",
						argv0);
	
		snprint(line, sizeof(line), "new -pid %d -dx %d -dy %d",
				getpid(), Width + 20, Height + 5);
		line[sizeof(line) - 1] = '\0';
		rfork(RFNAMEG);
	
		if(mount(wfd, -1, "/mnt/wsys", MREPL, line) < 0) 
			sysfatal("%s: Cannot mount %s under /mnt/wsys: %r",
						argv0, line);
	
		if(bind("/mnt/wsys", "/dev", MBEFORE) < 0) 
			sysfatal("%s: Cannot bind /mnt/wsys in /dev: %r",
						argv0);
	
	}
	if((wctlfd = open("/dev/wctl", OWRITE)) < 0)
		sysfatal("%s: Cannot open /dev/wctl: %r", argv0);
	if(initdraw(nil, nil, "trace") < 0)
		sysfatal("%s: initdraw failure: %r", argv0);

	Width = Dx(screen->r);
	Height = Dy(screen->r);

	if((mousectl = initmouse(nil, screen)) == nil)
		sysfatal("%s: cannot initialize mouse: %r", argv0);

	if((keyboardctl = initkeyboard(nil)) == nil)
		sysfatal("%s: cannot initialize keyboard: %r", argv0);

	colinit();

	paused = 0;
	scaleno = 7;	/* 100 milliseconds */
	now = nsec();
	for(;;) {
		Alt a[] = {
			{ mousectl->c,			nil,		CHANRCV		},
			{ mousectl->resizec,	nil,		CHANRCV		},
			{ keyboardctl->c,		&r,			CHANRCV		},
			{ nil,					nil,		CHANNOBLK	},
		};

		switch (alt(a)) {
		case 0:
			continue;

		case 1:
			if(getwindow(display, Refnone) < 0)
				sysfatal("drawrt: Cannot re-attach window");
			if(newwin){
				if(Dx(screen->r) != Width || 
					Dy(screen->r) != (ntasks * Height)){
					fprint(2, "resize: x: have %d, need %d; y: have %d, need %d\n",
							Dx(screen->r), Width + 8, Dy(screen->r), (ntasks * Height) + 8);
					fprint(wctlfd, "resize -dx %d -dy %d\n", 
							Width + 8, (ntasks * Height) + 8);
				}
			}
			else{
				Width = Dx(screen->r);
				Height = ntasks? Dy(screen->r)/ntasks: 
							Dy(screen->r);
			}
			break;

		case 2:

			switch(r){
			case 'r':
				for(i = 0; i < ntasks; i++){
					tasks[i].tstart = now;
					tasks[i].total = 0;
					tasks[i].runtime = 0;
					tasks[i].runmax = 0;
					tasks[i].runthis = 0;
					tasks[i].runs = 0;
					memset(tasks[i].tevents, 0, Nevent*sizeof(ulong));
					
				}
				break;

			case 'p':
				paused ^= 1;
				prevts = 0;
				break;

			case '-':
				if (scaleno < nelem(scales) - 1)
					scaleno++;
				prevts = 0;
				break;

			case '+':
				if (scaleno > 0)
					scaleno--;
				prevts = 0;
				break;

			case 'q':
				threadexitsall(nil);

			case 'v':
				verbose ^= 1;

			default:
				break;
			}
			break;
			
		case 3:
			now = nsec();
			while((n = read(logfd, eventbuf, Nevents*sizeof(Traceevent))) > 0){
				assert((n % sizeof(Traceevent)) == 0);
				nevents = n / sizeof(Traceevent);
				for (ep = eventbuf; ep < eventbuf + nevents; ep++){
					if ((ep->etype & 0xffff) >= Nevent){
						print("%ld %t Illegal event %ld\n",
							ep->pid, ep->time, ep->etype & 0xffff);
						continue;
					}
					if (verbose)
						print("%ld %t %s\n",
							ep->pid, ep->time, schedstatename[ep->etype & 0xffff]);

					for(i = 0; i < ntasks; i++)
						if(tasks[i].pid == ep->pid)
							break;

					if(i == ntasks){
						t = newtask(ep->pid);
						t->tstart = ep->time;
					}else
						t = &tasks[i];

					doevent(t, ep);
				}
			}
			if(!paused)
				redraw(scaleno);
		}
		sleep(scales[scaleno].sleep);
	}
}

int
timeconv(Fmt *f)
{
	char buf[128], *sign;
	vlong t;

	buf[0] = 0;
	switch(f->r) {
	case 'U':
		t = va_arg(f->args, vlong);
		break;
	case 't':		// vlong in nanoseconds
		t = va_arg(f->args, vlong);
		break;
	default:
		return fmtstrcpy(f, "(timeconv)");
	}
	if (t < 0) {
		sign = "-";
		t = -t;
	}else
		sign = "";
	if (t > S(1)){
		t += OneRound;
		sprint(buf, "%s%d.%.3ds", sign, (int)(t / S(1)), (int)(t % S(1))/1000000);
	}else if (t > MS(1)){
		t += MilliRound;
		sprint(buf, "%s%d.%.3dms", sign, (int)(t / MS(1)), (int)(t % MS(1))/1000);
	}else if (t > US(1))
		sprint(buf, "%s%d.%.3dµs", sign, (int)(t / US(1)), (int)(t % US(1)));
	else
		sprint(buf, "%s%dns", sign, (int)t);
	return fmtstrcpy(f, buf);
}
