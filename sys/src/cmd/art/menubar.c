#include "art.h"
char *mslopes[], *mparallels[], *mcircles[], *mangles[], *mgrid[], *mgravity[], *mheating[];
char **menubar[]={
	mslopes,
	mangles,
	mparallels,
	mcircles,
	mgrid,
	mgravity,
	mheating,
	0
};
int hitmenubar(void){
	int hit=mbarhit(&mouse);
	if(hit==-1) return -1;
	switch(hit/100){
	case 0: Oslope(hit%100); break;
	case 1: Oangle(hit%100); break;
	case 2: Oparallel(hit%100); break;
	case 3: Ocirc(hit%100); break;
	case 4: Ogrid(hit%100); break;
	case 5:	Ogravity(hit%100); break;
	case 6: Oheating(hit%100); break;
	}
	return hit;
}
Rectangle drawmenubar(void){
	return mbarinit(menubar);
}
/*
 * Menu bar library routines.
 *
 * Rectangle mbarinit(char **menubar[])
 *	draws a menu bar on the screen, returns a rectangle covering the free
 *	part of the screen (not in the menu bar and not in the 8-1/2 border).
 *	Must be called before to 
 *	The argument is an array of pointers, one per menu, to arrays of strings.
 *	The first element of each string array is a name to display on the menu bar.
 *	The remaining elements are the menu items.
 *	All arrays end at zero pointers.
 *
 * int mbarhit(Mouse *m)
 *	returns a button indicator (100*col+row), where menubar[col][row+1] is the button
 *	hit, or -1 on no hit.
 *	The argument initially points to a mouse event that should have some button down.
 *	It is updated to be the event that caused mbarhit to return.  If m->buttons
 *	is non-zero on return, then m->xy was not inside the menu bar on entry.  (In this
 *	case, m has not changed and mbarhit returns -1.)
 */
#define	BORDER	4	/* width of 8+1/2 border */
#define	SPACE	10	/* inter-button spacing on menu bar */
static Rectangle bar;	/* encloses menu bar */
static char ***buttons;
Rectangle mbarinit(char **menubar[]){
	int i;
	Point p;
	buttons=menubar;
	bar.min=add(screen.r.min, Pt(BORDER, BORDER));
	bar.max.x=screen.r.max.x-BORDER;
	bar.max.y=screen.r.min.y+BORDER+4+font->height;
	bitblt(&screen, bar.min, &screen, bar, Zero);
	border(&screen, bar, 1, F);
	p=add(bar.min, Pt(2+SPACE/2, 2));
	for(i=0;buttons[i];i++){
		string(&screen, p, font, buttons[i][0], S);
		p.x+=strwidth(font, buttons[i][0])+SPACE;
	}
	return Rect(bar.min.x, bar.max.y, bar.max.x, screen.r.max.y-BORDER);
}
int mbarhit(Mouse *m){
	Bitmap *save=0;		/* screen under menu is saved here */
	Rectangle colcom;	/* menu bar is complemented under this rectangle */
	int col=-1;		/* index of currently visible menu */
	Rectangle rowcom;	/* menu is complemented under this rectangle */
	int row=-1;		/* index of currently visible menu item */
	int nmenu;		/* number of rows in currently visible menu */
	int i, j, right;
	Rectangle r;
	Point p;
	if(!m->buttons || !ptinrect(m->xy, bar)) return -1;
	for(;m->buttons;*m=emouse()){
		if(save && ptinrect(m->xy, save->r)){
			if(m->xy.y<save->r.min.y+1) j=0;
			else{
				j=(m->xy.y-save->r.min.y-1)/(font->height+2);
				if(nmenu<=j) j=nmenu-1;
			}
			if(row==j) continue;
			if(row>=0) bitblt(&screen, rowcom.min, &screen, rowcom, ~D);
			row=j;
			rowcom.min.x=save->r.min.x+1;
			rowcom.max.x=save->r.max.x-1;
			rowcom.min.y=save->r.min.y+1+row*(2+font->height);
			rowcom.max.y=rowcom.min.y+2+font->height;
			bitblt(&screen, rowcom.min, &screen, rowcom, ~D);
		}
		else{
			if(row>=0){
				bitblt(&screen, rowcom.min, &screen, rowcom, ~D);
				row=-1;
			}
			if(!ptinrect(m->xy, bar)) continue;
			r.min.y=bar.min.y;
			r.max.y=bar.max.y;
			r.max.x=bar.min.x+SPACE/2;
			for(i=0;buttons[i];i++){
				r.min.x=r.max.x;
				r.max.x+=strwidth(font, buttons[i][0])+SPACE;
				if(ptinrect(m->xy, r)) break;
			}
			if(!buttons[i]) i=-1;
			if(i==col) continue;
			col=i;
			if(save){
				bitblt(&screen, save->r.min, save, save->r, S);
				bitblt(&screen, colcom.min, &screen, colcom, ~D);
				bfree(save);
				save=0;
			}
			if(i==-1) continue;
			colcom=r;
			r.min=Pt(colcom.min.x, colcom.max.y-1);
			r.max=add(r.min, Pt(4, 2));
			colcom.min.y++;
			--colcom.max.y;
			for(j=1;buttons[i][j];j++){
				right=strwidth(font, buttons[i][j])+4+r.min.x;
				if(r.max.x<right) r.max.x=right;
				r.max.y+=font->height+2;
			}
			save=balloc(r, screen.ldepth);
			if(!save) break;
			bitblt(&screen, colcom.min, &screen, colcom, ~D);
			bitblt(save, r.min, &screen, r, S);
			bitblt(&screen, r.min, &screen, r, Zero);
			border(&screen, r, 1, F);
			p=add(r.min, Pt(2, 2));
			for(j=1;buttons[i][j];j++){
				string(&screen, p, font, buttons[i][j], S);
				p.y+=2+font->height;
			}
			nmenu=j-1;
		}
	}
	if(save){
		bitblt(&screen, save->r.min, save, save->r, S);
		bitblt(&screen, colcom.min, &screen, colcom, ~D);
		bfree(save);
	}
	return col>=0 && row>=0?col*100+row:-1;
}
