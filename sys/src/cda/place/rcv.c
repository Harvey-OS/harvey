#include <u.h>
#include <libc.h>
#include <libg.h>
#include "menu.h"
#include "term.h"
#include "proto.h"

extern long wraplen;

long getl(void);
int getc(void);
int getn(void);
Point getp(void);
Rectangle getr(void);
char *gets(void);
void getchip(Chip *, int);
short chsig[512];
extern Font *defont;
extern Bitmap screen;
extern char message[];
extern int errfd;

static
ccmp(Chip * a, Chip * c)
{
	return(strcmp(b.chipstr+a->name, b.chipstr+c->name));
}

static
scmp(Signal * a, Signal * c)
{
	return(strcmp(b.chipstr+a->name, b.chipstr+c->name));
}

int
rcv(void)
{
	int n, i;
	Pinhole *ph;
	Pin *pn;
	char buf[128];
	char *s;
	register Chip *c;
	Point *pp;
	Signal *ss;
	extern char rfilename[];

	n = rcvchar();
	fprint(errfd, "rcv: %c %x\n", n, n);
	switch(n)
	{
	case BOARDNAME:
		if(caption) {
			free(caption);
			if(message[0] == 0)
				string(&screen, scrn.bname.min, defont, caption, S^D);
		}
		if(b.name) free(b.name);
		b.name = gets();
		strcpy(buf, b.name);
		strcat(buf, "   ");
		if(b.chips) {
			for(c = b.chips; !(c->flags&EOLIST); c++)
				if((c->flags&(SELECTED|WSIDE))
					== (scrn.selws ? (SELECTED|WSIDE) : SELECTED))
						break;
			if(!(c->flags&EOLIST)) 
				strcat(buf, c->name+b.chipstr);
		}
		caption = (char *)  malloc(1+strlen(buf));
		strcpy(caption, buf);
		if(message[0] == 0)
			string(&screen, scrn.bname.min, defont, caption, S^D);
		break;
	case CHIP:
		n = getn();
		for(c = b.chips; c->id != n; c++)
			if(c->flags&EOLIST)
				panic("chip upd");
		cursorswitch(&blank);
		drawchip(c);
		getchip(c, n);
		drawchip(c);
		cursorswitch((Cursor *) 0);
		b.resig = 1;
		break;
	case CHIPS:
		n = getn()+1;
		if(b.chips)
			free((char *)b.chips);
		b.chips = (Chip *) malloc(n*sizeof(Chip));
		for(i = 0; i < n-1; i++){
			b.chips[i].flags = 0;
			b.chips[i].b = (Bitmap *) 0;
			b.chips[i].pins = (Pin *) 0;
			b.chips[i].npins = -1;
			getchip(&b.chips[i], i+1);
		}
		b.chips[i].flags = EOLIST;
		break;

	case PINS:
		n = getn();
		for(c = b.chips; c->id != n; c++)
			if(c->flags&EOLIST)
				panic("chip upd");
		n = (c->npins >= 0) ? 1 : -1;
		c->npins = getn();
		if(c->npins == 0) {
			break;
		}
		if(c->pins == (Pin *) 0)
			c->pins = (Pin *) malloc(c->npins*sizeof(Pin));
		for(i = 0, pn = c->pins; i < c->npins; ++i,++pn) {
			pn->pos = getp();
			pn->type = getn();
		}
		c->npins *= n;
		break;

	case SIG:
		n = getn();
		for(i = 0, ss = b.sigs; ss->id != n; i++, ss++)
			if(ss->name == 0)
				panic("bad signal id");
		for(i = 0, pp = ss->pts, n = ss->npts; i < n; i++)
			*pp++ = getp();
		drawsig(ss-b.sigs);
		break;
	case SIG_ND:
		n = getn();
		for(i = 0, ss = b.sigs; ss->id != n; i++, ss++)
			if(ss->name == 0)
				panic("bad signal id");
		drawsig(ss-b.sigs);
		break;
	case SIGS:
		n = getn();
		if(b.sigs)
			free((char *)b.sigs);
		b.sigs = (Signal *) malloc((n+1)*sizeof(Signal));
		for(i = 0, ss = b.sigs; i < n; i++, ss++){
			ss->name = getn();
			ss->npts = getn();
			ss->pts = (Point *) malloc(b.sigs[i].npts*sizeof(Point));
			ss->pts[0].x = -2;
			ss->id = i;
		}
		b.sigs[i].name = 0;
		b.ncursig = 0;
		b.resig = 0;
		qsort(b.sigs, n, sizeof(Signal), scmp);
		break;
	case CHIPSIGS:
		for(n = 0; (chsig[n] = getn()) >= 0; n++)
			;
		getsigs(chsig);
		break;
	case PINHOLES:
		n = getn()+1;
		if(b.pinholes)
			free((char *)b.pinholes);
		b.pinholes = (Pinhole *) malloc(n*sizeof(Pinhole));
		for(i = 1, ph = b.pinholes; i < n; i++, ph++){
			ph->r = getr();
			ph->sp = getp();
		}
		ph->sp.x = -1;
		break;
	case CHIPSTR:
		n = getn();
		if(b.chipstr)
			free(b.chipstr);
		b.chipstr = (char *) malloc(n);
		for(i = 0; i < n; i++)
			b.chipstr[i] = getc();
		if(b.chips){
			for(i = 0; !(b.chips[i].flags&EOLIST); i++)
				;
			qsort(b.chips, i, sizeof(Chip), ccmp);
		}
		break;
	case PLANES:
		for(n = 0; n < 6; n++)
			b.planemap[n] = 0;
		b.nplanes = n = getn();
		if(b.planes)
			free((char *)b.planes);
		b.planes = (Plane *) malloc(n*sizeof(Plane));
		for(i = 0; i < n; i++){
			b.planemap[b.planes[i].signo = getn()] = 1;
			b.planes[i].r = getr();
		}
		planecolours();
		break;
	case KEEPOUTS:
		for(n = 0; n < 6; n++)
			b.keepoutmap[n] = 0;
		b.nkeepouts = n = getn();
		if(b.keepouts)
			free((char *)b.keepouts);
		b.keepouts = (Plane *) malloc(n*sizeof(Plane));
		for(i = 0; i < n; i++){
			b.keepoutmap[b.keepouts[i].signo = getn()] = 1;
			b.keepouts[i].r = getr();
		}
		break;
	case FILES:
		s = gets();
		strcpy(rfilename, s);	
		free(s);
		break;
	case DRAW:
		ereshaped(screen.r);
		break;
	case BBOUNDS:
		scrn.bmaxx = getr();
		newbmax();
		break;
	case EOF:
		return(0);

	default:
		sprint(buf, "unknown command from execute server %x", n);
		hosterr(buf, 1);
		return(1);

	case ERROR:
		s = gets();
		n = (*s == '*') ? 0 : 1;
		hosterr(s, n);
		free(s);
		if(n) return(1);
		break;

	case IMPROVES:
		improveinfo(getl());
		string(&screen, scrn.bname.min, defont, message, S^D);
		sprint(message, "wrap length = %ld", wraplen);
		string(&screen, scrn.bname.min, defont, message, S^D);
		break;

	}
	return(1);
}

void
getchip(Chip * c, int n)
{
	c->flags = getc() | (c->flags&(SELECTED|MAPPED));
	if((c->flags&UNPLACED) == 0)
		c->flags |= PLACED;
	c->br = getr();
	c->pmin = getp();
	c->name = getn();
	c->type = getn();
	c->id = n;
}

long
getl(void)
{
	long l;

	l = getn()<<16;
	l |= getn()&0xFFFF;
	return(l);
}

Point
getp(void)
{
	Point p;

	p.x = getn();
	p.y = getn();
	return(p);
}

Rectangle
getr(void)
{
	Rectangle r;

	r.origin = getp();
	r.corner = getp();
	return(r);
}

char *
gets(void)
{
	char buf[256];
	char *s;

	for(s = buf; *s = getc(); s++)
		;
	s = (char *)  malloc(1+(int)(s-buf));
	strcpy(s, buf);
	return(s);
}
