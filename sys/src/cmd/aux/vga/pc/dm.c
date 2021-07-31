/*
 * dump vga mode registers.
 */

#include <graph.h>
#include <ctype.h>
#include <stdio.h>
#include <conio.h>
#include <dos.h>

typedef unsigned char uchar;
typedef unsigned short ushort;

#include "vga.h"

enum vgatype chip = generic;

ushort extport;	/* for ati devices */

/*
 * argument processing
 */
#define	ARGBEGIN	for(argv++,argc--;\
			    argv[0] && argv[0][0]=='-' && argv[0][1];\
			    argc--, argv++) {\
				char *_args, *_argt;\
				char _argc;\
				_args = &argv[0][1];\
				if(_args[0]=='-' && _args[1]==0){\
					argc--; argv++; break;\
				}\
				_argc = 0;\
				while(_argc = *_args++)\
				switch(_argc)
#define	ARGEND		}
#define	ARGF()		 (_argt=_args, _args="",\
				(*_argt? _argt: argv[1]? (argc--, *++argv): 0))
#define	ARGC()		_argc

#define	inb(x)	inp(x)
#define outb(u,i)	outp(u,i)

char comment[1000];

typedef	unsigned char	uchar;


void far *
addr(ushort seg, ushort off) {
	void far *a;
	FP_SEG(a) = seg;
	FP_OFF(a) = off;
	return a;
}

#include "common.c"

int
csegstr(char *s1, unsigned short off) {
	uchar far *vp = addr(0xc000, off);
	short n = strlen(s1);
	
	while (n-- > 0)
		if (*vp++ != *s1++)
			return 0;
	return 1;
}
	
enum vgatype
getchiptype(void) {
	if (csegstr("VGA=", 0x7d))
		return pvga1a;
	if (csegstr("Tseng", 0x76))
		return tseng;
	if (csegstr("ATI", 0xC1)) {
		extport = *((ushort far *)addr(0xc000, 0x0010));
		return ati;
	}
	return generic;
}

int
setdosmode(int mode, enum vgatype type) {
	struct WORDREGS r;
	r.ax = mode;
	int86(0x10, (union REGS *)&r, (union REGS *)&r);
	comment[0] = '\0';
	/*
	 * we would like to know if the set mode worked.  This is
	 * one rom function that does not return a code.  This
	 * awful kludge is worth having.
	 */
	switch (type) {
	case tseng:
		return r.ax == 0x0020;
	default:
		fprintf(stderr, "setdos returned flags=0x%.4x ax=0x%.4x\n",
			r.cflag, r.ax);
		return 1;
	}
}

void
writeconfig(FILE *fd, VGAmode *v, int mode) {
	int i;

	_moveto(0,0);
	fprintf(fd, "	/* mode 0x%02x (%s) */\n", mode, comment);
	writeregisters(fd, v);
	fflush(fd);
}

void
testmem(int width, int planar) {
	unsigned char far *buf;
	int nbytes = planar ? width : width/8;
	int white = planar ? 0x80 : 0xff;
	int i;
	int lines;
	FP_SEG(buf) = 0xa000;
	FP_OFF(buf) = 0;

	for (lines=0; lines <8; lines++)
		for (i=0; i<nbytes; i++)
			*buf++ = white;

	for (lines=8; lines <16; lines++)
		for (i=0; i<nbytes; i++)
			if (i < 8 || i > nbytes-8)
				*buf++ = white;
			else
				*buf++ = 0;

	for (lines=16; lines <24; lines++) {
		for (i=0; i<nbytes; i++) {
			if (i % 10 == 0)
				*buf++ = 0;
			else
				*buf++ = i & 0xff;
		}
	}
}

void
readmode(int *m) {
	int newmode = 0;
	int ch = getch();
	
	while (isxdigit(ch)) {
		if (isdigit(ch))
			newmode = 16*newmode + ch - '0';
		else
			newmode = 16*newmode + tolower(ch) - 'a' + 10;
		ch = getch();
	}

	if (newmode)
		*m = newmode;
	return;
}

uchar
port46(ushort i) {
	return inb(0x4600+i);
}

void
dumpall (uchar in(ushort)) {
	short i;

	for (i=0; i<=0xff; i++) {
		if (i % 16 == 0)
			fprintf(stderr, "\n%.2x  ", i);
		fprintf(stderr, "%.2x ", in(i));
	}
	fprintf(stderr, "\n\n");
}

void
usage(void) {
	fprintf(stderr, "usage: dm [-t chip-type] [comment]\n");
	exit(1);
}

main(int argc, char *argv[]) {
	VGAmode v;
	char *vganame;
	int i,j;
	int mode = 0;

	ARGBEGIN {
	case 't':
		vganame = ARGF();
		for (chip = generic; vganames[chip]; chip++)
			if (strcmp(vganames[chip], vganame) == 0)
				break;
		if (vganames[chip] == 0)
			usage();
		if (chip == ati)
			extport = *((ushort far *)addr(0xc000, 0x0010));
		break;
	case 'm':
		sscanf(ARGF(), "%x", &mode);
		setdosmode(mode, chip);
	} ARGEND;
	
	if (mode || argc) {
		getmode(&v, chip);
		strcpy(comment, argv[0]);
		writeconfig(stdout, &v, mode);
		exit(0);
	}

	do {
		char line[300];
		int ch, len;
		fgets(line, sizeof(line)-1, stdin); 
		len = strlen(line)-1;
		line[len] = '\0';
		if (len == 0)
			continue;
		ch = line[0];
		switch (ch) {
		case '1':
			testmem(320, 0);
			break;
		case '2':
			testmem(640, 0);
			break;
		case '3':
			testmem(800, 0);
			break;
		case '4':
			testmem(1024, 0);
			break;
		case '5':
			testmem(1280, 0);
			break;
		case '6':
			testmem(320, 1);
			break;
		case '7':
			testmem(640, 1);
			break;
		case '8':
			testmem(800, 1);
			break;
		case '9':
			testmem(1024, 1);
			break;
		case '0':
			testmem(1280, 1);
			break;
		case '+':
			for (++mode; !setdosmode(mode, chip) &&
			    mode < 0xff; ++mode)
				;
			getmode(&v, chip);
			writeconfig(stdout, &v, mode);
			break;
		case '-':
			for (--mode; mode >= 0 && !setdosmode(mode, chip);)
				mode--;
			writeconfig(stdout, &v, mode);
			break;
		case 'c':
			strcpy(comment, &line[1]);
			writeconfig(stdout, &v, mode);
			break;
		case 'd':
			getmode(&v, chip);
			writeconfig(stdout, &v, mode);
			break;
		case 'm':
			sscanf(&line[1], "%x", &mode);
			if (setdosmode(mode, chip)) {
				getmode(&v, chip);
/*				writeconfig(stdout, &v, mode);*/
			} else
				fprintf(stderr, "mode 0x.2x not available\n",
					mode);
			break;
		case 't':
			chip = getchiptype();
			fprintf(stderr, "Chip type is %s\n", vganames[chip]);
			break;
		case 'w':
			writeconfig(stdout, &v, mode);
			break;
		case 'x':
			exit(0);
		case 'A':
			doreg(arin, arout, &line[1]);
			break;
		case 'C':
			doreg(crin, crout, &line[1]);
			break;
		case 'D':	/* dump registers */
			switch (line[1]) {
			case 'a':	dumpall(arin);	break;
			case 'c':	dumpall(crin);	break;
			case 'e':	dumpall(extin);	break;
			case 'g':	dumpall(grin);	break;
			case 's':	dumpall(srin);	break;
			case '4':	dumpall(port46); break;
			default:
				fprintf(stderr, "D needs a,c,e,g,s,4\n");
			}
			break;
		case 'E':	/* ext ports for ATI */
			doreg(extin, extout, &line[1]);
			break;
		case 'G':
			doreg(grin, grout, &line[1]);
			break;
		case 'S':
			doreg(srin, srout, &line[1]);
			break;
		default:
			fprintf(stderr, "what?  '%s'\n", line);
		}
	} while (1);
}
