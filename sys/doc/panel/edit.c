#include <u.h>
#include <libc.h>
#include <libg.h>
#include <panel.h>
Panel *root;
Panel *mkpanels(void);
void eventloop(void);
void main(void){
	binit(0, 0, 0);
	einit(Emouse|Ekeyboard);
	plinit(screen.ldepth);

	root=mkpanels();

	ereshaped(screen.r);

	eventloop();
}
void eventloop(void){
	Event e;
	for(;;){
		switch(event(&e)){
		case Ekeyboard:
			plkeyboard(e.kbdc);
			break;
		case Emouse:
			plmouse(root, e.mouse);
			break;
		}
	}
}
void ereshaped(Rectangle r){
	screen.r=r;
	plpack(root, r);
	bitblt(&screen, r.min, &screen, r, Zero);
	pldraw(root, &screen);
}
Rune text[]={
#include "from.macbeth"
};
#include "edit1.c"
#include "edit2.c"
#include "edit3.c"
#include "edit4.c"
