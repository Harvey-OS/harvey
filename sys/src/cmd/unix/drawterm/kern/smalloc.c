#include "u.h"
#include "lib.h"
#include "dat.h"
#include "fns.h"
#include "error.h"

void*
smalloc(ulong n)
{
	return mallocz(n, 1);
}

void*
malloc(ulong n)
{
	return mallocz(n, 1);
}

