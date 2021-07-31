#include "art.h"
Dpoint neargroup(Item *ip, Dpoint testp){
	msg("near called on group!");	/* uses walk */
	return testp;
}
void drawgroup(Item *ip, int bits, Bitmap *b, Fcode c){
	msg("draw called on group");	/* uses walk */
}
void editgroup(void){
	track(moveall, 0, selection);
}
void translategroup(Item *ip, Dpoint delta){
	ip->p[0]=dadd(ip->p[0], delta);
}
void deletegroup(Item *p){
}
void writegroup(Item *ip, int f){
	if(!goodgroup(ip->group)) return;
	fprint(f, "g %d %.3f %.3f\n", grpmap[ip->group], ip->p[0].x, ip->p[0].y);
}
void activategroup(Item *ip){
	msg("activate called on group");	/* uses walk */
}
int inboxgroup(Item *ip, Drectangle r){
	msg("inbox called on group");		/* uses walk */
	return 0;
}
Drectangle bboxgroup(Item *ip){
	if(goodgroup(ip->group))
		return draddp((*group[ip->group]->fn->bbox)(group[ip->group]), ip->p[0]);
	return Drpt(ip->p[0], ip->p[0]);
}
Dpoint nearvertgroup(Item *ip, Dpoint testp){
	msg("nearvert called on group");
	return testp;
}
Itemfns groupfns={
	deletegroup,
	writegroup,
	activategroup,
	neargroup,
	drawgroup,
	editgroup,
	translategroup,
	inboxgroup,
	bboxgroup,
	nearvertgroup,
};
Item *addgroup(Item *head, int group, Dpoint offs){
	return additem(head, GROUP, 0., 0, 0, group, &groupfns, 1, offs);
}
/*
 * Is n the index of a usable group?
 */
int goodgroup(int n){
	return 0<=n && n<=NGROUP && group[n];
}
