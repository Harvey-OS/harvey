#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>

/*
 * No such concept in plan9, but supposed to be always successful
 */

mode_t
umask(mode_t)
{
	return 0;
}
