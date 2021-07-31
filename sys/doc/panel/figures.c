#include <u.h>
#include <libc.h>
#include <libg.h>
#include <panel.h>
#include <fb.h>
#define	LDEPTH	3
typedef struct Figure Figure;
struct Figure{
	char *name;
	void (*fn)(void);
};
Panel *root;
void hello(void){
#include "hello.eg"
}
void pack1(void){
#include "pack1.eg"
}
void pack2(void){
#include "pack2.eg"
}
void pack3(void){
#include "pack3.eg"
}
void pack4(void){
#include "pack4.eg"
}
void pack5(void){
#include "pack5.eg"
}
void pack6(void){
#include "pack6.eg"
}
void button(void){
#include "button.eg"
}
void message(void){
#include "message.eg"
}
#include "slider.eg"
#include "scroll.eg"
void entry(void){
#include "entry.eg"
}
#include "popup.neg"
#include "mbar.neg"
void cascade(void){
#include "cascade.neg"
}
Figure figure[]={
	"hello",	hello,
	"pack1",	pack1,
	"pack2",	pack2,
	"pack3",	pack3,
	"pack4",	pack4,
	"pack5",	pack5,
	"pack6",	pack6,
	"button",	button,
	"message",	message,
	"slider",	slider,
	"scroll",	scroll,
	"entry",	entry,
	"popup",	popup,
	"mbar",		mbar,
	"cascade",	cascade,
	0,		0
};
void ereshaped(Rectangle r){
	USED(r);
	exits("reshape!");
}
void main(int argc, char *argv[]){
	Figure *p;
	Bitmap *b;
	PICFILE *out;
	Mouse m;
	if(getflags(argc, argv, "h:3[b x y]")!=2){
	Usage:
		fprint(2, "Examples are:\n");
		for(p=figure;p->name;p++) fprint(2, "\t%s\n", p->name);
		usage("example");
	}
	for(p=figure;p->name;p++) if(strcmp(p->name, argv[1])==0) break;
	if(p->name==0) goto Usage;
	binit(0, 0, 0);
	einit(Emouse);
	plinit(LDEPTH);
	p->fn();
	if(!plpack(root, Rect(0,0,300,200))){
		fprint(2, "%s: pack fails r=%d %d %d %d\n", argv[0],
			root->r.min.x, root->r.min.y, root->r.max.x, root->r.max.y);
		exits("bad pack");
	}
	b=balloc(root->r, LDEPTH);
	bitblt(b, root->r.min, b, root->r, Zero);
	pldraw(root, b);
	if(flag['h']){
		m.buttons=1<<(atoi(flag['h'][0])-1);
		m.xy.x=atoi(flag['h'][1]);
		m.xy.y=atoi(flag['h'][2]);
		plmouse(root, m);
	}
	out=picopen_w("OUT", "runcode", root->r.min.x, root->r.min.y,
		root->r.max.x-root->r.min.x, root->r.max.y-root->r.min.y, "m", argv, 0);
	if(out==0){
		fprint(2, "%s: can't picopen_w OUT\n", argv[0]);
		exits("bad picopen_w");
	}
	wrpicfile(out, b);
	exits(0);
}
