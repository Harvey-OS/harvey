/*
 * code to render drawings on the screen
 */
#include "art.h"
int drawbits;
int drawcode;
void drawprim(Item *ip, Item *op){
	if(!(ip->flags&INVIS)) (*ip->fn->draw)(ip, drawbits, &screen, drawcode);
}
void draw(Item *ip, Dpoint offs, int bits, Fcode c){
	drawbits=bits;
	drawcode=c;
	marks();
	walk(ip, offs, drawprim);
	marks();
}
/*
 * Draw (or undraw) little dinks at argument points
 * These are always done in xor mode, and must be erased and redrawn
 * around any non xor-mode drawing.
 */
Bitmap *sqmark=0;
void marks(void){
	Point p0, p1, p2;
	int i;
	if(sqmark==0){
		sqmark=balloc(Rect(0, 0, 3, 3), 0);
		clrb(sqmark, sqmark->r, FAINT);
	}
	if(narg>1){
		p0=D2P(dadd(arg[0], scoffs));
		p1=D2P(dadd(arg[1], scoffs));
		if(!eqpt(p0, p1))
			bitblt(&screen, Pt(p1.x-1, p1.y-1), sqmark, sqmark->r, S^D);
		if(narg>2 && !eqpt(p0, p2=D2P(dadd(arg[2], scoffs))) && !eqpt(p1, p2)){
			segment(&screen, sub(p2, Pt(2, 2)), add(p2, Pt(3, 3)), FAINT, S^D);
			segment(&screen, sub(p2, Pt(2, -2)), p2, FAINT, S^D);
			segment(&screen, sub(p2, Pt(-2, 2)), p2, FAINT, S^D);
		}
	}
	p0=D2P(dadd(anchor, scoffs));
	bitblt(&screen, Pt(p0.x-8, p0.y-7), anchormark, anchormark->r, S^D);
}
/*
 * Redraw absolutely everything
 */
void redraw(void){
	rectf(sel, screen.r, F);
	rectf(sel, echobox, Zero);
	rectf(sel, msgbox, Zero);
	rectf(sel, dwgbox, Zero);
	if(bg)
		bitblt(sel, Pt(dwgbox.min.x, dwgbox.max.y-(bg->r.max.y-bg->r.min.y)),
			bg, bg->r, S);
	rectf(sel, echobox, Zero);
	rectf(sel, msgbox, Zero);
	bitblt(&screen, screen.r.min, sel, sel->r, S);
	drawmenubar();
	echo(cmd);
	msg(cmdname);
	if(scsp!=scstack)
		draw(scstack[0].scene, scstack[0].scoffs, DARK, S|D);
	else
		draw(scene, scoffs, DARK, S|D);
	draw(active, Dpt(0., 0.), FAINT, S|D);
	drawcurpt();
	marks();
	drawsel();
	drawgrid();
}
#include <fb.h>
void setbg(char *file){
	PICFILE *f=picopen_r(file);
	if(f==0){
	NoGood:
		picerror(file);
		exits("can't get background");
	}
	bg=rdpicfile(f, screen.ldepth);
	if(bg==0) goto NoGood;
}
