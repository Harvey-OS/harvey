#include <u.h>
#include <libc.h>
#include <libg.h>
#include "term.h"
#include "proto.h"

#define		MSIZE		100

void uncache(void);
extern char message[];
extern int errfd;
extern Bitmap *black;
extern Bitmap *grey;
extern Bitmap screen;
extern Font * defont, * tiny;
static fc = F_XOR;

void
ereshaped(Rectangle r)
{
	screen.r = r;
	message[0] = 0;
	rectf(&screen, Drect, F_CLR);
	r = scrn.sr = Drect;
	scrn.sr.origin.y += 2+MSIZE;
	r.corner.y = scrn.sr.origin.y;
	r.origin.y = r.corner.y - 2;
	rectf(&screen, r, F_OR);
	scrn.map.origin = Drect.origin;
	scrn.map.corner = add(scrn.map.origin, Pt(MSIZE, MSIZE));
	r = raddp(Rect(0, -MSIZE, 2, 0), scrn.map.corner);
	rectf(&screen, r, F_OR);
	scrn.panel.origin.x = scrn.map.corner.x+4;
	scrn.panel.origin.y = scrn.map.origin.y;
	scrn.panel.corner.x = Drect.corner.x;
	scrn.panel.corner.y = scrn.map.corner.y;
	scrn.bname.origin = add(scrn.panel.origin, Pt(5,5));
	scrn.bname.corner = add(scrn.bname.origin, Pt(defont->height*32, defont->height));
	scrn.coord = scrn.panel;
	scrn.coord.origin.y = scrn.map.corner.y - defont->height;
	scrn.coord.corner.x = scrn.coord.origin.x + defont->height*11;
	scrn.br = rstob(scrn.sr);
	scrn.bmr = Rpt(scrn.map.origin, scrn.map.origin);
	Clip(scrn.sr);
	drawmap();
	drawcolourmap();
	panto(scrn.bmax.origin);
}

void
drawplane(int signo)
{
	register Plane *p;
	register i;

	for(i = 0, p = b.planes; i < b.nplanes; i++, p++)
		if(p->signo == signo) {
			wrbitmap(colour, 0, 16, &colourbits[32 * -b.planemap[signo]]);
			Ctexture(&screen, rbtos(p->r), colour, F_XOR);
		}
}

void
drawkeepout(int signo)
{
	register Plane *p;
	register i;

	for(i = 0, p = b.keepouts; i < b.nkeepouts; i++, p++)
		if(p->signo == signo) {
			wrbitmap(colour, 0, 16, &colourbits[32 * -b.planemap[signo]]);
			Ctexture(&screen, rbtos(p->r), colour, F_XOR);
		}
}
#define WDSZ 8
void drawcpin(Bitmap *, Chip *);
void freeall(Chip *c);

void
drawchip(Chip *c)
{
	Point p, ssize;
	char *s;
	Rectangle r;
	Bitmap * bb, *bbbb;
	int tmp, widths,  widthd;
	Font * font;
	uchar *bufs;
	uchar *bufd;

	if((c->flags&PLACED) == 0)
		return;
	c->sr = rbtos(c->br);
	if((c->flags&MAPPED)) 
		goto mapped;
	if(bbbb = c->b) bfree(bbbb);
	r = Rect(0, 0, c->sr.max.x - c->sr.min.x, c->sr.max.y - c->sr.min.y);
	if((bbbb = balloc(r, 0)) == 0) {
		freeall(c);
		if((bbbb = balloc(r, 0)) == 0) {
			perror("can't balloc");
			exits("can't balloc");
		} 
	}
	texture(bbbb, r, black, S);
	texture(bbbb, inset(r, 1), black, F_XOR);
	if(c->npins > 0) {
		drawcpin(bbbb, c);
		c->flags |= MAPPED;
		c->b = bbbb;
		goto mapped;
	}
	rectf(bbbb, rsubp(rbtos(raddp(Rect(-50, -50, 50, 50), c->pmin)), c->sr.min),
		F_XOR);
	font = defont;
	s = &b.chipstr[scrn.shown ? c->name : c->type];
	ssize = strsize(font, s);
	if((c->sr.corner.y - c->sr.origin.y) > (c->sr.corner.x	- c->sr.origin.x)) {
		if((ssize.y > (c->sr.corner.x - c->sr.origin.x)) ||
			(ssize.x > (c->sr.corner.y - c->sr.origin.y))) {
				font = tiny;
				ssize = strsize(font, s);
		}
		if((bb = balloc(Rect(0, 0, ssize.x, ssize.y), 0)) == 0) {
			freeall(c);
			if((bb = balloc(Rect(0, 0, ssize.x, ssize.y), 0)) == 0) {
				perror("can't balloc");
				exits("can't balloc");
			}
		}
		string(bb, Pt(0,0), font, s, F_STORE);
		widths = (ssize.x + WDSZ - 1)/WDSZ;
		bufs = (uchar *) malloc((long) (widths * ssize.y));
		rdbitmap(bb, 0, ssize.y, bufs);
		bfree(bb);
		if((bb = balloc(Rect(0, 0, ssize.y, ssize.x), 0)) == 0){
			freeall(c);
			if((bb = balloc(Rect(0, 0, ssize.y, ssize.x), 0)) == 0) {
				perror("can't balloc");
				exits("can't balloc");
			}
		}
;
		widthd = (ssize.y + WDSZ - 1)/WDSZ;
		bufd = (uchar *) malloc((long) (widthd * ssize.x));
		rot(bufd, widthd, bufs, widths, ssize.x, ssize.y);
		wrbitmap(bb, 0, ssize.x, bufd);
		free(bufs);
		free(bufd);
		tmp = ssize.x;
		ssize.x = ssize.y;
		ssize.y = tmp;
	}
	else {
		if((ssize.x > (c->sr.corner.x - c->sr.origin.x)) ||
			(ssize.y > (c->sr.corner.y - c->sr.origin.y))) {
				font = tiny;
				ssize = strsize(font, s);
		}
		if((bb = balloc(Rect(0, 0, ssize.x, ssize.y), 0)) == 0) {
			freeall(c);
			if((bb = balloc(Rect(0, 0, ssize.x, ssize.y), 0)) == 0) {
				perror("can't balloc");
				exits("can't balloc");
			}
		}
		string(bb, Pt(0,0) , font, s, F_STORE);
	}
	p = div(sub(sub(c->sr.corner, c->sr.origin), ssize), 2);
	bitblt(bbbb, p, bb, Rect(0, 0, ssize.x, ssize.y), F_XOR);
	bfree(bb);
	c->flags |= MAPPED;
	c->b = bbbb;
mapped:
	if((c->flags&WSIDE) != (scrn.selws ? WSIDE : 0)) {
		Crectf(&screen, c->sr, fc);
		Crectf(&screen, inset(c->sr, 1), F_XOR);
	}
	else {		
		Cbitblt(&screen, c->sr.min, c->b, (c->b)->r, fc);
		if(c->flags&SELECTED)
			Crectf(&screen, c->sr, fc);
	}

}

extern Bitmap *gndpin;
extern Bitmap *vccpin;
extern Bitmap *sigpin;
extern Bitmap *ncpin;

void
drawcpin(Bitmap *b, Chip *c)
{
	Point pp;
	Pin *p;
	int i;
	Bitmap *bb;

	if(c->pins == (Pin*) 0) {
		put1(SENDPIN);
		putn(c->id);
		while(rcv());
	}
	for(p = c->pins, i = 0; i < c->npins; i++, p++){
		pp.x = muldiv(scrn.mag, p->pos.x, UNITMAG*10) - 8;
		pp.y = muldiv(scrn.mag, p->pos.y, UNITMAG*10) - 8;
		if(p->type == -1)
			bb = gndpin;
		else if(p->type == -2)
			bb = vccpin;
		else if(p->type == -8)
			bb = ncpin;
		else 
			bb = sigpin; 
		bitblt(b, pp, bb, Rect(0, 0, 16, 16), F_XOR);
	}
}

void
drawpins(Pinhole *p)
{
	Point q;

	for(q.x = p->r.origin.x; q.x < p->r.corner.x; q.x += p->sp.x)
		for(q.y = p->r.origin.y; q.y < p->r.corner.y; q.y += p->sp.y)
			if(ptinrect(q, scrn.br))
				point(&screen, pbtos(q),  ~0, fc);
}


void
gdrawsig(int i)
{
	register Signal *s = b.sigs+i;
	if(b.resig || (s->pts[0].x < -1)){
		put1(SENDSIG);
		putn(s->id);
		while(rcv());
	} else
		drawsig(i);
}

void newcaption(Chip *);

void
drawsig(int i)
{
	register Signal *s = b.sigs+i;
	Point pp, lp;
	register Point *p;

	if(s->pts[0].x < -1)
		return;
	for(p = s->pts, i = 0; i < s->npts; i++, p++){
		if(p->x < 0)
			continue;
		pp = pbtos(*p);		
		Crectf(&screen, Rect(pp.x-3, pp.y-3, pp.x+3, pp.y+3), F_OR);
		if(i)
			Csegment(&screen, lp, pp, F_OR);
		lp = pp;
	}
	newcaption((Chip *) 0);
}

void
xdraw(void)
{
	fc = F_XOR;
	draw();
}

void
odraw(void)
{
	fc = F_OR;
	draw();
}

void
draw(void)
{
	register Chip *c;
	register Pinhole *p;
	register i;

	rectf(&screen, scrn.bname, F_CLR);
	if(message[0])
		string(&screen, scrn.bname.min, defont, message, S^D);
	else if(caption)
		string(&screen, scrn.bname.origin, defont, caption, S^D);
	rectf(&screen, scrn.sr, F_CLR);
	if(scrn.showo)
		drawoverlay();
	if(b.chips)
		for(c = b.chips; !(c->flags&EOLIST); c++)
			if(rectXrect(c->br, scrn.br))
				drawchip(c);
			else
				c->sr.corner.x = -1;	/* not valid */
	if(b.pinholes && scrn.showp)
		for(p = b.pinholes; p->sp.x >= 0; p++)
			if(rectXrect(p->r, scrn.br))
				drawpins(p);
	if(b.sigs && (b.ncursig > 0)){
		for(i = 0; i < b.ncursig; i++)
			gdrawsig(b.cursig[i]);
		b.resig = 0;
	}
	if(b.planes && (b.nplanes > 0)){
		for(i = 0; i < 6; i++)
			if(b.planemap[i] < 0)
				drawplane(i);
	}
	if(b.keepouts && (b.nkeepouts > 0)){
		for(i = 0; i < 6; i++)
			if(b.keepoutmap[i] < 0)
				drawkeepout(i);
	}
	track();
}

void
strxor(Point p, char *s)
{
	Cstring(&screen, p, scrn.font, s, F_XOR);
}

void
shchip(void)
{
	scrn.shown = !scrn.shown;
	uncache();
	draw();
}

void
shpin(void)
{
	scrn.showp = !scrn.showp;
	draw();
}


void
slws(void)
{
	scrn.selws = !scrn.selws;
	draw();
}

void
shovly(void)
{
	scrn.showo = !scrn.showo;
	draw();
}

void
shfont(void)
{
	if(scrn.font == defont)
		scrn.font = tiny;
	else
		scrn.font = defont;
	draw();
}

void
zoomin(void)
{
	int tmp;
	int lowest = muldiv(UNITMAG, 3, 2);
	if(scrn.mag < lowest){
		uncache();
		tmp = muldiv(scrn.mag, 9, 8);
		scrn.mag = (tmp > scrn.mag) ? tmp : tmp + 1;
		if(scrn.mag > lowest)
			scrn.mag = lowest;
		scrn.br = rstob(scrn.sr);
		panto(rstob(scrn.sr).origin);
	}
}

void
zoomout(void)
{
	int tmp;
	int biggest = muldiv(UNITMAG, 1, 3);

	if(scrn.mag > biggest){
		uncache();
		tmp = muldiv(scrn.mag, 8, 9);
		scrn.mag = (tmp < scrn.mag) ? tmp : tmp -1;
		if(scrn.mag < biggest)
			scrn.mag = biggest;
		scrn.br = rstob(scrn.sr);
		panto(rstob(scrn.sr).origin);
	}
}

void
freeall(Chip *cc)
{
	Chip *c;
	Bitmap *bbbb;
	for(c = b.chips; !(c->flags&EOLIST); c++) {
		if(c == cc) continue;
		if(bbbb = c->b) bfree(bbbb);
		c->b = (Bitmap *) 0;
		c->flags &= ~MAPPED;
	}
}

void
uncache(void)
{
	Chip *c;
	for(c = b.chips; !(c->flags&EOLIST); c++)
			c->flags &= ~MAPPED;
}
