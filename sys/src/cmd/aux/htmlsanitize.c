/*
 * Because sed can't handle really long lines.
 */
#include <u.h>
#include <libc.h>
#include <bio.h>

Biobuf bout;

void
cat(Biobuf *b)
{
	int c;

	while((c = Bgetrune(b)) >= 0){
		switch(c){
		case '<':
			Bprint(&bout, "&lt;");
			break;
		case '>':
			Bprint(&bout, "&gt;");
			break;
		case '&':
			Bprint(&bout, "&amp;");
			break;
		default:
			Bputrune(&bout, c);
			break;
		}
	}
}

void
main(int argc, char *argv[])
{
	int i;
	Biobuf *b, bin;

	argv0 = "htmlsanitize";
	Binit(&bout, 1, OWRITE);
	if(argc == 1){
		Binit(&bin, 0, OREAD);
		cat(&bin);
	}
	else for(i=1; i<argc; i++){
		b = Bopen(argv[i], OREAD);
		if(b == nil)
			sysfatal("can't open %s: %r", argv[i]);
		else{
			cat(b);
			Bterm(b);
		}
	}
	Bflush(&bout);
	exits(0);
}
