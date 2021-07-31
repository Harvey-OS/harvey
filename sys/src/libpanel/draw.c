#include <u.h>
#include <libc.h>
#include <libg.h>
#include <panel.h>
#include "pldefs.h"
#define	PWID	1	/* width of label border */
#define	BWID	1	/* width of button relief */
#define	FWID	2	/* width of frame relief */
#define	SPACE	1	/* space inside relief of button or frame */
#define	CKSIZE	3	/* size of check mark */
#define	CKSPACE	2	/* space around check mark */
#define	CKWID	1	/* width of frame around check mark */
#define	CKINSET	1	/* space around check mark frame */
#define	CKBORDER 2	/* space around X inside frame */
static int plldepth;
static Bitmap *pl_white, *pl_light, *pl_dark, *pl_black;
int pl_drawinit(int ldepth){
	int ff;
	plldepth=ldepth;
	switch(ldepth){
	default:
		ff=(1<<(1<<ldepth))-1;
		pl_white=balloc(Rect(0,0,1,1), ldepth);
		pl_light=balloc(Rect(0,0,1,1), ldepth);
		pl_dark=balloc(Rect(0,0,1,1), ldepth);
		pl_black=balloc(Rect(0,0,1,1), ldepth);
		if(pl_white==0 || pl_light==0 || pl_black==0 || pl_dark==0) return 0;
		point(pl_white, Pt(0,0), ff*0/3, S);
		point(pl_light, Pt(0,0), ff*1/3, S);
		point(pl_dark, Pt(0,0), ff*2/3, S);
		point(pl_black, Pt(0,0), ff*3/3, S);
		break;
	case 0:
		pl_light=pl_white=balloc(Rect(0,0,1,1), 0);
		pl_dark=pl_black=balloc(Rect(0,0,1,1), 0);
		if(pl_white==0 || pl_black==0) return 0;
		point(pl_white, Pt(0,0), 0, S);
		point(pl_black, Pt(0,0), 1, S);
		break;
	}
	return 1;
}
void pl_relief(Bitmap *b, Bitmap *ul, Bitmap *lr, Rectangle r, int wid){
	int x, y;
	texture(b, Rect(r.min.x, r.max.y-wid, r.max.x, r.max.y), lr, S); /* bottom */
	texture(b, Rect(r.max.x-wid, r.min.y, r.max.x, r.max.y), lr, S); /* right */
	texture(b, Rect(r.min.x, r.min.y, r.min.x+wid, r.max.y), ul, S); /* left */
	texture(b, Rect(r.min.x, r.min.y, r.max.x, r.min.y+wid), ul, S); /* top */
	for(x=0;x!=wid;x++) for(y=wid-1-x;y!=wid;y++){
		texture(b, raddp(Rect(0,0,1,1), Pt(x+r.max.x-wid, y+r.min.y)), lr, S);
		texture(b, raddp(Rect(0,0,1,1), Pt(x+r.min.x, y+r.max.y-wid)), lr, S);
	}
}
Rectangle pl_boxoutline(Bitmap *b, Rectangle r, int style, int fill){
	if(plldepth==0) switch(style){
	case UP:
		pl_relief(b, pl_black, pl_black, r, BWID);
		r=inset(r, BWID);
		if(fill) texture(b, r, pl_white, S);
		else border(b, r, SPACE, Zero);
		break;
	case DOWN:
	case DOWN1:
	case DOWN2:
	case DOWN3:
		pl_relief(b, pl_black, pl_black, r, BWID);
		r=inset(r, BWID);
		if(fill) texture(b, r, pl_black, S);
		border(b, r, SPACE, F);
		break;
	case PASSIVE:
		if(fill) texture(b, r, pl_white, S);
		r=inset(r, PWID);
		if(!fill) border(b, r, SPACE, Zero);
		break;
	case FRAME:
		pl_relief(b, pl_white, pl_black, r, FWID);
		r=inset(r, FWID);
		pl_relief(b, pl_black, pl_white, r, FWID);
		r=inset(r, FWID);
		if(fill) texture(b, r, pl_white, S);
		else border(b, r, SPACE, Zero);
		break;
	}
	else switch(style){
	case UP:
		pl_relief(b, pl_white, pl_black, r, BWID);
		r=inset(r, BWID);
		if(fill) texture(b, r, pl_light, S);
		else border(b, r, SPACE, Zero);
		break;
	case DOWN:
	case DOWN1:
	case DOWN2:
	case DOWN3:
		pl_relief(b, pl_black, pl_white, r, BWID);
		r=inset(r, BWID);
		if(fill) texture(b, r, pl_dark, S);
		else border(b, r, SPACE, Zero);
		break;
	case PASSIVE:
		if(fill) texture(b, r, pl_light, S);
		r=inset(r, PWID);
		if(!fill) border(b, r, SPACE, Zero);
		break;
	case FRAME:
		pl_relief(b, pl_white, pl_black, r, FWID);
		r=inset(r, FWID);
		pl_relief(b, pl_black, pl_white, r, FWID);
		r=inset(r, FWID);
		if(fill) texture(b, r, pl_light, S);
		else border(b, r, SPACE, Zero);
		break;
	}
	return inset(r, SPACE);
}
Rectangle pl_outline(Bitmap *b, Rectangle r, int style){
	return pl_boxoutline(b, r, style, 0);
}
Rectangle pl_box(Bitmap *b, Rectangle r, int style){
	return pl_boxoutline(b, r, style, 1);
}
Point pl_boxsize(Point interior, int state){
	switch(state){
	case UP:
	case DOWN:
	case DOWN1:
	case DOWN2:
	case DOWN3:
		return add(interior, Pt(2*(BWID+SPACE), 2*(BWID+SPACE)));
	case PASSIVE:
		return add(interior, Pt(2*(PWID+SPACE), 2*(PWID+SPACE)));
	case FRAME:
		return add(interior, Pt(4*FWID+2*SPACE, 4*FWID+2*SPACE));
	}
}
void pl_interior(int state, Point *ul, Point *size){
	switch(state){
	case UP:
	case DOWN:
	case DOWN1:
	case DOWN2:
	case DOWN3:
		*ul=add(*ul, Pt(BWID+SPACE, BWID+SPACE));
		*size=sub(*size, Pt(2*(BWID+SPACE), 2*(BWID+SPACE)));
		break;
	case PASSIVE:
		*ul=add(*ul, Pt(PWID+SPACE, PWID+SPACE));
		*size=sub(*size, Pt(2*(PWID+SPACE), 2*(PWID+SPACE)));
		break;
	case FRAME:
		*ul=add(*ul, Pt(2*FWID+SPACE, 2*FWID+SPACE));
		*size=sub(*size, Pt(4*FWID+2*SPACE, 4*FWID+2*SPACE));
	}
}
void pl_drawicon(Bitmap *b, Rectangle r, int stick, int flags, Icon *s){
	Rectangle save;
	Point ul, offs;
	save=b->clipr;
	clipr(b, r);
	ul=r.min;
	offs=sub(sub(r.max, r.min), pl_iconsize(flags, s));
	switch(stick){
	case PLACENW:	                                break;
	case PLACEN:	ul.x+=offs.x/2;                 break;
	case PLACENE:	ul.x+=offs.x;                   break;
	case PLACEW:	                ul.y+=offs.y/2; break;
	case PLACECEN:	ul.x+=offs.x/2; ul.y+=offs.y/2; break;
	case PLACEE:	ul.x+=offs.x;                   break;
	case PLACESW:	                ul.y+=offs.y;   break;
	case PLACES:	ul.x+=offs.x/2; ul.y+=offs.y;   break;
	case PLACESE:	ul.x+=offs.x;   ul.y+=offs.y;   break;
	}
	if(flags&BITMAP) bitblt(b, ul, (Bitmap *)s, ((Bitmap *)s)->r, S);
	else if(plldepth==0) string(b, ul, font, s, S^D);
	else string(b, ul, font, s, S|D);
	clipr(b, save);
}
/*
 * Place a check mark at the left end of r.  Return the unused space.
 * Caller must guarantee that r.max.x-r.min.x>=r.max.y-r.min.y!
 */
Rectangle pl_radio(Bitmap *b, Rectangle r, int val){
	Rectangle remainder;
	remainder=r;
	r.max.x=r.min.x+r.max.y-r.min.y;
	remainder.min.x=r.max.x;
	r=inset(r, CKINSET);
	if(plldepth==0)
		pl_relief(b, pl_black, pl_black, r, CKWID);
	else
		pl_relief(b, pl_black, pl_white, r, CKWID);
	r=inset(r, CKWID);
	if(plldepth==0)
		texture(b, r, pl_white, S);
	else
		texture(b, r, pl_light, S);
	if(val) texture(b, inset(r, CKSPACE), pl_black, S);
	return remainder;
}
Rectangle pl_check(Bitmap *b, Rectangle r, int val){
	Rectangle remainder;
	remainder=r;
	r.max.x=r.min.x+r.max.y-r.min.y;
	remainder.min.x=r.max.x;
	r=inset(r, CKINSET);
	if(plldepth==0)
		pl_relief(b, pl_black, pl_black, r, CKWID);
	else
		pl_relief(b, pl_black, pl_white, r, CKWID);
	r=inset(r, CKWID);
	if(plldepth==0)
		texture(b, r, pl_white, S);
	else
		texture(b, r, pl_light, S);
	r=inset(r, CKBORDER);
	if(val){
		segment(b, Pt(r.min.x,   r.min.y+1), Pt(r.max.x-1, r.max.y  ), ~0, S);
		segment(b, Pt(r.min.x,   r.min.y  ), Pt(r.max.x,   r.max.y  ), ~0, S);
		segment(b, Pt(r.min.x+1, r.min.y  ), Pt(r.max.x,   r.max.y-1), ~0, S);
		segment(b, Pt(r.min.x  , r.max.y-2), Pt(r.max.x-1, r.min.y-1), ~0, S);
		segment(b, Pt(r.min.x,   r.max.y-1), Pt(r.max.x,   r.min.y-1), ~0, S);
		segment(b, Pt(r.min.x+1, r.max.y-1), Pt(r.max.x,   r.min.y  ), ~0, S);
	}
	return remainder;
}
int pl_ckwid(void){
	return 2*(CKINSET+CKSPACE+CKWID)+CKSIZE;
}
void pl_sliderupd(Bitmap *b, Rectangle r1, int dir, int lo, int hi){
	Rectangle r2, r3;
	r2=r1;
	r3=r1;
	if(lo<0) lo=0;
	if(hi<=lo) hi=lo+1;
	switch(dir){
	case HORIZ:
		r1.max.x=r1.min.x+lo;
		r2.min.x=r1.max.x;
		r2.max.x=r1.min.x+hi;
		if(r2.max.x>r3.max.x) r2.max.x=r3.max.x;
		r3.min.x=r2.max.x;
		break;
	case VERT:
		r1.max.y=r1.min.y+lo;
		r2.min.y=r1.max.y;
		r2.max.y=r1.min.y+hi;
		if(r2.max.y>r3.max.y) r2.max.y=r3.max.y;
		r3.min.y=r2.max.y;
		break;
	}
	texture(b, r1, pl_light, S);
	texture(b, r2, pl_dark, S);
	texture(b, r3, pl_light, S);
}
void pl_draw1(Panel *p, Bitmap *b);
void pl_drawall(Panel *p, Bitmap *b){
	if(p->flags&INVIS) return;
	p->b=b;
	p->draw(p);
	for(p=p->child;p;p=p->next) pl_draw1(p, b);
}
void pl_draw1(Panel *p, Bitmap *b){
	if(b!=0)
		pl_drawall(p, b);
}
void pldraw(Panel *p, Bitmap *b){
	pl_draw1(p, b);
	bflush();
}
void pl_invis(Panel *p, int v){
	for(;p;p=p->next){
		if(v) p->flags|=INVIS; else p->flags&=~INVIS;
		pl_invis(p->child, v);
	}
}
Point pl_iconsize(int flags, Icon *p){
	if(flags&BITMAP) return sub(((Bitmap *)p)->r.max, ((Bitmap *)p)->r.min);
	return strsize(font, (char *)p);
}
