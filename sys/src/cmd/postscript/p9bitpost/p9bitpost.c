#include <u.h>
#include <libc.h>
#include <draw.h>
#include <memdraw.h>
#include "pslib.h"

#define HDLEN	60

int dpi = -1;
int debug = 0;
int landscape = 0;
char *file = "<stdin>";

int paperlength = 11*72;
int paperwidth = 612;	/* 8.5*72 */

void
error(char *s)
{
	fprint(2, "p9bitpost: can't %s file %s: %r\n", s, file);
	exits("error");
}

void
main(int argc, char *argv[]) {
	int i, fd = 0;
	double xmag = 1.0, ymag = 1.0;
	char *optstr, *Patch;
	Memimage *memimage;

	Patch = nil;
	for (i=1; i<argc; i++) {
		if (*argv[i] != '-') break;
		switch(argv[i][1]) {
		case 'b':
			if (argv[i][2] == '\0')
				dpi = atoi(argv[++i]);
			else
				dpi = atoi(&(argv[i][2]));
			break;
		case 'd':
			debug = 1;
			break;
		case 'm':
			if (argv[i][2] == '\0')
				optstr = argv[++i];
			else
				optstr = &(argv[i][2]);
			if ((optstr=strtok(optstr, " ,")) != 0)
				xmag = ymag = atof(optstr);
			if ((optstr=strtok(0, " ,")) != 0)
				ymag = atof(optstr);
			break;
		case 'L':
			landscape = 1;
			break;
		case 'P':
			if (argv[i][2] == '\0')
				Patch = argv[++i];
			else
				Patch = &(argv[i][2]);
			break;
		case 'p':
			optstr = argv[++i];
			if(optstr == nil)
				goto Usage;
			paperlength = 72*atof(optstr);
			optstr = argv[++i];
			if(optstr == nil)
				goto Usage;
			paperwidth = 72*atof(optstr);
			if(paperlength < 72 || paperwidth < 72)
				goto Usage;
			break;
		default:
		Usage:
			fprint(2, "usage: %s [-b dpi] [-m magnification] [-L] [-P postscript_patch_string] [-p paperlength paperwidth (in inches)] inputfile\n", argv[0]);
			exits("usage");
		}
	}

	if (i < argc) {
		file = argv[i];
		fd = open(file, OREAD);
		if (fd < 0)
			error("open");
	}

	memimageinit();
	memimage = readmemimage(fd);
	if(memimage == nil)
		error("alloc memory for");

	psinit(0, 0);
	if(xmag != 1.0)
		psopt("xmagnification", &xmag);
	if(ymag != 1.0)
		psopt("ymagnification", &ymag);
	if(landscape)
		psopt("landscape", &landscape);
	if(Patch)
		psopt("Patch", &Patch);
	image2psfile(1, memimage, dpi);
	exits("");
}
