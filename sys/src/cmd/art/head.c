#include "art.h"
Dpoint nearhead(Item *ip, Dpoint testp){
	msg("near called on head");	/* uses walk */
	return testp;
}
void drawhead(Item *ip, int bits, Bitmap *b, Fcode c){
	msg("draw called on head");	/* uses walk */
}
void edithead(void){
	track(moveall, 0, selection);
}
void translatehead(Item *ip, Dpoint delta){
	Item *p;
	for(p=ip->next;p!=ip;p=p->next)
		(*p->fn->translate)(p, delta);
}
void deletehead(Item *p){
	while(p->next!=p)
		delete(p->next);
}
void writehead(Item *ip, int f){
	Item *p;
	for(p=ip->next;p!=ip;p=p->next)
		(*p->fn->write)(p, f);
}
void activatehead(Item *ip){
	msg("activate called on head");	/* uses walk */
}
int inboxhead(Item *ip, Drectangle r){
	msg("inbox called on head");		/* uses walk */
	return 0;
}
Drectangle bboxhead(Item *ip){
	Drectangle r;
	if(ip->next==ip) return Drect(0., 0., 0., 0.);
	ip=ip->next;
	r=(*ip->fn->bbox)(ip);
	for(ip=ip->next;ip->type!=HEAD;ip=ip->next)
		r=dunion(r, (*ip->fn->bbox)(ip));
	return r;
}
Dpoint nearverthead(Item *ip, Dpoint testp){
	msg("nearvert called on group");
	return testp;
}
Itemfns headfns={
	deletehead,
	writehead,
	activatehead,
	nearhead,
	drawhead,
	edithead,
	translatehead,
	inboxhead,
	bboxhead,
	nearverthead,
};
Item *addhead(void){
	return additem(0, HEAD, 0., 0, 0, 0, &headfns, 0);
}
