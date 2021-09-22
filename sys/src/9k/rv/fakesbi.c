/*
 * fake interface to sbi (bios/firmware) for rv64
 */
#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"

vlong
sbicall(uvlong, uvlong, uvlong, struct Sbiret *, uvlong *)
{
	return -1;
}
