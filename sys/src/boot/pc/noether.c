#include "u.h"
#include "lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "io.h"

int
etherinit(void)
{
	return -1;
}

void
etherinitdev(int, char*)
{
}

void
etherprintdevs(int)
{
}

int
etherrxpkt(int, Etherpkt*, int)
{
	return -1;
}

int
ethertxpkt(int, Etherpkt*, int, int)
{
	return -1;
}

int
bootpboot(int, char*, Boot*)
{
	return -1;
}

void*
pxegetfspart(int, char*, int)
{
	return nil;
}
