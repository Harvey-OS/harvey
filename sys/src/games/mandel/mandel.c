#include <u.h>
#include <libc.h>
#include <libg.h>
#include "m.h"

#define CHUNK	10
#define MAXPROC	20
#define MAX	30
#define NPROC	1
#define MAXJOBS	30

typedef double	FLOAT;

#define BUTTON(n)	((mouse.buttons >> (n-1)) & 1)
#define BUTTON123()	((int)(mouse.buttons & 0x7))

#define rectf(b,r,f)	bitblt(b,r.min,b,r,f)	/* stub - remove rectf */

typedef struct jobinfo {
	FLOAT		xmin, ymin;	/* upper left hand corner */
	FLOAT		xmax, ymax;	/* lower right hand corner */
	FLOAT		incr;
	Rectangle	selected;
} jobinfo;

jobinfo jobs[MAXJOBS];
int thisjob=0;
int setup=0;

int linewidth;		/* pixels per line */
int screenlen;		/* lines per display */
int maxloop;		/* pt assumed in the set */
int nproc=NPROC;	/* number of procs to help out */

Rectangle rscreen, rarena;

int mousefd=-1;
int reshaped;
Mouse mouse;

char *menu2text[]={
	"go",
	"stop",
	"out",
	"reset",
	0
};

enum {
	Go,
	Stop,
	Out,
	Reset
};

char *menu3text[]={
	"max *= 1.2",
	"max /= 1.2",
	"nproc++",
	"nproc--",
	"quit",
	0
};

enum {
	Moreloops,
	Fewerloops,
	Moreprocs,
	Fewerprocs,
	Quit,
};

enum {
	RGo,
	RStop,
	ROut,
	RReset,
	RQuit,
	RMoreloops,
	RFewerloops,
	RMoreprocs,
	RFewerprocs,
	RNone,
	RReshaped,
	RSelected,
};

Menu menu2={menu2text};
Menu menu3={menu3text};


void
box(Rectangle r, int f)
{
	r.max=sub(r.max, Pt(1, 1));
	segment(&screen, r.min, Pt(r.min.x, r.max.y), ~0, f);
	segment(&screen, Pt(r.min.x, r.max.y), r.max, ~0, f);
	segment(&screen, r.max, Pt(r.max.x, r.min.y), ~0, f);
	segment(&screen, Pt(r.max.x, r.min.y) ,r.min, ~0, f);
}

void
getmouse(void)
{
	uchar buf[14];
	uchar *up = buf;
	Point newpt;
	bflush();
	if(read(mousefd, buf, sizeof buf)!=sizeof buf || buf[0]!='m')
		perror("getmouse read");
	newpt.x = BGLONG(up+2);
	newpt.y = BGLONG(up+6);
	if (!ptinrect(newpt, rarena))
		return;
	mouse.xy = newpt;
	mouse.buttons = up[1]&7;
	reshaped = (up[1] & 0x80);
}

void
mouseinit(char *fn)
{
	if (mousefd >= 0)
		close(mousefd);
	mousefd=open(fn, 2);
	if(mousefd<0) {
		perror("Open /dev/mouse");
		exits("could not open mouse");
	}
	do
		getmouse();
	while (BUTTON123());
}

void
sweeprect(void) {
	Rectangle select;
	select.min = select.max = mouse.xy;
	if (setup)
		box(jobs[thisjob+1].selected, DxorS);
	box(select, DxorS);
	do {
		getmouse();
		box(select, DxorS);
		select.max = mouse.xy;
		box(select, DxorS);
	} while (BUTTON(1));
	if (eqpt(select.min, select.max))
		select.max = add(select.min, Pt(1,1));
	bflush();
	jobs[thisjob+1].selected = rcanon(select);
	setup = 1;
}

int
do_mouse(void) {
	if (BUTTON(1)) {
		sweeprect();
		return RSelected;
	}
	else if (BUTTON(2))
		switch(mymenuhit(2, &menu2)) {
		case Go:	return RGo;
		case Stop:	return RStop;
		case Out:	return ROut;
		case Reset:	return RReset;
		}
	else if (BUTTON(3))
		switch(mymenuhit(3, &menu3)) {
		case Quit:	return RQuit;
		case Moreloops:
			maxloop = 1.2*maxloop;
			return RNone;
		case Fewerloops:
			maxloop = maxloop/1.2;
			return RNone;
		case Moreprocs:
			nproc++;
			return RNone;
		case Fewerprocs:
			if (nproc > 1)
				nproc--;
			return RNone;
		}
	else if (reshaped)
		return RReshaped;
	return RNone;
}

void
solve_lines (int firstrow, int lastrow, FLOAT startre, FLOAT startim, FLOAT incr,
		Bitmap *bline) {
	int i, j, shift;
	uchar line[1024*2/8];
	uchar *cp;
	for (i=firstrow; i<lastrow; i++) {
		FLOAT cim=startim - i*incr;
		cp=line;  *cp = 0;
		for (j=0, shift=6; j<linewidth; j++) {
			FLOAT cre=j*incr + startre;
			FLOAT re=0.0, im=0.0, t;
			int count=0;
			do {	/* z = z*z + c */
				t = re;
				re = re*re - im*im + cre;
				im = 2.0*t*im + cim;
			} while (re*re < 4.0 && im*im < 4.0 &&
			         count++ < maxloop);
			if (count > maxloop)
				*cp |= 3<<shift;
			else
				*cp |= ((count/5) % 3)<<shift;
			shift -= 2;
			if (shift < 0) {
				*(++cp)=0; shift=6;
			}
		}
		wrbitmap(bline, 0, 1, line);
		bitblt(&screen, add(rarena.min, Pt(0,i)), bline,
			Rect(0,0, linewidth, 1), DxorS);
	}
	bflush();
}

int
solve(int ji) {
	int pipefd[2], beat[2];
	struct list {
		int start, stop;
	} list;
	int nextline=0, finishing=0, nrunning=0;
	int i;
	int reason, r;

	linewidth = (FLOAT)(jobs[ji].xmax -
		jobs[ji].xmin)/jobs[ji].incr;
	screenlen = (FLOAT)(jobs[ji].ymax -
		jobs[ji].ymin)/jobs[ji].incr;
	if (pipe(pipefd) < 0) {
		perror("pipe");
		exits("pipe");
	}
	for (i=0; !finishing && i<nproc; i++) {
		Bitmap *bline = balloc(Rect(0, 0, 1024, 1), 1);

		if (bline == (Bitmap *)0) {
			perror("balloc");
			exits("balloc");
		}
		switch(fork()) {
		case -1:
			perror("fork");
			exits("fork");
		case 0:
			close(pipefd[0]);
			while(read(pipefd[1], &list, sizeof(list)) > 0) {
				solve_lines(list.start, list.stop,
					jobs[ji].xmin, jobs[ji].ymax,
					jobs[ji].incr, bline);
				write(pipefd[1], &i, sizeof(i));
			}
			close(pipefd[1]);
			exits("");
		default:
			nrunning++;
			list.start = nextline;
			nextline = list.stop = nextline + CHUNK;
			if (list.stop > screenlen) {
				list.stop = screenlen;
				finishing = 1;
			}
			write(pipefd[0], &list, sizeof(list));
		}
	}
	if (pipe(beat) < 0) {
		perror("pipe beat");
		exits("pipe beat");
	}
	switch(fork()) {
	case -1:
		perror("fork heartbeat");
		exits("fork heartbeat");
	case 0:
		close(beat[0]);
		do {
			i = -1;
			sleep(50);
			write(pipefd[1], &i, sizeof(i));
			i = read(beat[1], &i, sizeof(i));
		} while (i > 0);
		close(beat[1]);
		exits("");
	}
	close(pipefd[1]);
	close(beat[1]);
	nrunning++;
	reason = RNone;
	while (nrunning > 0) {
		getmouse();
		r = do_mouse();
		if (r == RQuit || r == RReshaped || r == RGo || r == RSelected ||
		    r == RStop || r == ROut || r == RReset) {
			finishing = 1;
			reason = r;
		}
		read(pipefd[0], &i, sizeof(i));
		if (i == -1) {
			if (finishing) {
				nrunning--;
				write(beat[0], &i, 0);
				wait(0);
			} else {
				i = 1;
				write(beat[0], &i, sizeof(i));
			}
			continue;
		}
		if (finishing) {
			write(pipefd[0], &i, 0);
			nrunning--;
			wait(0);
			continue;
		}
		list.start = nextline;
		nextline = list.stop = nextline + CHUNK;
		if (list.stop > screenlen) {
			list.stop = screenlen;
			finishing = 1;
		}
		write(pipefd[0], &list, sizeof(list));
	}
	close(pipefd[0]);
	close(beat[0]);
	return reason;
		
}

void
do_reshape(void) {
	rscreen = bscreenrect(0);
	jobs[thisjob].selected = jobs[thisjob+1].selected =
		rarena = inset(rscreen, 5);
	linewidth = Dx(rarena);
	screenlen = Dy(rarena);
	rectf(&screen, rarena, Zero);
}

void
setdisplay(int ji) {
	FLOAT xincr, yincr;

	xincr = (jobs[ji].xmax - jobs[ji].xmin)/Dx(rarena);
	yincr = (jobs[ji].ymax - jobs[ji].ymin)/Dy(rarena);
	jobs[ji].incr  = xincr > yincr ? xincr : yincr;
}

void
setselected(int ji) {
	int t;
	jobs[ji].selected.max = jobs[ji].selected.min = rarena.min;
	t = (int)(jobs[ji].xmax - jobs[ji].xmin)/jobs[ji].incr;
	jobs[ji].selected.max.x = jobs[ji].selected.max.x + t; 
	t = (int)(jobs[ji].ymax - jobs[ji].ymin)/jobs[ji].incr; 
	jobs[ji].selected.max.y = jobs[ji].selected.max.y + t;
}

void
dumpjob(int ji) {
	print("%G %G %G %G %G %R\n",
		jobs[ji].xmin, jobs[ji].ymin, jobs[ji].xmax,
		jobs[ji].ymax, jobs[ji].incr, jobs[ji].selected);
}

/*
 * compute the relevant co-ordinates for job ji given jobs[ji].selected
 * and the information for jobs[ji-1].
 */
void
setjob(int ji) {

	jobs[ji].xmin = jobs[ji-1].xmin +
		(jobs[ji].selected.min.x - jobs[ji-1].selected.min.x)*
		jobs[ji-1].incr;
	jobs[ji].ymin = jobs[ji-1].ymin +
		(jobs[ji-1].selected.max.y - jobs[ji].selected.max.y)*
		jobs[ji-1].incr;
	jobs[ji].xmax = jobs[ji-1].xmin +
		(jobs[ji].selected.max.x - jobs[ji-1].selected.min.x)*
		jobs[ji-1].incr;
	jobs[ji].ymax = jobs[ji-1].ymin +
		(jobs[ji-1].selected.max.y - jobs[ji].selected.min.y)*
		jobs[ji-1].incr;
	setdisplay(ji);
}

void
showjob(int ji) {
	char buf[100];
	Point start = Pt(rarena.max.x - strwidth(font, "M")*25, rarena.min.y);

#define SHOWSTRING	string(&screen, start = add(start,	\
				Pt(0, font->height+1)),	\
				font, buf, S);

	sprint(buf, "Xmin: %11.9G", jobs[ji].xmin);		SHOWSTRING;
	sprint(buf, "Ymin: %11.9G", jobs[ji].ymin);		SHOWSTRING;
	sprint(buf, "Xmax: %11.9G", jobs[ji].xmax);		SHOWSTRING;
	sprint(buf, "Ymax: %11.9G", jobs[ji].ymax);		SHOWSTRING;
	sprint(buf, "Incr: %11.9G", jobs[ji].incr);		SHOWSTRING;
	sprint(buf, "Max:     %11d", maxloop);			SHOWSTRING;
	sprint(buf, "Nproc:   %11d", nproc);			SHOWSTRING;
}

void
init_job(void) {
	jobs[0].xmin = -2.2;
	jobs[0].ymin = -1.7;
	jobs[0].xmax = 1.2;
	jobs[0].ymax = 1.7;
	jobs[0].selected = rarena;

	setdisplay(0);
	setselected(0);

	jobs[1] = jobs[0];
	thisjob=1;
	setjob(thisjob);
	setup = 0;
	maxloop=MAX;
}

void
main(void) {
	int reason = RGo;

	binit(0, 0, "");
	mouseinit("/dev/mouse");
	do_reshape();
	init_job();

	while (1) {
		showjob(thisjob);
		switch (reason) {
		case RGo:
			rectf(&screen, rarena, Zero);
			if (thisjob+1 < MAXJOBS && setup) {
				thisjob++;
				setselected(thisjob);
				setup = 0;
			}
			showjob(thisjob);
			mouseinit("/dev/nbmouse");
			reason = solve(thisjob);
			mouseinit("/dev/mouse");
			break;
		case RNone:
		case RStop:
			getmouse();
			reason = do_mouse();
			break;
		case ROut:
			if (thisjob > 1) {
				thisjob--;
				setselected(thisjob);
				setup = 0;
				reason = RGo;
			}
			break;
		case RReset:
			init_job();
			setup = 0;
			reason = RGo;
			break;
		case RReshaped:
			do_reshape();
			setjob(thisjob);
			setup = 0;
			reason = RNone;
			break;
		case RSelected:
			setjob(thisjob+1);
			setup = 1;
			reason = RNone;
			break;
		case RQuit:
			exits("");
		};
	}
}
