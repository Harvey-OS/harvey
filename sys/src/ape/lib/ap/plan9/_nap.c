#include "lib.h"
#include <unistd.h>
#include <time.h>
#include "sys9.h"

/*
 * This is an extension to POSIX
 */
unsigned int
_nap(unsigned int millisecs)
{
	time_t t0, t1;

	t0 = time(0);
	if(_SLEEP(millisecs) < 0){
		t1 = time(0);
		return t1-t0;
	}
	return 0;
}
