#include <u.h>
#include <libc.h>
#include <auth.h>
#include <fcall.h>
#include <thread.h>
#include <9p.h>
#include "flashfs.h"

extern int	chatty9p;

static void
usage(void)
{
	fprint(2, "usage: %s [-rD] [-n nsect] [-z sectsize] [-m mount] [-f file]\n", argv0);
	exits("usage");
}

static ulong
argval(char *arg)
{
	long v;
	char *extra;

	if(arg == nil)
		usage();
	v = strtol(arg, &extra, 0);
	if(*extra || v <= 0)
		usage();
	return v;
}

void
main(int argc, char **argv)
{
	int ro;
	char *file, *mount;

	mount = "/n/brzr";
	ro = 0;
	file = "/dev/flash/fs";

	ARGBEGIN {
	case 'D':
		chatty9p++;
		break;
	case 'r':
		ro++;
		break;
	case 'n':
		nsects = argval(ARGF());
		break;
	case 'z':
		sectsize = argval(ARGF());
		break;
	case 'f':
		file = ARGF();
		break;
	case 'm':
		mount = ARGF();
		break;
	default:
		usage();
	} ARGEND

	if(argc != 0)
		usage();

	initdata(file, 0);
	sectbuff = emalloc9p(sectsize);
	einit();
	loadfs(ro);
	serve(mount);
	exits(nil);
}
