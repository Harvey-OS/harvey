#include <u.h>
#include <libc.h>
#include <auth.h>
#include <fcall.h>
#include <thread.h>
#include <9p.h>
#include "flashfs.h"

static void
usage(void)
{
	fprint(2, "usage: %s [-n nsect] [-z sectsize] file\n", argv0);
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
	ulong i;
	int m, n;
	char *file;
	uchar hdr[MAXHDR];

	ARGBEGIN {
	case 'n':
		nsects = argval(ARGF());
		break;
	case 'z':
		sectsize = argval(ARGF());
		break;
	default:
		usage();
	} ARGEND

	if(argc != 1)
		usage();
	file = argv[0];

	sectbuff = emalloc9p(sectsize);
	initdata(file, 1);

	memmove(hdr, magic, MAGSIZE);
	m = putc3(&hdr[MAGSIZE], 0);
	n = putc3(&hdr[MAGSIZE + m], 0);
	clearsect(0);
	writedata(0, 0, hdr, MAGSIZE + m + n, 0);

	for(i = 1; i < nsects - 1; i++)
		clearsect(i);

	m = putc3(&hdr[MAGSIZE], 1);
	n = putc3(&hdr[MAGSIZE + m], 0);
	clearsect(nsects - 1);
	writedata(0, nsects - 1, hdr, MAGSIZE + m + n, 0);
}
