#include <u.h>
#include <libc.h>
#define RET 0xc3

void
handler(void *v, char *s)
{
	double x = 1.0;
	x = x * 100.3;
	print("PASS\n");
	exits("PASS");
}

void
main(void)
{
	void (*f)(void) = nil;
	if (notify(handler)){
		fprint(2, "%r\n");
		exits("notify fails");
	}
	double y = 47.1;
	f();
	print("50=%f\n", y + 2.9);
	print("FAIL");
	exits("FAIL");
}
