/*
 * code to print in the message area and to die horrible deaths (with a message)
 */
#include "art.h"
void fatal(char *m){
	fprint(2, "fig: %s\n", m);
	exits(m);
}
#define	NPREV	512
char prevmsg[NPREV];
void lastmsg(void){
	Point p=string(&screen, msgbox.min, font, prevmsg, S);
	rectf(&screen, Rect(p.x, msgbox.min.y, msgbox.max.x, msgbox.max.y), Zero);
}
void msg(char *fmt, ...){
	doprint(prevmsg, prevmsg+NPREV, fmt, &fmt+1);
	lastmsg();
}
void echo(char *m){
	Point p;
	p=string(&screen, echobox.min, font, m, S);
	rectf(&screen, Rect(p.x, echobox.min.y, echobox.max.x, echobox.max.y), Zero);
}
