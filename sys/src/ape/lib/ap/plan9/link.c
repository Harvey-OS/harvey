#include <unistd.h>
#include <errno.h>

/*
 * BUG: LINK_MAX==1 isn't really allowed
 */
int
link(const char *name1, const char *name2)
{
	errno = EMLINK;
	return -1;
}
