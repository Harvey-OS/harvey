#include <u.h>
#include <libc.h>

void
main(void)
{
	int32_t x = 0;
	print("Acquire x %d\n", x);
	tsemacquire(&x, 5);
	print("Acquired x %d\n", x);
	tsemacquire(&x, 15);
	print("Acquired x %d did we time out?\n", x);
}
