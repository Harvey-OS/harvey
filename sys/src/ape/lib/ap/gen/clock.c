#include <time.h>
#include <sys/times.h>

clock_t
clock(void)
{
	struct tms t;

	if(times(&t) == (clock_t)-1)
		return (clock_t)-1;
	return t.tms_utime+t.tms_stime;
}
