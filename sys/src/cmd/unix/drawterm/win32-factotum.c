#include <u.h>
#include <libc.h>
#include <auth.h>
#include <fcall.h>
#include <authsrv.h>
#include <libsec.h>
#include "drawterm.h"

#undef getenv

char*
getuser(void)
{
	return getenv("USER");
}

int
dialfactotum(void)
{
	return -1;
}

