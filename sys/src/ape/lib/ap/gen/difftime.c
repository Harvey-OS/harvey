#include <time.h>

/* Difference in seconds between two calendar times */

double
difftime(time_t t1, time_t t0)
{
	return (double)t1-(double)t0;
}
