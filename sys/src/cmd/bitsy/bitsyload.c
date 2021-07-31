#include <u.h>
#include <libc.h>

enum
{
	Magic= 0x5a5abeef,
};

typedef struct Method Method;
struct Method {
	char *verb;
	char *part;
	char *file;
};

Method method[] =
{
	{ "k", "/dev/flash/kernel", "/sys/src/9/bitsy/9bitsy" },
	{ "r", "/dev/flash/ramdisk", "/sys/src/9/bitsy/paqdisk" },
	{ "u", "/dev/flash/user", "/sys/src/9/bitsy/user" },
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
	char ctlfile[64];
	char ctldata[1024];
	char *args[10];
	Dir *d;
	ulong nsects, sectsize;
	Method *m;

	ARGBEGIN {
	} ARGEND;

	if(argc == 0)
		usage();

	for(m = method; m < method + nelem(method); m++)
		if(strcmp(argv[0], m->verb) == 0)
			break;
	if(m == method + nelem(method))
		usage();

	if(argc < 2)
		file = m->file;
	else
		file = argv[1];

	in = open(file, OREAD);
	if(in < 0)
		sysfatal("%s: %r", file);
	d = dirfstat(in);
	if(d == nil)
		sysfatal("stating %s: %r", file);

	out = open(m->part, OWRITE);
	if(out < 0)
		sysfatal("%s: %r", m->part);
	sprint(ctlfile, "%sctl", m->part);
	ctl = open(ctlfile, ORDWR);
	if(ctl < 0)
		sysfatal("%s: %r", ctlfile);

	i = read(ctl, ctldata, sizeof(ctldata) - 1);
	if (i <= 0)
		sysfatal("%s: %r", ctlfile);
	ctldata[i] = '\0';
	i = tokenize(ctldata, args, nelem(args));
	if(i < 7)
		sysfatal("bad flash geometry");

	nsects = atoi(args[5]);
	sectsize = atoi(args[6]);
	if(nsects < 3)
		sysfatal("unreasonable value for nsects: %lud", nsects);
	if(sectsize < 512)
		sysfatal("unreasonable value for sectsize: %lud", sectsize);

	/* allocate a buffer and read in the whole file */
	n = d->length;
	buf = malloc(n + sectsize);
	if(buf == nil)
		sysfatal("not enough room to read in file: %r");

	print("reading %d bytes of %s\n", n, file);
	if(readn(in, buf, n) != n)
		sysfatal("error reading file: %r");
	close(in);

	memset(buf + n, 0, sectsize);
	n = ((n+sectsize-1)/sectsize)*sectsize;

	if (nsects * sectsize < n)
		sysfatal("file too large (%d) for partition (%lud)", n, nsects * sectsize);

	print("erasing %s\n", m->part);
	if(fprint(ctl, "erase") < 0)
		sysfatal("erase %s: %r", ctlfile);
	close(ctl);
	print("writing %s\n", m->part);

	for (i = 0; i < n; i += sectsize)
		if(write(out, buf + i, sectsize) != sectsize)
			sysfatal("writing %s at %d: %r", file, i);

	print("write done\n");
	close(in);
	close(out);
}
