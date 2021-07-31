#include <u.h>
#include <libc.h>
#include <libg.h>
#include <panel.h>
Panel *root, *list;
char *genlist(Panel *, int which){
	static char buf[7];
	if(which<0 || 26<=which) return 0;
	sprint(buf, "item %c", which+'a');
	return buf;
}
void hitgen(Panel *p, int buttons, int sel){
	USED(p, buttons, sel);
}
void ereshaped(Rectangle r){
	screen.r=r;
	r=inset(r, 4);
	plpack(root, r);
	bitblt(&screen, screen.r.min, &screen, screen.r, Zero);
	pldraw(root, &screen);
}
void done(Panel *p, int buttons){
	USED(p, buttons);
	bitblt(&screen, screen.r.min, &screen, screen.r, Zero);
	exits(0);
}
Panel *msg;
void message(char *s, ...){
	char buf[1024], *out;
	out = doprint(buf, buf+sizeof(buf), s, (&s+1));
	*out='\0';
	plinitlabel(msg, PACKN|FILLX, buf);
	pldraw(msg, &screen);
}
Scroll s;
void save(Panel *p, int buttons){
	USED(p, buttons);
	s=plgetscroll(list);
	message("save %d %d %d %d", s);
}
void revert(Panel *p, int buttons){
	USED(p, buttons);
	plsetscroll(list, s, &screen);
	message("revert %d %d %d %d", s);
}
void main(void){
	Panel *g;
	binit(0,0,0);
	einit(Emouse);
	plinit(screen.ldepth);
	root=plgroup(0, 0);
	g=plgroup(root, PACKN|EXPAND);
	list=pllist(g, PACKE|EXPAND, genlist, 8, hitgen);
	plscroll(list, 0, plscrollbar(g, PACKW));
	msg=pllabel(root, PACKN|FILLX, "");
	plbutton(root, PACKW, "save", save);
	plbutton(root, PACKW, "revert", revert);
	plbutton(root, PACKE, "done", done);
	ereshaped(screen.r);
	for(;;) plmouse(root, emouse(), &screen);
}
