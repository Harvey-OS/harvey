#include	<alef.h>

byte	*argv0;

void
fatal(byte *fmt, ...)
{
	byte buf[1024], *p;

	p = doprint(buf, buf+sizeof(buf), fmt, ...);
	*p = 0;
	fprint(2, "%s: %s\n", argv0, buf);
	exits(buf);
}

void
main(int argc, byte **argv)
{
	argv0 = argv[0];

	fatal("nargs = %d", argc);
}
