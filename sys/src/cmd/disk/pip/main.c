#include	"all.h"

enum
{
	Cload		= 0,	/* commands */
	Cverif,
	Cstore,
	Ctoc,
	Ccleartoc,
	Cremove,
	Cpublish,
	Csession,
	Cfixate,
	C9660,
	Csum,
	Chelp,
	Cend,
};

char*	c1cmd[] =
{
	[Cload*2]	"load",		"fromdevice fromtrack format disktrack -",
	[Cverif*2]	"verif",	"fromdevice fromtrack format disktrack -",
	[Cstore*2]	"store",	"disktrack tofile -",
	[Ctoc*2]	"toc",		"[device] -",
	[Ccleartoc*2]	"cleartoc",	"- clear table of contents",
	[Cremove*2]	"remove",	"disktrack - clear toc entry",
	[Cpublish*2]	"publish",	"disktrack -",
	[Csession*2]	"session",	"format - fix cd and start new session",
	[Cfixate*2]	"fixate",	"format - fix cd",
	[C9660*2]	"9660",		"protofile disktrack -",
	[Csum*2]	"sum",		"disktrack -",
	[Chelp*2]	"help",
	[Cend*2]	0
};

void
main(int argc, char *argv[])
{
	int f, i;

	ARGBEGIN {
	default:
		fprint(2, "usage: pip [ifiles]\n");
		exits("usage");
	} ARGEND

	if(dopen())
		exits("toc");
	for(i=0; i<argc; i++) {
		f = open(argv[i], OREAD);
		if(f < 0) {
			fprint(2, "cant open %s: %r\n", argv[i]);
			exits("open");
		}
		input(f);
	}
	if(argc == 0)
		input(0);
	exits(0);
}

void
input(int f)
{
	int i;
	Binit(&bin, f, OREAD);

loop:
	while(!eol())
		if(getword())
			return;
	print("+ ");
	if(getword())
		return;

	for(i=0; i<Cend; i++)
		if(c1cmd[i*2])
			if(strcmp(c1cmd[i*2], word) == 0)
				break;

	switch(i) {
	default:
		print("cant find command %s\n", word);
		break;
	case Cload:
		doload();
		break;
	case Cverif:
		doverif();
		break;
	case Cstore:
		dstore();
		break;
	case Ctoc:
		dotoc();
		break;
	case Ccleartoc:
		dcleartoc();
		break;
	case Cremove:
		doremove();
		break;
	case Cpublish:
		dpublish();
		break;
	case Csession:
		psession();
		break;
	case Cfixate:
		pfixate();
		break;
	case C9660:
		mk9660();
		break;
	case Csum:
		dsum();
		break;
	case Chelp:
		for(i=0; c1cmd[i]; i+=2)
			if(c1cmd[i+1])
				print("%s %s\n", c1cmd[i], c1cmd[i+1]);
			else
				print("%s\n", c1cmd[i]);
		break;
	}
	goto loop;
}

void
doload(void)
{
	int from;

	from = getdev("from [tosh/nec/plex/phil/file]");
	switch(from) {
	default:
		print("illegal device\n");
		break;
	case Dnec:
		nload();
		break;
	case Dtosh:
		tload();
		break;
	case Dplex:
		xload();
		break;
	case Dphil:
		pload();
		break;
	case Dfile:
		fload();
		break;
	}
}

void
doverif(void)
{
	int from;

	from = getdev("from [tosh/plex]");
	switch(from) {
	default:
		print("illegal device\n");
		break;
	case Dtosh:
		tverif();
		break;
	case Dplex:
		xverif();
		break;
	}
}

void
dotoc(void)
{
	int dev;

	dev = Ddisk;
	if(!eol())
		dev = getdev("from");
	switch(dev) {
	default:
		print("illegal device\n");
		break;
	case Ddisk:
		dtoc();
		break;
	case Dtosh:
		ttoc();
		break;
	case Dplex:
		xtoc();
		break;
	case Dnec:
		ntoc();
		break;
	case Dphil:
		ptoc();
		break;
	}
}

void
doremove(void)
{
	int track;

	track = gettrack("disk track [number]/*");
	if(track < 0 || track >= Ntrack) {
		if(track != Trackall) {
			print("from track out of range %d-%d\n", 0, Ntrack);
			return;
		}
	}
	dclearentry(track, 1);
}
