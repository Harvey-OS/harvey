#include "art.h"
/*
 * Code for manipulating the selection
 */
Item *selection;
void setselection(Item *p){
	if(selection==p) return;
	/*
	 * This should be done differently in the brave new non-xor world
	 * but it appears to involve redrawing everything when the selection changes
	 */
	drawsel();
	selection=p;
	drawsel();
}
/*
 * It would be nice if this were faster
 */
void drawselprim(Item *ip, Item *op){
	(*ip->fn->draw)(ip, faint, sel, S|D);
}
void drawsel(void){
	if(selection==0) return;
	rectf(sel, sel->r, Zero);
	walk(selection, scoffs, drawselprim);
	bitblt(sel, add(sel->r.min, Pt(1, 1)), sel, sel->r, S|D);
	bitblt(sel, add(sel->r.min, Pt(1, -1)), sel, sel->r, S|D);
	bitblt(&screen, add(dwgbox.min, Pt(-1, 0)), sel, dwgbox, S^D);
}
