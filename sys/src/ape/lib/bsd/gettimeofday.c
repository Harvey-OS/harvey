#include <sys/types.h>
#include <time.h>
#include <sys/time.h>

int
gettimeofday(struct timeval *tp, struct timezone *tzp)
{
	tp->tv_sec = time(0);
	tp->tv_usec = 0;

	if(tzp) {
		tzp->tz_minuteswest = 240;
		tzp->tz_dsttime = 1;
	}

	return 0;
}
