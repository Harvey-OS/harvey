/*
 * Linux and BSD
 */
#include	"u.h"
#include	"lib.h"
#include	"dat.h"
#include	"fns.h"
#include	"error.h"
#include	"devaudio.h"

/* maybe this should return -1 instead of sysfatal */
void
audiodevopen(void)
{
	error("no audio support");
}

void
audiodevclose(void)
{
	error("no audio support");
}

void
audiodevsetvol(int what, int left, int right)
{
	error("no audio support");
}

void
audiodevgetvol(int what, int *left, int *right)
{
	error("no audio support");
}

