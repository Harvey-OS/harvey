#include "lib.h"
#include <string.h>
#include <stdlib.h>
#include "sys9.h"
#include "dir.h"

long
_WRITE(int fd, void *buf, long n)
{
	return _PWRITE(fd, buf, n, -1LL);
}
