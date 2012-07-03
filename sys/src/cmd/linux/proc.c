#include <u.h>
#include <libc.h>

#include "fns.h"
#include "dat.h"

int sys_set_tid_address(int *tidptr)
{
	uproc.cleartidptr = tidptr;
	return uproc.pid;
}
