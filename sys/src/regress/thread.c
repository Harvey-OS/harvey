
#include <u.h>
#include <libc.h>
#include <thread.h>

enum { STACK = 2048,
};

void
clockproc(void* arg)
{
	int t;
	Channel* c;

	c = arg;
	for(t = 0;; t++) {
		sleep(100);
		sendul(c, t);
	}
}

void
threadmain(int argc, char* argv[])
{
	int tick, tock;
	Alt a[] = {
	    {nil, &tick, CHANRCV}, {nil, &tock, CHANRCV}, {nil, nil, CHANEND},
	};

	fprint(2, "threadmain hi!\n");

	a[0].c = chancreate(sizeof tick, 0);
	a[1].c = chancreate(sizeof tock, 0);

	proccreate(clockproc, a[0].c, STACK);
	proccreate(clockproc, a[1].c, STACK);

	tick = 0;
	tock = 0;
	while(tick < 10 || tock < 10) {
		switch(alt(a)) {
		case 0:
			fprint(2, "tick %d\n", tick);
			break;
		case 1:
			fprint(2, "tock %d\n", tock);
			break;
		default:
			sysfatal("can't happen");
		}
	}

	print("PASS\n");
	exits("PASS");
}