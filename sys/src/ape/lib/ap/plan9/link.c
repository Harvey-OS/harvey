#include <unistd.h>
#include <errno.h>

/*
 * BUG: LINK_MAX==1 isn't really allowed
 */
int
link(const char *, const char *)
{
	errno = EMLINK;
	return -1;
}
