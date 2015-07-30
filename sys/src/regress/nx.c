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
	void (*f)(void);
	if (notify(handler)){
		fprint(2, "%r\n");
		exits("notify fails");
	}

	uint8_t *ret = malloc(1);
	*ret = RET;
	f = (void *)ret;
	f();
	print("FAIL");
	exits("FAIL");
}
