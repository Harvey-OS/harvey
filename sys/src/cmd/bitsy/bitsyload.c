#include <u.h>
#include <libc.h>

enum
{
	Magic= 0x5a5abeef,
};

struct {
	char *verb;
	char *part;
	char *file;
	int hdr;
}method[] =
{
	{ "k", "/dev/flash/kernel", "/sys/src/9/bitsy/9bitsy", 1 },
	{ "r", "/dev/flash/ramdisk", "/sys/src/9/bitsy/ramdisk", 0 },
};

void
leputl(uchar *p, ulong x)
{
	*p++ = x;
	*p++ = x>>8;
	*p++ = x>>16;
	*p = x>>24;
}

void
usage(void)
{
	fprint(2, "usage: %s k|r [file]\n", argv0);
	exits("usage");	
}

void
main(int argc, char **argv)
{
	char *file;
	int ctl, in, out;
	int i, n;
	uchar *buf;
	uchar hdr[12];
	char ctlfile[64];
	Dir d;

	ARGBEGIN {
	} ARGEND;

	if(argc == 0)
		usage();

	for(i = 0; i < nelem(method); i++)
		if(strcmp(argv[0], method[i].verb) == 0)
			break;
	if(i == nelem(method))
		usage();
	if(argc < 2)
		file = method[i].file;
	else
		file = argv[1];

	in = open(file, OREAD);
	if(in < 0)
		sysfatal("%s: %r", file);
	if(dirfstat(in, &d) < 0)
		sysfatal("stating %s: %r", file);

	/* allocate a buffer and read in the whole file */
	n = d.length;
	buf = malloc(n+4);
	if(buf == nil)
		sysfatal("not enough room to read in file: %r");
	print("reading %d bytes of %s\n", n, file);
	if(readn(in, buf, n) != n)
		sysfatal("error reading file: %r");
	close(in);

	out = open(method[i].part, OWRITE);
	if(out < 0)
		sysfatal("%s: %r", method[i].part);
	sprint(ctlfile, "%sctl", method[i].part);
	ctl = open(ctlfile, OWRITE);
	if(ctl < 0)
		sysfatal("%s: %r", ctlfile);

	print("erasing %s\n", method[i].part);
	if(fprint(ctl, "erase") < 0)
		sysfatal("erase %s: %r", ctlfile);
	close(ctl);
	print("writing %s\n", method[i].part);

	switch(method[i].hdr){
	case 1:
		leputl(hdr, Magic);
		leputl(hdr+4, d.length);
		leputl(hdr+8, 0xc0000000);
		if(write(out, hdr, 12) != 12)
			sysfatal("writing header: %r");
		break;
	case 0:
		leputl(hdr, d.length);
		if(write(out, hdr, 4) != 4)
			sysfatal("writing header: %r");
		break;
	}

	if(n & 3)
		n += 4-(n&3);
	if(write(out, buf, n) != n)
		sysfatal("writing %s: %r", file);

	print("write done\n");
	close(in);
	close(out);
}
