#include <u.h>
#include <libc.h>
#include <auth.h>
#include <fcall.h>
#include <thread.h>
#include <9p.h>
#include "flashfs.h"

int	chatty9p	= 0;

static void
usage(void)
{
	fprint(2, "usage: %s -m file\n", prog);
	exits("usage");
}

void
main(int argc, char **argv)
{
	char *file;

	prog = "dummyfs";
	file = "/n/brzr";
	limit = 100*1024;

	ARGBEGIN {
	case 'm':
		file = ARGF();
		break;
	default:
		usage();
	} ARGEND

	if(argc != 0)
		usage();

	einit();
	serve(file);
}
