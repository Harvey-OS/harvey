#include <u.h>
#include <libc.h>

#include "dat.h"
#include "fns.h"

int sys_set_tid_address(int *tidptr)
{
	uproc.cleartidptr = tidptr;
	return uproc.pid;
}
