#include	"l.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/times.h>
#undef getwd
#include <unistd.h>	/* For sysconf() and _SC_CLK_TCK */

void*
mysbrk(usize size)
{
	return (void*)sbrk(size);
}

double
cputime(void)
{

	struct tms tmbuf;
	double	ret_val;

	/*
	 * times() only fails if &tmbuf is invalid.
	 */
	(void)times(&tmbuf);
	/*
	 * Return the total time (in system clock ticks)
	 * spent in user code and system
	 * calls by both the calling process and its children.
	 */
	ret_val = (double)(tmbuf.tms_utime + tmbuf.tms_stime +
			tmbuf.tms_cutime + tmbuf.tms_cstime);
	/*
	 * Convert to seconds.
	 */
	ret_val *= sysconf(_SC_CLK_TCK);
	return ret_val;
	
}

int
fileexists(char *name)
{
	struct stat sb;

	return stat(name, &sb) >= 0;
}
