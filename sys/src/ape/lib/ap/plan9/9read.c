#include "lib.h"
#include <string.h>
#include <stdlib.h>
#include "sys9.h"
#include "dir.h"

long
_READ(int fd, void *buf, long n)
{
	return _PREAD(fd, buf, n, -1LL);
}
