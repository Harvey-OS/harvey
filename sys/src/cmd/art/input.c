/*
 * Low level input
 */
#include "art.h"
Mouse mouse;
/*
 * Get a new mouse position
 * Process any intervening typein, as a side-effect
 */
void getmouse(void){
	Event e;
	for(;;){
		switch(event(&e)){
		case Emouse:
			mouse=e.mouse;
			return;
		case Ekeyboard:
			typein(e.kbdc);
			break;
		}
	}
}
int button1(void){
	return mouse.buttons&1;
}
int button2(void){
	return mouse.buttons&2;
}
int button3(void){
	return mouse.buttons&4;
}
int button123(void){
	return mouse.buttons&7;
}
int button23(void){
	return mouse.buttons&6;
}
int button(int n){
	return mouse.buttons&(1<<(n-1));
}
