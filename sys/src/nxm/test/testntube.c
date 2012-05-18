#include <u.h>
#include <libc.h>
#include <tube.h>

/*
 * NIX named tubes testing.
 !	6c -FTVw ntube.c && 6l -o 6.ntube ntube.6
 */

void
usage(void)
{
	fprint(2, "usage: %s\n", argv0);
	exits("usage");
}

void
main(int argc, char *argv[])
{
	Tube *t;
	char *name;

	name = argv[0];
	ARGBEGIN{
	default:
		usage();
	}ARGEND;
	if(argc == 1)
		name = "ntube";
	else if(argc != 0)
		usage();

	name = smprint("%s!test", name);
	t = namedtube(name, sizeof(ulong), 16, 1);
	if(t == nil)
		sysfatal("namedtube: %r");
	print("%s = %#p\n", name, t);
	exits(nil);
}
