#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"../port/error.h"
#include	"ip.h"

/*
 *  some hacks for commonality twixt inferno and plan9
 */

char*
commonuser(void)
{
	return up->env->user;
}

Chan*
commonfdtochan(int fd, int mode, int a, int b)
{
	return fdtochan(up->env->fgrp, fd, mode, a, b);
}

char*
commonerror(void)
{
	return up->env->error;
}

int
postnote(Proc*, char*)
{
	return -1;
}
