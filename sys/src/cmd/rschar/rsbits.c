#include <u.h>
#include <libc.h>
#include <bio.h>
#include "../dict/kuten.h"
#include "rsclass.h"

uchar	rsclass[RMAX-RMIN];

int	debug;
Biobuf *out;

void
main(int argc, char **argv)
{
	Rune *rp;
	int i;

	ARGBEGIN{
	case 'D':
		++debug;
		break;
	}ARGEND
	for(rp=tabjis208; rp<&tabjis208[JIS208MAX]; rp++)
		if(RMIN <= *rp && *rp < RMAX)
			rsclass[*rp-RMIN] |= JIS208;
	for(rp=tabgb2312; rp<&tabgb2312[GB2312MAX]; rp++)
		if(RMIN <= *rp && *rp < RMAX)
			rsclass[*rp-RMIN] |= GB2312;
	for(rp=tabbig5; rp<&tabbig5[BIG5MAX]; rp++)
		if(RMIN <= *rp && *rp < RMAX)
			rsclass[*rp-RMIN] |= BIG5;
	out = Bopen("/fd/1", OWRITE);
	Bprint(out, "#include <u.h>\n\n");
	Bprint(out, "uchar rsclass[] = {\n");
	for(i=0; i<sizeof rsclass; i++){
		if(i%20 == 0)
			Bprint(out, "\t");
		else
			Bprint(out, " ");
		Bprint(out, "%d,", rsclass[i]);
		if(i%20 == 19)
			Bprint(out, "\n");
	}
	if(i%20 != 0)
		Bprint(out, "\n");
	Bprint(out, "};\n");
	exits(0);
}
