#include "lib9.h"

void
oserrstr(char *buf, uint nerr)
{
	*buf = 0;
	errstr(buf, nerr);
}
