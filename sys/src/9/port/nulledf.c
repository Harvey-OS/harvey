#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"../port/error.h"
#include	"../port/edf.h"

static int
isedf(Proc*)
{
	return 0;
}

static void
edfbury(Proc*)
{
}

static int
edfanyready(void)
{
	return 0;
}

static void
edfready(Proc*)
{
}

static Proc*
edfrunproc(void)
{
	return nil;
}

static void
edfblock(Proc*)
{
}

static void
edfinit(void)
{
}

static void
edfexpel(Task*)
{
}

static char *
edfadmit(Task*)
{
	return "No edf";
}

static void
edfdeadline(Proc*)
{
}

Edfinterface nulledf = {
	.isedf		= isedf,
	.edfbury		= edfbury,
	.edfanyready	= edfanyready,
	.edfready		= edfready,
	.edfrunproc	= edfrunproc,
	.edfblock		= edfblock,
	.edfinit		= edfinit,
	.edfexpel		= edfexpel,
	.edfadmit		= edfadmit,
	.edfdeadline	= edfdeadline,
};
