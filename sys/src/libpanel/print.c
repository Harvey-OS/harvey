#include <u.h>
#include <libc.h>
#include <libg.h>
#include <panel.h>
#include "pldefs.h"
void pl_iprint(int indent, char *fmt, ...){
	char buf[8192];
	memset(buf, '\t', indent);
	write(1, buf, doprint(buf+indent, buf+sizeof(buf), fmt, &fmt+1)-buf);
}
void pl_ipprint(Panel *p, int n){
	Panel *c;
	char *place, *stick;
	pl_iprint(n, "%s (0x%.8x)\n", p->kind, p);
	pl_iprint(n, "  r=(%d %d, %d %d)\n",
		p->r.min.x, p->r.min.y, p->r.max.x, p->r.max.y);
	switch(p->flags&PACK){
	default: SET(place); break;
	case PACKN: place="n"; break;
	case PACKE: place="e"; break;
	case PACKS: place="s"; break;
	case PACKW: place="w"; break;
	}
	switch(p->flags&PLACE){
	default: SET(stick); break;
	case PLACECEN:	stick=""; break;
	case PLACES:	stick=" stick s"; break;
	case PLACEE:	stick=" stick e"; break;
	case PLACEW:	stick=" stick w"; break;
	case PLACEN:	stick=" stick n"; break;
	case PLACENE:	stick=" stick ne"; break;
	case PLACENW:	stick=" stick nw"; break;
	case PLACESE:	stick=" stick se"; break;
	case PLACESW:	stick=" stick sw"; break;
	}
	pl_iprint(n, "  place %s%s%s%s%s%s\n",
		place,
		p->flags&FILLX?" fill x":"",
		p->flags&FILLY?" fill y":"",
		stick,
		p->flags&EXPAND?" expand":"",
		p->flags&FIXED?" fixed":"");
	if(!eqpt(p->pad, Pt(0, 0))) pl_iprint(n, "  pad=%d,%d)\n", p->pad.x, p->pad.y);
	if(!eqpt(p->ipad, Pt(0, 0))) pl_iprint(n, "  ipad=%d,%d)\n", p->ipad.x, p->ipad.y);
	pl_iprint(n, "  size=(%d,%d), sizereq=(%d,%d)\n",
		p->size.x, p->size.y, p->sizereq.x, p->sizereq.y);
	for(c=p->child;c;c=c->next)
		pl_ipprint(c, n+1);
}
void pl_print(Panel *p){
	pl_ipprint(p, 0);
}
