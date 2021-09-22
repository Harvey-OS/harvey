#include "common.h"

void
main(int argc, char **argv)
{
	String *mbox;
	Mlock *l;

	ARGBEGIN{
	}ARGEND;

	mbox = mboxpath("mbox", getlog(), s_new(), 0);
	if(mbox == nil)
		sysfatal("internal error");
	l = trylock(s_to_c(mbox));
	if(l == nil)
		sysfatal("locking mail box: %r");
	switch(fork()){
	case -1:
		sysunlock(l);
		sysfatal("forking: %r");
		break;
	case 0:
		for(;;)
			sleep(10000);
		sysunlock(l);
		break;
	}
	exits(0);
}
