#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"

#include <ctype.h>

int
hexvalc(int c)
{
	if (!isxdigit(c))
		return 0;
	else if (isdigit(c))
		return c - '0';
	else
		return c - (islower(c)? 'a': 'A') + 10;
}
