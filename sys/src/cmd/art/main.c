/*
 * initialization of globals and command loop
 */
#include "art.h"
#define	MARGIN	3		/* width of 8.5 margin */
#define	SPACE	1		/* width of interior vertical margins */
Item *scene, *active;
Alpt *alpt;
short tcur[16]={
	 0x0100, 0x0100, 0x0380, 0x0380,
	 0x07C0, 0x07C0, 0x0FE0, 0x0EE0,
	 0x1C70, 0x1830, 0x3018, 0x2008,
	 0x0000, 0x0000, 0x0000, 0x0000,
};
short anc[16]={
	0x3838, 0x7C7C, 0xF01E, 0xF83E,
	0xDC76, 0x4EE4, 0x07C0, 0x0280,
	0x07C0, 0x4EE4, 0xDC76, 0xF83E,
	0xF01E, 0x7C7C, 0x3838, 0x0000,
};
Bitmap *cur;
Rectangle echobox;
Rectangle msgbox;
Rectangle dwgbox;
void ereshaped(Rectangle r){
	Rectangle rest;
	screen.r=r;
	rest=drawmenubar();
	fontp=curface->font;
	/* does this look like we need an array, or what? */
	echobox.min=add(rest.min, Pt(SPACE, 0));
	echobox.max=add(Pt(rest.max.x, rest.min.y), Pt(-SPACE, SPACE+fontp->height));
	msgbox.min=add(rest.min, Pt(SPACE, 2*SPACE+fontp->height));
	msgbox.max=add(Pt(rest.max.x, rest.min.y), Pt(-SPACE, 3*SPACE+2*fontp->height));
	dwgbox.min=add(rest.min, Pt(SPACE, 4*SPACE+2*fontp->height));
	dwgbox.max=sub(rest.max, Pt(SPACE, SPACE));
	if(dwgbox.max.y<=dwgbox.min.y){
		fprint(2, "%s: please make the window higher and restart: there's no space to draw in.\n",
			cmdname);
		exits("need higher window");
	}
	dwgdrect=drcanon(Drpt(P2D(dwgbox.min), P2D(dwgbox.max)));
	if(sel!=0) bfree(sel);
	sel=balloc(screen.r, screen.ldepth);
	if(sel==0){
		fprint(2, "%s: out of bitmap space\n", cmdname);
		exits("no space");
	}
	redraw();
}
void main(int argc, char *argv[]){
	Point p;
	char *fontname, *bgname;
	cmdname=argv[0];
	binit(0, 0, cmdname);
	einit(Emouse|Ekeyboard);
	ainit();
	setface("pelm,unicode,9");	/* do this anyway, in case the following fails */
	ARGBEGIN{
	case 'f':
		fontname=ARGF();
		if(fontname==0) goto Usage;
		setface(ARGF());
		break;
	case 'b':
		bgname=ARGF();
		if(bgname==0) goto Usage;
		setbg(bgname);
		break;
	default:
	Usage:
		fprint(2, "Usage: %s [-f font] [-b background]\n", cmdname);
		exits("bad font");
	}ARGEND
	faint=0x55555555&((1<<(1<<screen.ldepth))-1);
	dark=screen.ldepth==0?1:faint<<1;
	cur=balloc(Rect(0, 0, 16, 16), 1);
	anchormark=balloc(Rect(0, 0, 16, 16), 1);
	for(p.y=0;p.y!=16;p.y++) for(p.x=0;p.x!=16;p.x++){
		point(cur, p, tcur[p.y]&(1<<p.x)?faint:0, S);
		point(anchormark, p, anc[p.y]&(1<<p.x)?faint:0, S);
	}
	gravity=.13;
	scene=addhead();
	active=addhead();
	scoffs=Dpt(0., 0.);
	curpt=Dpt(0., 0.);
	ereshaped(screen.r);
	mvcurpt(P2D(div(add(dwgbox.min, dwgbox.max), 2)));
	for(;;) command();
}
