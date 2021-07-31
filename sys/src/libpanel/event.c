#include <u.h>
#include <libc.h>
#include <libg.h>
#include <panel.h>
#include "pldefs.h"
Panel *pl_kbfocus;
void plgrabkb(Panel *g){
	pl_kbfocus=g;
}
void plkeyboard(Rune c){
	if(pl_kbfocus){
		pl_kbfocus->type(pl_kbfocus, c);
		bflush();
	}
}
Panel *pl_ptinpanel(Point p, Panel *g){
	Panel *v;
	for(;g;g=g->next) if(ptinrect(p, g->r)){
		if(g->flags&HITME) return g;
		v=pl_ptinpanel(p, g->child);
		if(v) return v;
		return g;
	}
	return 0;
}
void plmouse(Panel *g, Mouse mouse){
	Panel *hit, *last;
	if(g->flags&REMOUSE)
		hit=g->lastmouse;
	else{
		hit=pl_ptinpanel(mouse.xy, g);
		last=g->lastmouse;
		if(last && last!=hit){
			mouse.buttons|=OUT;
			last->hit(last, &mouse);
			mouse.buttons&=~OUT;
		}
	}
	if(hit){
		if(hit->hit(hit, &mouse))
			g->flags|=REMOUSE;
		else
			g->flags&=~REMOUSE;
		g->lastmouse=hit;
	}
	bflush();
}
