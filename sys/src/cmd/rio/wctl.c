#include <u.h>
#include <libc.h>
#include <draw.h>
#include <thread.h>
#include <cursor.h>
#include <mouse.h>
#include <keyboard.h>
#include <frame.h>
#include <auth.h>
#include <fcall.h>
#include <plumb.h>
#include "dat.h"
#include "fns.h"
#include <ctype.h>

char	Ebadwr[]		= "bad rectangle in wctl request";
char	Ewalloc[]		= "window allocation failed in wctl request";

enum
{
	New,
	Resize,
	Move,
	Top,
	Bottom,
	Current,
};

static char *cmds[] = {
	[New]	= "new",
	[Resize]	= "resize",
	[Move]	= "move",
	[Top]	= "top",
	[Bottom]	= "bottom",
	[Current]	= "current",
	nil
};

enum
{
	Cd,
	Minx,
	Miny,
	Maxx,
	Maxy,
	Deltax,
	Deltay,
	R,
	PID,
};

static char *params[] = {
	[Cd]	 	= "-cd",
	[Minx]	= "-minx",
	[Miny]	= "-miny",
	[Maxx]	= "-maxx",
	[Maxy]	= "-maxy",
	[Deltax]	= "-dx",
	[Deltay]	= "-dy",
	[R]		= "-r",
	[PID]		= "-pid",
	nil
};

int
goodrect(Rectangle r)
{
	if(!eqrect(canonrect(r), r))
		return 0;
	if(Dx(r)<100 || Dy(r)<3*font->height)
		return 0;
	if(Dx(r) > Dx(screen->r))
		return 0;
	if(Dy(r) > Dy(screen->r))
		return 0;
	return 1;
}

static
int
word(char **sp, char *tab[])
{
	char *s, *t;
	int i;

	s = *sp;
	while(isspace(*s))
		s++;
	t = s;
	while(*s!='\0' && !isspace(*s))
		s++;
	for(i=0; tab[i]!=nil; i++)
		if(strncmp(tab[i], t, strlen(tab[i])) == 0){
			*sp = s;
			return i;
	}
	return -1;
}

int
set(int sign, int neg, int abs, int pos)
{
	if(sign < 0)
		return neg;
	if(sign > 0)
		return pos;
	return abs;
}

Rectangle
newrect(void)
{
	static int i = 0;
	int minx, miny, dx, dy;

	dx = 600;
	dy = 400;
	if(dx > Dx(screen->r))
		dx = Dx(screen->r);
	if(dy > Dy(screen->r))
		dy = Dy(screen->r);
	minx = 32 + 16*i;
	miny = 32 + 16*i;
	i++;
	i %= 10;

	return Rect(minx, miny, minx+dx, miny+dy);
}

void
shift(int *minp, int *maxp, int min, int max)
{
	if(*minp < min){
		*maxp += min-*minp;
		*minp = min;
	}
	if(*maxp > max){
		*minp += max-*maxp;
		*maxp = max;
	}
}

Rectangle
rectonscreen(Rectangle r)
{
	shift(&r.min.x, &r.max.x, screen->r.min.x, screen->r.max.x);
	shift(&r.min.y, &r.max.y, screen->r.min.y, screen->r.max.y);
	return r;
}

int
parsewctl(char **argp, Rectangle r, Rectangle *rp, int *pidp, char **cdp, char *s, char *err)
{
	int cmd, param, xy, sign;
	char *t;

	*cdp = nil;
	cmd = word(&s, cmds);
	if(cmd < 0){
		strcpy(err, "unrecognized wctl command");
		return -1;
	}
	if(cmd == New)
		r = newrect();

	strcpy(err, "missing or bad wctl parameter");
	while((param = word(&s, params)) >= 0){
		if(param == R){
			r.min.x = strtoul(s, &t, 10);
			if(t == s)
				return -1;
			s = t;
			r.min.y = strtoul(s, &t, 10);
			if(t == s)
				return -1;
			s = t;
			r.max.x = strtoul(s, &t, 10);
			if(t == s)
				return -1;
			s = t;
			r.max.y = strtoul(s, &t, 10);
			if(t == s)
				return -1;
			s = t;
			continue;
		}
		while(isspace(*s))
			s++;
		if(param == Cd){
			*cdp = s;
			while(*s && !isspace(*s))
				s++;
			if(*s != '\0')
				*s++ = '\0';
			continue;
		}
		sign = 0;
		if(*s == '-'){
			sign = -1;
			s++;
		}else if(*s == '+'){
			sign = +1;
			s++;
		}
		if(!isdigit(*s))
			return -1;
		xy = strtol(s, &s, 10);
		switch(param){
		case -1:
			strcpy(err, "unrecognized wctl parameter");
			return -1;
		case Minx:
			r.min.x = set(sign, r.min.x-xy, xy, r.min.x+xy);
			break;
		case Miny:
			r.min.y = set(sign, r.min.y-xy, xy, r.min.y+xy);
			break;
		case Maxx:
			r.max.x = set(sign, r.max.x-xy, xy, r.max.x+xy);
			break;
		case Maxy:
			r.max.y = set(sign, r.max.y-xy, xy, r.max.y+xy);
			break;
		case Deltax:
			r.max.x = set(sign, r.max.x-xy, r.min.x+xy, r.max.x+xy);
			break;
		case Deltay:
			r.max.y = set(sign, r.max.y-xy, r.min.y+xy, r.max.y+xy);
			break;
		case PID:
			if(pidp != nil)
				*pidp = xy;
			break;
		}
	}

	*rp = rectonscreen(rectaddpt(r, screen->r.min));

	while(isspace(*s))
		s++;
	if(cmd!=New && *s!='\0'){
		strcpy(err, "extraneous text in wctl message");
		return -1;
	}

	if(argp)
		*argp = s;

	return cmd;
}

int
wctlnew(Rectangle rect, char *arg, char *dir, char *err)
{
	char **argv;
	Image *i;

	if(!goodrect(rect)){
		strcpy(err, Ebadwr);
		return -1;
	}
	argv = emalloc(4*sizeof(char*));
	argv[0] = "rc";
	argv[1] = "-c";
	while(isspace(*arg))
		arg++;
	if(*arg == '\0'){
		argv[1] = "-i";
		argv[2] = nil;
	}else{
		argv[2] = arg;
		argv[3] = nil;
	}
	i = allocwindow(wscreen, rect, Refbackup, DWhite);
	if(i == nil){
		strcpy(err, Ewalloc);
		return -1;
	}
	border(i, rect, Selborder, red, ZP);

	new(i, 0, dir, "/bin/rc", argv);

	free(argv);	/* when new() returns, argv and args have been copied */
	return 1;
}

int
writewctl(Xfid *x, char *err)
{
	int cnt, cmd;
	Image *i;
	char *arg, *dir;
	Rectangle rect;
	Window *w;

	w = x->f->w;
	cnt = x->count;
	x->data[cnt] = '\0';

	rect = rectsubpt(w->screenr, screen->r.min);
	cmd = parsewctl(&arg, rect, &rect, nil, &dir, x->data, err);
	if(cmd < 0)
		return -1;

	switch(cmd){
	case New:
		return wctlnew(rect, arg, dir, err);
	case Move:
		rect = Rect(rect.min.x, rect.min.y, rect.min.x+Dx(w->screenr), rect.min.y+Dy(w->screenr));
		rect = rectonscreen(rect);
		/* fall through */
	case Resize:
		if(!goodrect(rect)){
			strcpy(err, Ebadwr);
			return -1;
		}
		if(eqrect(rect, w->screenr))
			return 1;
		i = allocwindow(wscreen, rect, Refbackup, DWhite);
		if(i == nil){
			strcpy(err, Ewalloc);
			return -1;
		}
		border(i, rect, Selborder, red, ZP);
		wsendctlmesg(w, Reshaped, i->r, i);
		return 1;
	case Top:
		wtopme(w);
		return 1;
	case Bottom:
		wbottomme(w);
		return 1;
	case Current:
		wcurrent(w);
		return 1;
	}
	strcpy(err, "invalid wctl message");
	return -1;
}

void
wctlthread(void *v)
{
	char *buf, *arg, *dir;
	int cmd;
	Rectangle rect;
	char err[ERRLEN];
	Channel *c;

	c = v;

	threadsetname("WCTLTHREAD");

	for(;;){
		buf = recvp(c);
		cmd = parsewctl(&arg, ZR, &rect, nil, &dir, buf, err);

		switch(cmd){
		case New:
			wctlnew(rect, arg, dir, err);
		}
		free(buf);
	}
}

void
wctlproc(void *v)
{
	char *buf;
	int n, eofs;
	Channel *c;

	threadsetname("WCTLPROC");
	c = v;

	eofs = 0;
	for(;;){
		buf = emalloc(MAXFDATA+1);	/* +1 for terminal '\0' */
		n = read(wctlfd, buf, MAXFDATA);
		if(n < 0)
			break;
		if(n == 0){
			if(++eofs > 20)
				break;
			continue;
		}
		eofs = 0;

		buf[n] = '\0';
		sendp(c, buf);
	}
}
