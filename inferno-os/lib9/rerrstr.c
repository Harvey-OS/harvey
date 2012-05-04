#include "lib9.h"

void
rerrstr(char *buf, uint nbuf)
{
	char tmp[ERRMAX];

	tmp[0] = 0;
	errstr(tmp, sizeof tmp);
	utfecpy(buf, buf+nbuf, tmp);
	errstr(tmp, sizeof tmp);
}
