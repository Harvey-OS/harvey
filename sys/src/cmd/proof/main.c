#include	<u.h>
#include	<libc.h>
#include	<libg.h>
#include	<bio.h>
#include	"proof.h"

int Mode = DorS;

Rectangle rpage = { 0, 0, 850, 1150 };
char devname[64];
double mag = DEFMAG;
int dbg = 0;
char *track = 0;
Biobuf bin;
char libfont[100] = "/lib/font/bit";
char mapfile[100] = "MAP";
char *mapname = "MAP";

void
main(int argc, char *argv[])
{
	char c;
	int dotrack = 0;
	
	for (argv++; *argv && (**argv == '-'); argv++)
		switch(argv[0][1]) {
		case 'm':	/* magnification */
			mag = atof(&argv[0][2]);
			if (mag < 0.1 || mag > 10){
				fprint(2, "ridiculous mag argument ignored\n");
				mag = DEFMAG;
			}
			break;
		case '/':
			nview = atoi(&argv[0][2]);
			if (nview < 1 || nview > MAXVIEW)
				nview = 1;
			break;
		case 'x':
			xyoffset.x += atoi(&argv[0][2]) * 100;
			break;
		case 'y':
			xyoffset.y += atoi(&argv[0][2]) * 100;
			break;
		case 'M':	/* change MAP file */
			if (argv[0][2])
				strcpy(mapname, &argv[0][2]);
			else {
				strcpy(mapname, argv[1]);
				argv++;
				argc--;
			}
			break;
		case 'F':	/* change /lib/font/bit directory */
			if (argv[0][2])
				strcpy(libfont, &argv[0][2]);
			else {
				strcpy(libfont, argv[1]);
				argv++;
				argc--;
			}
			break;
		case 'd':
			dbg = 1;
			break;
		case 't':
			dotrack = 1;
			break;
		default:
			fprint(2, "unknown option '%s' ignored!\n", *argv);
			break;
		}
	if (*argv) {
		close(0);
		if (open(*argv, 0) == -1) {
			perror(*argv);
			exits("open failure");
		}
		if(dotrack)
			track = *argv;
	}
	Binit(&bin, 0, OREAD);
	sprint(mapfile, "%s/%s", libfont, mapname);
	readmapfile(mapfile);
	for (c = 0; c < NFONT; c++)
		loadfontname(c, "??");
	mapscreen();
	clearscreen();
	readpage(); 
}
