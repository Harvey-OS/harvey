#include <u.h>
#include <libc.h>
#include <bio.h>
#include <draw.h>
#include <html.h>
#include "dat.h"

int verbose;

void
usage(void)
{
	fprint(2, "usage: webscript [-xv] [script ...]\n");
	exits("usage");
}

void
main(int argc, char **argv)
{
	int i;
	static Biobuf b0;

	fmtinstall('F', formfieldfmt);
	fmtinstall('T', thingfmt);
	fmtinstall('W', wherefmt);
	fmtinstall('Z', runeurlencodefmt);
	fmtinstall('z', urlencodefmt);
	fmtinstall('y', cencodefmt);
	
	ARGBEGIN{
	case 'L':
		yylexdebug = 1;
		break;
	case 'x':
		xtrace = 1;
		break;
	case 'v':
		verbose = 1;
		break;
	default:
		usage();
	}ARGEND
	
	if(argc == 0){
		Binit(&b0, 0, OREAD);
		filename = "<stdin>";
		lineno = 0;
		bscript = &b0;
		yyparse();
		bscript = nil;
	}else{
		for(i=0; i<argc; i++){
			if((bscript = Bopen(argv[i], OREAD)) == nil)
				sysfatal("open %s: %r", argv[i]);
			filename = argv[i];
			lineno = 0;
			yyparse();
			Bterm(bscript);
			bscript = nil;
		}
	}
}

