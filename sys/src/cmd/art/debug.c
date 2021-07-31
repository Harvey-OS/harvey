/*
 * print debug messages
 */
#include "art.h"
void bug(char *fmt, ...){
	char buf[200];
	doprint(buf, buf+200, fmt, &fmt+1);
	msg("%s", buf);
	if(button123()){
		while(button123()) getmouse();
		while(!button123()) getmouse();
	}
	else{
		while(!button123()) getmouse();
		while(button123()) getmouse();
	}
}
