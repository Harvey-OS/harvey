/*
 * dummy support for memory-type region registers.
 */
#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"

int
mtrr(uvlong, uvlong, char *)
{
	error("mtrr support excluded");
	return 0;
}

int
mtrrprint(char *, long)
{
	return 0;
}
