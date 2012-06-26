#include "lib.h"
#include <sys/stat.h>
#include <fcntl.h>

int
creat(const char *name, mode_t mode)
{
	int n;

	n = open(name, O_WRONLY | O_CREAT | O_TRUNC, mode);
	/* no need to _syserrno; open did it already */
	return n;
}
