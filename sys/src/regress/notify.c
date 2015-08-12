#include <u.h>
#include <libc.h>
#define RET 0xc3

void
handler(void *v, char *s)
{
	print("PASS\n");
	exits("PASS");
}

void
main(void)
{
	void (*f)(void) = nil;
	if(notify(handler)) {
		fprint(2, "%r\n");
		exits("notify fails");
	}

	f();
	print("FAIL");
	exits("FAIL");
}
