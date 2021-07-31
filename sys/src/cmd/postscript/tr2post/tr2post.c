#include <u.h>
#include <libc.h>
#include <bio.h>
#include <stdio.h>
#include "common.h"
#include "tr2post.h"
#include "comments.h"
#include "path.h"

int formsperpage = 1;
int picflag = 1;
double aspectratio = 1.0;
int copies = 1;
int landscape = 0;
double magnification = 1.0;
int linesperpage = 66;
int pointsize = 10;
double xoffset = .25;
double yoffset = .25;
char *passthrough = 0;

Biobuf binp, *bstdout, bstderr;
Biobufhdr *Bstdin, *Bstdout, *Bstderr;
int debug = 0;

char tmpfilename[MAXTOKENSIZE];
char copybuf[BUFSIZ];


struct charent **build_char_list = 0;
int build_char_cnt = 0;

void
prologues(void) {
	int i;
	char charlibname[MAXTOKENSIZE];

	Bprint(Bstdout, "%s", CONFORMING);
	Bprint(Bstdout, "%s %s\n", VERSION, PROGRAMVERSION);
	Bprint(Bstdout, "%s %s\n", CREATOR, PROGRAMNAME);
	Bprint(Bstdout, "%s %s\n", DOCUMENTFONTS, ATEND);
	Bprint(Bstdout, "%s %s\n", PAGES, ATEND);
	Bprint(Bstdout, "%s", ENDCOMMENTS);

	if (cat(DPOST)) {
		Bprint(Bstderr, "can't read %s\n", DPOST);
		exits("dpost prologue");
	}

	if (drawflag) {
		if (cat(DRAW)) {
			Bprint(Bstderr, "can't read %s\n", DRAW);
			exits("draw prologue");
		}
	}

	if (DOROUND)
		cat(ROUNDPAGE);

	Bprint(Bstdout, "%s", ENDPROLOG);
	Bprint(Bstdout, "%s", BEGINSETUP);
	Bprint(Bstdout, "mark\n");
	if (formsperpage > 1) {
		Bprint(Bstdout, "%s %d\n", FORMSPERPAGE, formsperpage);
		Bprint(Bstdout, "/formsperpage %d def\n", formsperpage);
	}
	if (aspectratio != 1) Bprint(Bstdout, "/aspectratio %g def\n", aspectratio);
	if (copies != 1) Bprint(Bstdout, "/#copies %d store\n", copies);
	if (landscape) Bprint(Bstdout, "/landscape true def\n");
	if (magnification != 1) Bprint(Bstdout, "/magnification %g def\n", magnification);
	if (pointsize != 10) Bprint(Bstdout, "/pointsize %d def\n", pointsize);
	if (xoffset != .25) Bprint(Bstdout, "/xoffset %g def\n", xoffset);
	if (yoffset != .25) Bprint(Bstdout, "/yoffset %g def\n", yoffset);
	cat(ENCODINGDIR"/Latin1.enc");
	if (passthrough != 0) Bprint(Bstdout, "%s\n", passthrough);

	Bprint(Bstdout, "setup\n");
	if (formsperpage > 1) {
		cat(FORMFILE);
		Bprint(Bstdout, "%d setupforms \n", formsperpage);
	}
/* output Build character info from charlib if necessary. */

	for (i=0; i<build_char_cnt; i++) {
		sprint(charlibname, "%s/%s", CHARLIB, build_char_list[i]->name);
		if (cat(charlibname))
		Bprint(Bstderr, "cannot open %s\n", charlibname);
	}

	Bprint(Bstdout, "%s", ENDSETUP);
}

void
cleanup(void) {
	remove(tmpfilename);
}

main(int argc, char *argv[]) {
	Biobuf *binp;
	Biobufhdr *Binp;
	int i, tot, ifd;
	char *t;

	programname = argv[0];
	if (Binit(&bstderr, 2, OWRITE) == Beof) {
		exits("Binit");
	}
	Bstderr = &bstderr.Biobufhdr;

	tmpnam(tmpfilename);
	if ((bstdout=Bopen(tmpfilename, OWRITE)) == 0) {
		Bprint(Bstderr, "cannot open temporary file %s\n", tmpfilename);
		exits("Bopen");
	}
	atexit(cleanup);
	Bstdout = &bstdout->Biobufhdr;
	
	ARGBEGIN{
		case 'a':			/* aspect ratio */
			aspectratio = atof(ARGF());
			break;
		case 'c':			/* copies */
			copies = atoi(ARGF());
			break;
		case 'd':
			debug = 1;
			break;
		case 'm':			/* magnification */
			magnification = atof(ARGF());
			break;
		case 'n':			/* forms per page */
			formsperpage = atoi(ARGF());
			break;
		case 'o':			/* output page list */
			pagelist(ARGF());
			break;
		case 'p':			/* landscape or portrait mode */
			if ( ARGF()[0] == 'l' )
				landscape = 1;
			else
				landscape = 0;
			break;
		case 'x':			/* shift things horizontally */
			xoffset = atof(ARGF());
			break;
		case 'y':			/* and vertically on the page */
			yoffset = atof(ARGF());
			break;
		case 'P':			/* PostScript pass through */
			t = ARGF();
			i = strlen(t) + 1;
			passthrough = malloc(i);
			if (passthrough == 0) {
				Bprint(Bstderr, "cannot allocate memory for argument string\n");
				exits("malloc");
			}
			strncpy(passthrough, t, i);
			break;
		default:			/* don't know what to do for ch */
			Bprint(Bstderr, "unknown option %C\n", ARGC());
			break;
	}ARGEND;
	readDESC();
	if (argc == 0) {
		if ((binp = (Biobuf *)malloc(sizeof(Biobuf))) == nil) {
			Bprint(Bstderr, "malloc failed.\n");
			exits("malloc");
		}
		if (Binit(binp, 0, OREAD) == Beof) {
			Bprint(Bstderr, "Binit of <stdin> failed.\n");
			exits("Binit");
		}
		Binp = &(binp->Biobufhdr);
		if (debug) Bprint(Bstderr, "using standard input\n");
		conv(Binp);
		Bterm(Binp);
	}
	for (i=0; i<argc; i++) {
		if ((binp=Bopen(argv[i], OREAD)) == 0) {
			Bprint(Bstderr, "cannot open file %s\n", argv[i]);
			continue;
		}
		Binp = &(binp->Biobufhdr);
		inputfilename = argv[i];
		conv(Binp);
		Bterm(Binp);
	}
	Bterm(Bstdout);

	if ((ifd=open(tmpfilename, OREAD)) < 0) {
		Bprint(Bstderr, "open of %s failed.\n", tmpfilename);
		exits("open");
	}

	bstdout = galloc(0, sizeof(Biobuf), "bstdout");
	if (Binit(bstdout, 1, OWRITE) == Beof) {
		Bprint(Bstderr, "Binit of <stdout> failed.\n");
		exits("Binit");
	}
	Bstdout = &(bstdout->Biobufhdr);
	prologues();
	Bflush(Bstdout);
	tot = 0; i = 0;
	while ((i=read(ifd, copybuf, BUFSIZ)) > 0) {
		if (write(1, copybuf, i) != i) {
			Bprint(Bstderr, "write error on copying from temp file.\n");
			exits("write");
		}
		tot += i;
	}
	if (debug) Bprint(Bstderr, "copied %d bytes to final output i=%d\n", tot, i);
	if (i < 0) {
		Bprint(Bstderr, "read error on copying from temp file.\n");
		exits("read");
	}
	finish();
		
	exits("");
}
