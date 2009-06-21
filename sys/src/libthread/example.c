/*
Threadmain spawns two subprocesses, one
to read the mouse, and one to receive
timer events.  The events are sent via a
channel to the main proc which prints a
word when an event comes in.  When mouse
button three is pressed, the application
terminates.
*/

#include <u.h>
#include <libc.h>
#include <thread.h>

enum
{
	STACK = 2048,
};

void
mouseproc(void *arg)
{
	char m[48];
	int mfd;
	Channel *mc;

	mc = arg;
	if((mfd = open("/dev/mouse", OREAD)) < 0)
		sysfatal("open /dev/mouse: %r");
	for(;;){
		if(read(mfd, m, sizeof m) != sizeof m)
			sysfatal("eof");
		if(atoi(m+1+2*12)&4)
			sysfatal("button 3");
		send(mc, m);
	}
}

void
clockproc(void *arg)
{
	int t;
	Channel *c;

	c = arg;
	for(t=0;; t++){
		sleep(1000);
		sendul(c, t);
	}
}


void
threadmain(int argc, char *argv[])
{
	char m[48];
	int t;
	Alt a[] = {
	/*	 c		v		op   */
		{nil,	m,	CHANRCV},
		{nil,	&t,	CHANRCV},
		{nil,	nil,	CHANEND},
	};

	/* create mouse event channel and mouse process */
	a[0].c = chancreate(sizeof m, 0);
	proccreate(mouseproc, a[0].c, STACK);

	/* create clock event channel and clock process */
	a[1].c = chancreate(sizeof(ulong), 0);	/* clock event channel */
	proccreate(clockproc, a[1].c, STACK);

	for(;;){
		switch(alt(a)){
		case 0:	/*mouse event */
			fprint(2, "click ");
			break;
		case 1:	/* clock event */
			fprint(2, "tic ");
			break;
		default:
			sysfatal("can't happen");
		}
	}
}
