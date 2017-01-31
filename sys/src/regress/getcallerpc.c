#include <u.h>
#include <libc.h>

/*
 * Do nothing. We can't exits from this one. It tests correctness of programs
 * that just return.
 */

void c(void);
void b(void);

void a(void)
{
	uintptr_t d = getcallerpc();
	fprint(2, "This should print something more than %p and less than %p: %p\n",
		b, c, d);
	if ((d > b) && (d < c)) {
		print("PASS\n");
		exits("PASS");
		return;
	}
	print("FAIL\n");
	exits("FAIL");
}

void b(void)
{
	a();
}

void c(void)
{
	b();
}

void
main(int argc, char *argv[])
{
	c();
}
