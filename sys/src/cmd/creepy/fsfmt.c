#include "all.h"

int
usrid(char*)
{
	return 3;
}

char*
usrname(int)
{
	return "sys";
}

int
member(int uid, int member)
{
	return uid == member;
}

int
allowed(int)
{
	return 1;
}

void
meltfids(void)
{
}

void
rwusers(Memblk*)
{
}

char*
ninestats(char *s, char*, int)
{
	return s;
}

char*
ixstats(char *s, char*, int)
{
	return s;
}

void
countfidrefs(void)
{
}

static void
usage(void)
{
	fprint(2, "usage: %s [-DFLAGS] [disk]\n", argv0);
	exits("usage");
}

void
threadmain(int argc, char *argv[])
{
	char *dev;
	int verb;

	dev = "disk";
	verb = 0;
	ARGBEGIN{
	case 'v':
		verb = 1;
		break;
	default:
		if((ARGC() >= 'A' && ARGC() <= 'Z') || ARGC() == '9'){
			dbg['d'] = 1;
			dbg[ARGC()] = 1;
			fatalaborts = 1;
		}else
			usage();
	}ARGEND;
	if(argc == 1)
		dev = argv[0];
	else if(argc > 0)
		usage();
	fmtinstall('H', mbfmt);
	fmtinstall('M', dirmodefmt);
	errinit(Errstack);
	if(catcherror())
		fatal("error: %r");
	fsfmt(dev);
	if(verb)
		fsdump(0, Mem);
	noerror();
	exits(nil);
}

