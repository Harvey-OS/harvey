#include <u.h>
#include <libc.h>
#include <thread.h>
#include <worker.h>

void
f(Worker*, void *arg)
{
	print("%d\n", (int)arg);
}

#define MS (1000000LL)
#define S (1000000000LL)

void
threadmain(int, char **)
{
	timerdispatch(f, (void*)2, nsec()+2*S);
	timerdispatch(f, (void*)1, nsec()+S);
	timerdispatch(f, (void*)3, nsec()+3*S);
	timerdispatch(f, (void*)5, nsec()+5*S);
	timerdispatch(f, (void*)4, nsec()+4*S);
	timerrecall(f, (void*)3);
	threadexits(0);
}
