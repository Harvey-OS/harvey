#include "lib.h"
#include <unistd.h>
#include "sys9.h"

pid_t
setsid(void)
{
	if(_RFORK(FORKNSG|FORKNTG) < 0){
		_syserrno();
		return -1;
	}
	_sessleader = 1;
	return getpgrp();
}
