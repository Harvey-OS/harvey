/*
 * item editing commands -- line, circle, arc, edit, delete, ...
 * and support therefor
 */
#include "art.h"
Dpoint svpoint;
void Oedit(void){
	Item *s=selection;
	if(!s || narg<1){
		msg("edit: nothing selected");
		return;
	}
	use1("edit");
	cool(s);
	s->flags|=INVIS;
	reheat();		/* does a redraw! */
	s->flags&=!INVIS;
	(*s->fn->edit)();
	draw(s, scoffs, dark, S|D);
	if(heatnew) heat(s);
	activate(s);
	argument();
}
void movep(Item *ip, int i, Dpoint p){
	draw(ip, scoffs, dark, S^D);
	ip->p[i]=p;
	draw(ip, scoffs, dark, S^D);
}
void mover(Item *ip, int i, Dpoint p){
	Flt r;
	r=dist(p, ip->p[0]);
	if(r!=ip->r){
		draw(ip, scoffs, dark, S^D);
		ip->r=r;
		draw(ip, scoffs, dark, S^D);
	}
}
void movex0(Item *ip, int i, Dpoint p){
	draw(ip, scoffs, dark, S^D);
	ip->p[0].x=p.x;
	draw(ip, scoffs, dark, S^D);
}
void movey0(Item *ip, int i, Dpoint p){
	draw(ip, scoffs, dark, S^D);
	ip->p[0].y=p.y;
	draw(ip, scoffs, dark, S^D);
}
void movex1(Item *ip, int i, Dpoint p){
	draw(ip, scoffs, dark, S^D);
	ip->p[1].x=p.x;
	draw(ip, scoffs, dark, S^D);
}
void movey1(Item *ip, int i, Dpoint p){
	draw(ip, scoffs, dark, S^D);
	ip->p[1].y=p.y;
	draw(ip, scoffs, dark, S^D);
}
void movex0y1(Item *ip, int i, Dpoint p){
	draw(ip, scoffs, dark, S^D);
	ip->p[0].x=p.x;
	ip->p[1].y=p.y;
	draw(ip, scoffs, dark, S^D);
}
void movex1y0(Item *ip, int i, Dpoint p){
	draw(ip, scoffs, dark, S^D);
	ip->p[1].x=p.x;
	ip->p[0].y=p.y;
	draw(ip, scoffs, dark, S^D);
}
void moveall(Item *ip, int i, Dpoint p){
	draw(ip, scoffs, dark, S^D);
	(*ip->fn->translate)(ip, dsub(p, svpoint));
	svpoint=p;
	draw(ip, scoffs, dark, S^D);
}
Cursor dxx = {
0,0,
0x00, 0x00, 
0x0F, 0x00, 
0x0F, 0x00, 
0x0F, 0x00, 
0x7F, 0x00, 
0xFF, 0xFC, 
0xFF, 0xFC, 
0xFF, 0xFC, 
0xEF, 0xFF, 
0xEF, 0xFF, 
0xEF, 0xFF, 
0xFF, 0xFF, 
0xFF, 0x3F, 
0xFF, 0x3F, 
0x7F, 0x3F, 
0x00, 0x00, 

0x00, 0x00, 
0x00, 0x00, 
0x06, 0x00, 
0x06, 0x00, 
0x06, 0x00, 
0x3E, 0x00, 
0x66, 0xD8, 
0xC6, 0xF8, 
0xC6, 0x70, 
0xC6, 0xFB, 
0xC6, 0xDF, 
0xC6, 0x0E, 
0x6E, 0x1F, 
0x36, 0x1B, 
0x00, 0x00, 
0x00, 0x00, 
};
int use1(char *why){
	msg("Use button 1 to %s, button 2 or 3 to abort", why);
	cursorswitch(&dxx);
	while(!button123()) getmouse();
	cursorswitch((Cursor *)0);
	msg(cmdname);
	if(button1()) return 1;
	while(button123()) getmouse();
	return 0;
}
void Oline(void){
	Item *ip;
	if(narg<1){
		msg("select an endpoint, then hit line");
		return;
	}
	if(!use1("draw a line")) return;
	ip=addline(scene, arg[0], arg[0]);
	hotpoint(arg[0]);
	track(movep, 1, ip);
	draw(ip, scoffs, dark, S|D);
	if(heatnew) heat(ip);
	activate(ip);
	argument();
}
void Ocircle(void){
	Item *ip;
	if(narg<1){
		msg("select a center point, then hit circle");
		return;
	}
	if(!use1("set the circle's radius")) return;
	ip=addcircle(scene, arg[0], 0.);
	hotpoint(arg[0]);
	track(mover, 0, ip);
	draw(ip, scoffs, dark, S|D);
	if(heatnew) heat(ip);
	activate(ip);
	argument();
	msg(cmdname);
}
void Ospline(void){
	Item *ip;
	Item *s=selection;
	int i, n;
	Dpoint p;
	if(narg<1){
		msg("select an endpoint or a spline, then hit spline");
		return;
	}
	if(s && (s->type==SPLINE /*|| s->type==LINE*/)){
		if(!use1("extend a spline")) return;
		draw(s, scoffs, dark, S^D);
		setselection(0);
		n=morespline(s, arg[0]);
		for(i=0;i!=s->np;i++) if(i!=n) hotpoint(s->p[i]);
		track(movespline, n, s);
		draw(s, scoffs, dark, S|D);
		if(heatnew) heat(s);
		activate(s);
		argument();
		return;
	}
	if(!use1("start a new spline")) return;
	ip=addspline(scene, arg[0], arg[0]);
	hotpoint(arg[0]);
	track(movep, 1, ip);
	draw(ip, scoffs, dark, S|D);
	if(heatnew) heat(ip);
	activate(ip);
	argument();
}
void sweepbox(Item *ip, int i, Dpoint p){
	Item *lp;
	draw(ip, scoffs, dark, S^D);
	ip->p[1]=p;
	draw(ip, scoffs, dark, S^D);
}
void Obox(void){
	Item *ip;
	if(!use1("sweep out the box")) return;
	hotline(curpt, dadd(curpt, Dpt(1.,0.)), L);
	hotline(curpt, dadd(curpt, Dpt(0.,1.)), L);
	ip=addbox(scene, curpt, curpt);
	track(sweepbox, 0, ip);
	draw(ip, scoffs, dark, S|D);
	if(heatnew) heat(ip);
	activate(ip);
	argument();
	msg(cmdname);
}
void Oarc(void){
	Item *ip;
	if(narg<2){
		msg("select two points, then hit arc");
		return;
	}
	if(!use1("draw an arc")) return;
	ip=addarc(scene, arg[0], arg[0], arg[1]);
	hotpoint(arg[0]);
	hotpoint(arg[1]);
	track(movep, 0, ip);
	draw(ip, scoffs, dark, S|D);
	if(heatnew) heat(ip);
	activate(ip);
	argument();
}
Drectangle grpbox;
void inboxprim(Item *ip, Item *op){
	if((*ip->fn->inbox)(ip, grpbox)) op->flags|=BOXED;
}
/*
 * Create a new group by grabbing all items inside the rectangle indicated by the user
 */
void Ogroup(void){
	Flt t;
	Item *box, *ip, *grp, *next;
	int grpnum;
	for(grpnum=0;group[grpnum];grpnum++)
		if(grpnum==NGROUP){
			msg("Group table full!");
			return;
		}
	if(!use1("sweep out the group")) return;
	box=addbox(0, curpt, curpt);
	track(sweepbox, 0, box);
	grpbox=drcanon(Drpt(box->p[0], box->p[1]));
	for(ip=scene->next;ip->type!=HEAD;ip=ip->next) ip->flags&=~BOXED;
	walk(scene, Dpt(0., 0.), inboxprim);
	setselection(0);
	delete(box);
	group[grpnum]=addhead();
	draw(scene, scoffs, dark, S^D);	/* nonce */
	for(ip=scene->next;ip->type!=HEAD;ip=next){
		next=ip->next;
		if(ip->flags&BOXED){
			ip->next->prev=ip->prev;
			ip->prev->next=ip->next;
			ip->next=group[grpnum];
			ip->prev=group[grpnum]->prev;
			ip->next->prev=ip;
			ip->prev->next=ip;
		}
	}
	ip=addgroup(scene, grpnum, Dpt(0., 0.));
	if(heatnew) heat(ip);
	draw(scene, scoffs, dark, S^D);	/* nonce */
	setselection(ip);
	argument();
	msg(cmdname);
}
/*
 * Make a copy of the current object
 */
void Ocopy(void){
	Item *s=selection;
	if(!s || narg<1){
		msg("copy: nothing selected");
		return;
	}
	if(s==scene){
		msg("copy: values of ß will give rise to dom");
		return;
	}
	use1("copy");
	s=additemv(scene, s->type, s->r,
		s->face, s->text, s->group, s->fn, s->np, s->p, Dpt(0., 0.));
	cool(s);
	reheat();
	track(moveall, 0, s);
	draw(s, scoffs, dark, S|D);
	if(heatnew) heat(s);
	activate(s);
	argument();
}
