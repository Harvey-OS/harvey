/*
 * code to print in the message area and to die horrible deaths (with a message)
 */
#include "art.h"
void fatal(char *m){
	fprint(2, "fig: %s\n", m);
	exits(m);
}
void msg(char *fmt, ...){
	char buf[512];
	Point p;
	doprint(buf, buf+512, fmt, &fmt+1);
	p=string(&screen, msgbox.min, font, buf, S);
	rectf(&screen, Rect(p.x, msgbox.min.y, msgbox.max.x, msgbox.max.y), Zero);
}
void echo(char *m){
	Point p;
	p=string(&screen, echobox.min, font, m, S);
	rectf(&screen, Rect(p.x, echobox.min.y, echobox.max.x, echobox.max.y), Zero);
}
