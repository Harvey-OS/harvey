#include "u.h"
#include "lib.h"
#include "dat.h"
#include "fns.h"
#include "error.h"

Proc *up;
Conf conf = 
{
	1,
	100,
	0,
	1024*1024*1024,
	1024*1024*1024,
	1024*1024*1024,
	1024*1024*1024,
	1024*1024*1024,
	1024*1024*1024,
	1024*1024*1024,
	1024*1024*1024,
	1024*1024*1024,
	1024*1024*1024,
	1024*1024*1024,
	1024*1024*1024,
	0,
};

char *eve = "eve";
ulong kerndate;
int cpuserver;
char hostdomain[] = "drawterm.net";
