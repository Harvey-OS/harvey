#include "lib.h"
#include <unistd.h>
#include <time.h>
#include "sys9.h"

unsigned int
sleep(unsigned int secs)
{
	time_t t0, t1;

	t0 = time(0);
	if(_SLEEP(secs*1000) < 0){
		t1 = time(0);
		return t1-t0;
	}
	return 0;
}
