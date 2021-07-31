#include	<u.h>
#include	<libc.h>

/* 
 *  we combine rand() and srand() with truerand() to provide a higher
 *  rate random number generator.
 */
ulong
fastrand(void)
{
	static ulong loops;

	if((loops++ % 1001) == 0)
		srand(truerand());
	return rand();
}
