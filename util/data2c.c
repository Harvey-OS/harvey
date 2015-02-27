#include <lib9.h>
#include <bio.h>

void
main(int argc, char *argv[])
{
	Biobuf bin, bout;
	long len;
	int n;
	uchar block[16], *c;

	if(argc != 2){
		fprint(2, "usage: data2s name\n");
		exits("usage");
	}
	/*
	 * No idea. Look at https://github.com/kabbi/wonderland
	 * setbinmode();
	 */
	Binit(&bin, 0, OREAD);
	Binit(&bout, 1, OWRITE);
	Bprint(&bout, "unsigned char %scode[] = {\n", argv[1]);
	for(len=0; (n=Bread(&bin, block, sizeof(block))) > 0; len += n){
		for(c=block; c < block+n; c++)
			if(*c)
				Bprint(&bout, "0x%ux,", *c);
			else
				Bprint(&bout, "0,");
		Bprint(&bout, "\n");
	}
	if(len == 0)
		Bprint(&bout, "0\n");	/* avoid empty initialiser */
	Bprint(&bout, "};\n");
	Bprint(&bout, "int %slen = %ld;\n", argv[1], len);
	exits(0);
}
