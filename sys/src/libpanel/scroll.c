#include <u.h>
#include <libc.h>
#include <libg.h>
#include <panel.h>
#include "pldefs.h"
void plscroll(Panel *scrollee, Panel *xscroller, Panel *yscroller){
	scrollee->xscroller=xscroller;
	scrollee->yscroller=yscroller;
	if(xscroller) xscroller->scrollee=scrollee;
	if(yscroller) yscroller->scrollee=scrollee;
}
Scroll plgetscroll(Panel *p){
	return p->scr;
}
void plsetscroll(Panel *p, Scroll s){
	if(p->scroll){
		if(s.size.x) p->scroll(p, HORIZ, 2, s.pos.x, s.size.x);
		if(s.size.y) p->scroll(p, VERT, 2, s.pos.y, s.size.y);
	}
}
