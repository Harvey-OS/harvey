#include <u.h>
#include <libc.h>
#include <bio.h>

void
main(int argc, char *argv[])
{
	Biobuf bin;
	int i;
	int nflag;

	Binit(&bin, 1, OWRITE);
	nflag = 0;
	if(argc>1 && strcmp(argv[1], "-n")==0)
		nflag = 1;
	for(i=1+nflag; i<argc; i++)
		Bprint(&bin, i==argc-1? "%s" : "%s ", argv[i]);
	if(!nflag)
		Bprint(&bin, "\n");
	Bclose(&bin);
	exits((char *)0);
}
