#include <u.h>
#include <libc.h>
#include "system.h"
#include <libg.h>
#include <stdio.h>
#ifdef Plan9
#include "mouse.h"
#include "defines.h"
#include "object.h"
#include "njerq.h"


void ereshaped(Rectangle r)
{
	screen.r = r;
}
int
getpageno(void){
	int c, i=0;
	char buf[50];
	sprintf(buf,"page? ");
	string(bp,bp->r.min, font, "page? ",D|S);
	while(1){
		c=ekbd();
		if((char)c == '\n')return(i);
		if(c >= '0' && c <= '9')
			i = i * 10 + c - '0';
		sprintf(buf,"page? %d",i);
		string(bp, bp->r.min, font, buf, S);
	}
}
int
ckmouse(int must){
	int val = -1, control3=0;
	Mouse mouse;
	ulong nextevent;
	struct Event e;

	if(must == 2)cursorswitch(&the_end);
	else if(must == 4)cursorswitch(&deadmouse);
	else {
		cursorswitch(&cherries);
		control3=1;
	}
	do {
		nextevent = eread(Emouse|Ekeyboard, &e);
		if(nextevent == Ekeyboard){
			if(e.kbdc == '\n')
				val = 1;
			continue;
		}
		mouse = e.mouse;	
		if(!ptinrect(mouse.xy,screen.r)){
			while(!ptinrect(mouse.xy,screen.r)){
				mouse = emouse();
			}
			cursorswitch(&cherries);
		}
		if(button3()&&ptinrect(mouse.xy,screen.r)){
			if(control3)menu3t[0] = more;
			else menu3t[0] = pmore;
			if(must == 4)menu3t[1] = npage;
			else menu3t[1] = ppage;
			switch(menuhit(3, &mouse, &menu3)){
			case MORE:
				val = 1;
				break;
			case PAGE:
				page = getpageno();
				skipflag=1;
				val = 0;
				break;
			case DONE:
				cursorswitch(&bye);
			again:	mouse =emouse();
				if(button3()){
				switch(menuhit(3, &mouse, &menu3)){
				case MORE: val=1;
					break;
				case PAGE:
					page = getpageno();
					skipflag=1;
					val=0;
					break;
				case DONE:
					done(1);
				}
				}
				else goto again;
			}
		}
	} while(val == -1);
	cursorswitch((Cursor *)0);
	return(val);
}
#endif
void
done(int i)
{
	exits("psi error exit");
}
