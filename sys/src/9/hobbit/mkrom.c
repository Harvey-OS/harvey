#include <u.h>
#include <libc.h>
#include <bio.h>
#include <mach.h>

static Biobuf bip, *bop;

static void
usage(void)
{
	fprint(2, "%s: usage: %s file\n", argv0, argv0);
	exits("usage");
}

static void
segment(long offset, long nbytes)
{
	int b0, b1, b2, b3;

	Bseek(&bip, offset, 0);
	while(nbytes > 0){
		if((b0 = Bgetc(&bip)) == Beof)
			break;
		nbytes--;
		if((b1 = Bgetc(&bip)) == Beof)
			break;
		nbytes--;
		if((b2 = Bgetc(&bip)) == Beof)
			break;
		nbytes--;
		if((b3 = Bgetc(&bip)) == Beof)
			break;
		nbytes--;
		BPUTC(bop, b2);
		BPUTC(bop, b3);
		BPUTC(bop, b0);
		BPUTC(bop, b1);
	}
	if(nbytes)
		fprint(2, "%s: warning: %d bytes left\n", argv0, nbytes);
}

void
main(int argc, char *argv[])
{
	int fd, i;
	Fhdr f;
	char *com;

	ARGBEGIN{
		USED(_argt);

	default:
		usage();
	}ARGEND
	if(argc != 1)
		usage();
	if((fd = open(argv[0], OREAD)) == 0){
		fprint(2, "%s: can't open %s - %r\n", argv0, argv[0]);
		exits("open");
	}
	if(crackhdr(fd, &f) == 0){
		fprint(2, "%s: unknown object type\n", argv0);
		exits("header");
	}
	if(Binit(&bip, fd, OREAD) == Beof){
		fprint(2, "%s: Binit %s\n", argv0, argv[0]);
		exits("Binit");
	}
	if((com = malloc(strlen(argv[0])+5)) == 0){
		fprint(2, "%s: can't malloc for %s.com\n", argv0, argv[0]);
		exits("malloc");
	}
	sprint(com, "%s.com", argv[0]);
	if((bop = Bopen(com, OWRITE)) == 0){
		fprint(2, "%s: can't create %s - %r\n", argv0, argv[0]);
		exits("create");
	}
	switch(f.type){

	case FMIPS:
	case FSPARC:
	case F68020:
	case FI386:
	case FI960:
	case FHOBBIT:
		f.txtoff = 32;
		f.txtsz -= 32;
		break;
	}
	segment(f.txtoff, f.txtsz);
	segment(f.datoff, f.datsz);
	for(i = ((f.txtsz+f.datsz)%4); i; i--)
		BPUTC(bop, 0);
	exits(0);
}
