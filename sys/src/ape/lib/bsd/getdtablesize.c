/* posix */
#include <sys/limits.h>

int
getdtablesize(void)
{
	return OPEN_MAX;
}
