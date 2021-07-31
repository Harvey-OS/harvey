#include <u.h>
#include <libc.h>

void
main(void)
{
	char *p;
	int d;

	p = malloc(1);
	d = msize(p);
	print("d=%d\n", d);
	memset(p, 0, d);
	free(p);
}
