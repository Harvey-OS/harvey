#include <u.h>
#include <libc.h>
#include <libg.h>
#include <ctype.h>
#include <bio.h>
#include <stdio.h>
#include "pc/vga.h"

int pfd;
int typefd;
int sizefd;

Biobuf *fd;

Rectangle Drect;
Point Display;	/* min of register display */
Rectangle RCommand;
Point bol;
int pitch;

int testmode = 0;
int dump = 0;
ushort extport;		/* extension port for ati cards */
char *configfile = "/lib/vgadb";

void
outb(int port, uchar value) {
	if (seek(pfd, port, 0) < 0) {
		perror("aux/vga: outb seek");
		exits("outb seek");
	}
	if (write(pfd, &value, 1) != 1) {
		perror("aux/vga: outb write");
		fprint(2, "port=0x%.4x value=0x%.2x\n", port, value);
		exits("outb write");
	}
}

uchar
inb(int port) {
	uchar value;

	if (seek(pfd, port, 0) < 0) {
		perror("aux/vga: inb seek");
		exits("inb seek");
	}
	if (read(pfd, &value, 1) != 1) {
		perror("aux/vga: inb read");
		exits("inb read");
	}
	return value;
}

#include "pc/common.c"

void
writeconfig(FILE *fd, VGAmode *v) {
	fprintf(fd, "%d %d %d\n", v->w, v->h, v->d);
	writeregisters(fd, v);
}

/*
 * compute the crazy vga end-of-x value. 
 */
int
next(int start, int end, int bits) {
	int size = 1<<bits;
	int mask = size-1;
	int t = (start&~mask) | (end&mask);
	if (t > start)
		return t;
	else
		return t + size;
}

void
box(Rectangle r, int f)
{
	r.max=sub(r.max, Pt(1, 1));
	segment(&screen, r.min, Pt(r.min.x, r.max.y), ~0, f);
	segment(&screen, Pt(r.min.x, r.max.y), r.max, ~0, f);
	segment(&screen, r.max, Pt(r.max.x, r.min.y), ~0, f);
	segment(&screen, Pt(r.max.x, r.min.y) ,r.min, ~0, f);
}

void
do_reshape(void) {
	Drect = inset(bscreenrect(0), 1);
	Display = add(Drect.min, Pt(50,50));
	RCommand.min = Pt(Drect.min.x+20, Drect.max.y-50);
	RCommand.max = Pt(Drect.max.x-20, RCommand.min.y+20);
}

void
init(void) {
	pfd = open("#v/vgamem", 2);
	if (pfd >= 0) {
		fprint(2, "aux/vga:  incompatible with this old kernel\n");
		exits("incompatible");
	}
	pfd = open("#v/vgaport", 2);
	if (pfd < 0) {
		perror("aux/vga: vga port");
		exits("vga port");
	}
	typefd = open("#v/vgatype", 2);
	if (typefd < 0) {
		perror("aux/vga: vga type");
		exits("vga type");
	}
	sizefd = open("#v/vgasize", 2);
	if (sizefd < 0) {
		perror("aux/vga: vgasize");
		exits("vgasize");
	}
	binit(0, 0, "vga");
	do_reshape();
}

/*
 * unlock the tseng chip.
 */
void
unlocktseng(void) {
	outb(0x3bf, 0x03);	/* hercules compatibility reg */
	outb(0x3d8, 0xa0);	/* display mode control register */
}

void
setscreen(char *type, int maxx, int maxy, int depth) {
	int i;
	char buf[200];

	seek(typefd, 0, 0);
	if (write(typefd, type, strlen(type)+1) != strlen(type)+1) {
		perror("aux/vga: write type");
		exits("write type");
	}

	sprint(buf, "%dx%dx%d", maxx, maxy, depth);
	seek(sizefd, 0, 0);
	if (write(sizefd, buf, strlen(buf)+1) != strlen(buf)+1) {
		perror("aux/vga: write size");
		exits("write size");
	}
	i = open("#b", 0);	/* make devbit notice the change */
	close(i);
}

void
setclock(VGAmode *v, int c) {
	if (v->type == tseng) {
		unlocktseng();
		srout(0x00, srin(0x00) & 0xFD);
	}
	outb(EMISCW, (inb(0x3cc)&0xf3) | 	((c&0x03) << 2));
	if (v->type == tseng)
		crout(0x34, (crin(0x34)&0xfd) |	((c&0x04) >> 1));
	srout(0x00, 0x03);
	if (v->type == tseng)
		unlocktseng();	/*why? */
}

/*
 * This does not work.  We need to know more about ATI's registers.
 */
void
setupati(VGAmode *v) {
	int i, saveb4, saveb8;
	char buf[200];
	int lock;
	extout(0xb8, extin(0xb8) & 0xfb);	/* setregs */
	extout(0xa3, extin(0xa3) & 0xdf);	/* setupext */
	/*srout(0x01, v->sequencer[1] | 0x20);	/*turn display off*/
	/*srout(0x00, srin(0x00) & 0xFD);	/* synchronous reset*/

	srout(0x0, 1);
	i = extin(0xb0) & 0xfc;
	extout(0xb0, i);
	extout(0xb0, i|0x02);
	extout(0xb0, i|0x03);
	/*srout(0x0, 0x03);*/
	extout(0xb0, extin(0xb0) | 0x01);
	srout(0x0, 1);
	srout(0x4, 0);
	for(i = 1; i < sizeof(v->sequencer); i++)
		srout(i, v->sequencer[i]);
	outb(EMISCW, v->general[0]);
	srout(0x00, 3);
	sleep(2*500);

	saveb4 = extin(0xb4);
	extout(0xb4, 0);	/* unlock crt regs? */
	crout(0x11, 0x30);	/* this is special for ati */
	/*crout(0x11, crin(0x11) & 0x7f); /*unlock crt regs*/
	for(i = 0; i < sizeof(v->crt); i++)
		if (i == 0x11) {
			crout(0x11, (v->crt[i]&0x4f) | 0x20);
		} else
			crout(i, v->crt[i]);
	outb(0x3ca, v->general[1]); /* set feature control */
	crout(0x11, v->crt[0x11]);
	extout(0xb4, saveb4);

	/*
	 * We don't change the palette values
	 */
	for(i = 0x10; i < sizeof(v->attribute); i++)
		if (i != 0x11)
			arout(i, v->attribute[i]);

	outb(0x3cc, 0);	/* unlock svga graphics? */
	outb(0x3ca, 1);
	for(i = 0; i < sizeof(v->graphics); i++)
		grout(i, v->graphics[i]);

	saveb8 = extin(0xb8);
	extout(0xb8, extin(0xb8)&0x3f | 0x40);

	/*set palette here*/

	outb(0x3C6,0xFF);	/* pel mask */
	outb(0x3C8,0x00);	/* pel write address */

	extout(0xa3, extin(0xa3) | 0x20);	/* turn on display? */
	crout(0x11, v->crt[0x11]);
	crout(0x15, v->crt[0x15]);
	for (i=0xa0; i<=0xbf; i++)
		extout(i, v->specials.ati.ext[i-0xa0]);
	extout(0xa9, v->specials.ati.ext[0xa9-0xa0]); /*no*/
	extout(0xb0, v->specials.ati.ext[0xb0-0xa0]); /*no*/
	extout(0xbd, v->specials.ati.ext[0xbd-0xa0]); /*ok*/
	for (i=0; i<8; i++) {
		outb(0x46e8 + i, v->specials.ati.four6e[i]);
		outb(0x46f8 + i, v->specials.ati.four6f[i]);
	}
	sleep(2*500);
	srout(0x0, 0x3);
	srout(0x01, v->sequencer[1]);	/* turn display back on */
}

void
setupregs(VGAmode *v) {
	int i;
	char buf[200];
	int lock;

	srout(0x01, v->sequencer[1] | 0x20);	/*turn display off*/
	sleep(2*500);
	switch (v->type) {
	case tseng:
		unlocktseng();
		crout(0x11, crin(0x11) & 0x7f); /*unlock crt regs*/
		outb(0x3cd, 0x00);		/* segment select */
		break;
	case pvga1a:
		lock = grin(0xf);
		grout(0xf, 5);
		break;
	case ati:
		outb(0x3cc, 0);	/* unlock svga? */
		outb(0x3ca, 1);
		break;
	}
	srout(0x00, srin(0x00) & 0xFD);	/* synchronous reset*/
	outb(EMISCW, v->general[0]);
	outb(0x3ca, v->general[1]);

	for(i = 0; i < sizeof(v->sequencer); i++)
		if (i != 1) /* we do #1 later */
			srout(i, v->sequencer[i]);
	/*
	 * We don't change the palette values
	 */
	for(i = 16; i < sizeof(v->attribute); i++)
		arout(i, v->attribute[i]);
	crout(0x11, crin(0x11) & 0x7f); /*unlock crt regs*/
	for(i = 0; i < sizeof(v->crt); i++)
		crout(i, v->crt[i]);
	for(i = 0; i < sizeof(v->graphics); i++)
		grout(i, v->graphics[i]);
	outb(0x3C6,0xFF);	/* pel mask */
	outb(0x3C8,0x00);	/* pel write address */
	switch (v->type) {
	pvga1a:
		grout(0xf, lock);
		break;
	case tseng:
		unlocktseng();	/* unlock the key */
		crout(0x11, crin(0x11) & 0x7f);		/* unlock crt 0-7 and 35 */
		srout(0x06, v->specials.tseng.sr6);	/* state ctrl p 142 */
		srout(0x07, v->specials.tseng.sr7);	/* ditto, AuxillaryMode */
		i = inb(0x3da); /* reset flip-flop. inp stat 1*/
		arout(0x16, v->specials.tseng.ar16);	/* misc */
		arout(0x17, v->specials.tseng.ar17);	/* misc 1*/
		crout(0x31, v->specials.tseng.crt31);	/* extended start. */
		crout(0x32, v->specials.tseng.crt32);	/* extended start. */
		crout(0x33, v->specials.tseng.crt33);	/* extended start. */
		crout(0x34, v->specials.tseng.crt34);	/* stub: 46ee + other bits */
		crout(0x35, v->specials.tseng.crt35);	/* overflow bits */
		crout(0x36, v->specials.tseng.crt36);	/* overflow bits */
		crout(0x37, v->specials.tseng.crt37);	/* overflow bits */
		outb(0x3c3, v->specials.tseng.viden);
		break;
	case ati:
		for (i=0xa0; i<=0xbf; i++)
			extout(i, v->specials.ati.ext[i-0xa0]);
	}
	sleep(2*500);
	srout(0x01, v->sequencer[1]);	/* turn display back on */
}

int lineno = 0;
int eor = 0;

int
readch(void) {
	int ch;

	if (eor)
		return -1;
	ch = BGETC(fd);
	if (ch == '\n') {
		lineno++;
		ch = BGETC(fd);
		if (ch != ' ' && ch != '\t')
			eor = 1;
		else
			Bungetc(fd);
	}
	return ch;
}

void
unreadch(void) {
	int ch;
	Bungetc(fd);
	ch = BGETC(fd);
	Bungetc(fd);
	if (ch == '\n')
		lineno --;
}

/*
 * Skip over crap in the input file.  Crap includes white space, commas,  and
 * C-style comments.  We stop at a '"' (string) or a 0 (beginning of a hex constant).
 */
void
skipcrap(void) {
	int ch, incomment=0;
	while ((ch = readch()) >= 0) {
		if (incomment) {
			if (ch == '*')
				if (readch() == '/') {
					incomment = 0;
					continue;
				} else
					unreadch();
			continue;
		}
		switch (ch) {
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
		case '"':
			unreadch();
			return;
		case '/':
			if (readch() == '*') {	/* C comment */
				incomment = 1;
			} else
				unreadch();
		case '\t':
		case '\n':
		case ' ':
		case ',':
		case '\r':
			continue;
		default:
			fprint(2, "aux/vga: config file line %d: Unexpected character: %c\n",
				lineno, ch);;
			exits("config file char");
		}
	}
}

int
gethex(void) {
	int ch;
	int val = 0;

	skipcrap();
	if (readch() != '0' || readch() != 'x' || !isxdigit(ch=readch())) {
		unreadch();
		fprint(2, "aux/vga: config file line %d: error in hex constant: %c\n",
			lineno, readch());
		exits("config file hex error");
	}
	do {
		if (isdigit(ch))
			val = 16*val + ch - '0';
		else if (isupper(ch))
			val = 16*val + tolower(ch) - 'a' + 10;
		else
			val = 16*val + ch - 'a' + 10;
	} while (isxdigit(ch=readch()));
	unreadch();
	return val;
}

int
getnum(void) {
	int ch;
	int val = 0;

	skipcrap();
	ch = readch();
	if (!isdigit(ch)) {
		fprint(2, "aux/vga: config file line %d: error in decimal constant\n",
			lineno);
		exits("config file number");
	}
	do {
		val = 10*val + ch - '0';
	} while (isdigit(ch=readch()));
	unreadch();
	return val;
}

enum vgatype
gettype(void) {
	char buf[100], *p = buf;
	int ch;
	enum vgatype v;

	skipcrap();
	if (readch() != '"') {
		fprint(2, "aux/vga: config file line %d: error in string\n", lineno);
		exits("config file string error");
	}

	while ((ch=readch()) != '"' && p < buf + sizeof(buf) - 1)
		*p++ = ch;
	*p = '\0';
	if (ch != '"') {
		fprint(2, "aux/vga: config file line %d: string too long\n", lineno);
		exits("config file string error");
	}
	for (v = generic; vganames[v]; v++)
		if (strcmp(vganames[v], buf) == 0)
			return v;
	fprint(2, "aux/vga: config file line %d: unknown vga type: %s\n",
		lineno, buf);
	exits("config file vga type");
}

void
findrecord(char *name, int xsize, int ysize, int zsize) {
	char ch;

	if (zsize == 2 || zsize == 4)	/* planar will handle these sizes */
		zsize = 1;
	/* We are at the start of a new line */
	ch = BGETC(fd);
	lineno = 1;
	while (ch >= 0) {
		if (isalnum(ch)) {	/* new configuration */
			char *np = name;
			while (*np && !isspace(ch))
				if (*np == ch) {
					ch = BGETC(fd);
					np++;
				} else
					break;
			if (*np == 0 && isspace(ch)) {
				int x, y, z;
				x = getnum();  y = getnum();  z = getnum();
				if (x == xsize && y == ysize && z == zsize)
					return;
			}
		}
		while (ch != '\n')	/* skip to next line */
			ch = BGETC(fd);
		lineno++;
		ch = BGETC(fd);
	}
	fprint(2, "aux/vga: not found in %s:  %s %d %d %d\n", configfile,
		name, xsize, ysize, zsize);
	exits("config not found");
}

void
readconfig(char *name, int xsize, int ysize, int zsize, VGAmode *v) {
	int i;

	findrecord(name, xsize, ysize, zsize);
	v->w = xsize;
	v->h = ysize;
	v->d = zsize;

	for (i=0; i<sizeof(v->general); i++)
		v->general[i] = gethex();
	for (i=0; i<sizeof(v->sequencer); i++)
		v->sequencer[i] = gethex();
	for (i=0; i<sizeof(v->crt); i++)
		v->crt[i] = gethex();
	for (i=0; i<sizeof(v->graphics); i++)
		v->graphics[i] = gethex();
	for (i=0; i<sizeof(v->attribute); i++)
		v->attribute[i] = gethex();
	v->type = gettype();
	switch (v->type) {
	case generic:
		break;
	case tseng:
		for (i=0; i<sizeof(v->specials.tseng); i++)
			v->specials.generic.dummy[i] = gethex();
		break;
	case pvga1a:
		for (i=0; i<sizeof(v->specials.pvga1a)-1/*stub: why!*/; i++) {
			v->specials.generic.dummy[i] = gethex();
		}
		break;
	case ati:
		for (i=0; i<sizeof(v->specials.ati)-2; i++) {
			v->specials.generic.dummy[i] = gethex();
		}
		extport = v->specials.ati.extport;
		break;
	}
}

Biobuf *
openconfig(char *fn) {
	char buf[100];
	Biobuf *fd;

	if (fn[0] == '/')	/* he gave an absolute path */
		return Bopen(fn, OREAD);

	fd = Bopen(fn, OREAD);	/* try local file */
	if (fd != (Biobuf *)0)
		return fd;

	/* try user's own vga directory */
	snprint(buf, sizeof(buf),
		"/usr/%s/lib/vga/%s", getenv("home"), fn);
	fd = Bopen(buf, OREAD);
	if (fd != (Biobuf *)0)
		return fd;

	/* try vga library with 'vgatype' environment variable */
	if (getenv("vgatype")) {
		snprint(buf, sizeof(buf), "/lib/vga/%s/%s", getenv("vgatype"), fn);
		fd = Bopen(buf, OREAD);
		if (fd != (Biobuf *)0)
			return fd;
	}

	snprint(buf, sizeof(buf), "/lib/vga/%s", fn);
	fd = Bopen(buf, OREAD);
	if (fd != (Biobuf *)0)
		return fd;

	perror("aux/vga: open config file");
	exits("open config");
}

void
showdiff(VGAmode *ov, VGAmode *nv, int chip) {
	int i;

	for (i=0; i<sizeof(ov->general); i++)
		if (ov->general[i] != nv->general[i])
			fprintf(stdout, "gen  %.2x %.2x %.2x %.2x\n",
				i, ov->general[i], nv->general[i],
				ov->general[i]^nv->general[i]);

	for (i=0; i<sizeof(ov->sequencer); i++)
		if (ov->sequencer[i] != nv->sequencer[i])
			fprintf(stdout, "seq  %.2x %.2x %.2x %.2x\n",
				i, ov->sequencer[i], nv->sequencer[i],
				ov->sequencer[i]^nv->sequencer[i]);

	for (i=0; i<sizeof(ov->crt); i++)
		if (ov->crt[i] != nv->crt[i])
			fprintf(stdout, "crt  %.2x %.2x %.2x %.2x\n",
				i, ov->crt[i], nv->crt[i],
				ov->crt[i]^nv->crt[i]);

	for (i=0; i<sizeof(ov->graphics); i++)
		if (ov->graphics[i] != nv->graphics[i])
			fprintf(stdout, "gr    %.2x %.2x %.2x %.2x\n",
				i, ov->graphics[i], nv->graphics[i],
				ov->graphics[i]^nv->graphics[i]);

	for (i=0x10; i<sizeof(ov->attribute)-0x10; i++)
		if (ov->attribute[i] != nv->attribute[i])
			fprintf(stdout, "attr %.2x %.2x %.2x %.2x\n",
				i, ov->attribute[i], nv->attribute[i],
				ov->attribute[i]^nv->attribute[i]);


	switch(chip) {
	case ati:
		for (i=0; i< sizeof(ov->specials.ati.ext); i++)
			if (ov->specials.ati.ext[i] != 
			    nv->specials.ati.ext[i])
				fprintf(stdout, "ext  %.2x %.2x %.2x %.2x\n",
					i+0xa0, ov->specials.ati.ext[i],
					nv->specials.ati.ext[i],
					nv->specials.ati.ext[i]^ov->specials.ati.ext[i]);
		for (i=0; i< sizeof(ov->specials.ati.four6e); i++)
			if (ov->specials.ati.four6e[i] != 
			    nv->specials.ati.four6e[i])
				fprintf(stdout, "46e%.1x %.2x %.2x %.2x\n",
					i+8, ov->specials.ati.four6e[i],
					nv->specials.ati.four6e[i],
					nv->specials.ati.four6e[i]^ov->specials.ati.four6e[i]);
		for (i=0; i< sizeof(ov->specials.ati.four6f); i++)
			if (ov->specials.ati.four6f[i] != 
			    nv->specials.ati.four6f[i])
				fprintf(stdout, "46f%.1x %.2x %.2x %.2x\n",
					i+8, ov->specials.ati.four6f[i],
					nv->specials.ati.four6f[i],
					nv->specials.ati.four6f[i]^ov->specials.ati.four6f[i]);
	}
}

void
usage(void) {
	fprint(2, "usage: aux/vga [-t vgatype] config [xsize [ysize [zsize]]]\n");
	exits("usage");
}

char *
main(int argc, char *argv[]) {
	int i, p, val;
	int depth = -1;
	char buf[200];
	VGAmode v;
	char *vganame;

	enum vgatype chip = generic;

	init();
	USED(argc);  USED(argv);
	ARGBEGIN {
	case 'd':
		depth = atoi(ARGF());
		break;
	case 'f':
		configfile = ARGF();
		break;
	case 't':
		vganame = ARGF();
		for (chip = generic; vganames[chip]; chip++)
			if (strcmp(vganames[chip], vganame) == 0)
				break;
		if (vganames[chip] == 0)
			usage();
		break;
	case 'i':
		testmode++;
		break;
	case 'D':
		dump++;
	} ARGEND;

	if (argc >= 1) {
		char *config = argv[0];
		long xsize = 640, ysize = 480, zsize = 1;

		argc--; argv++;
		if (argc >= 1) {
			xsize = atol(argv[0]);
			switch (xsize) {
			case 320:	ysize = 200;	break;
			case 640:	ysize = 480;	break;
			case 800:	ysize = 600;	break;
			case 1024:	ysize = 768;	break;
			case 1280:	ysize = 1024;	break;
			default:
				fprint(2, "aux/vga: Illegal X size: %d\n", argv[0]);
				exits("params");
			}
			argc--, argv++;
		}
		if (argc >= 1) {
			ysize = atol(argv[0]);
			argc--, argv++;
		}
		if (argc >= 1) {
			zsize = atol(argv[0]);
			argc--, argv++;
		}
		fd = openconfig(configfile);
		readconfig(config, xsize, ysize, zsize, &v);
		chip = v.type;
		Bclose(fd);
		if (depth < 0)
			depth = v.d;
		else
			if ((v.d == 8 && depth != 8) ||
			    (v.d < 8 && depth >= 8)) {
				fprint(2, "aux/vga: Depth %d not supported by VGA depth %d\n",
					depth, v.d);
				exits("depth error");
			}
		setscreen(vganames[v.type], v.w, v.h, depth);
		switch (chip) {
		case ati:
			setupati(&v);
			break;
		default:
			setupregs(&v);
		}
	}

	if (dump) {
		VGAmode nv;
		getmode(&nv, chip);
		showdiff(&v, &nv, chip);
	}

	if (!testmode)
		exits("");

	v.type = chip;
	getmode(&v, chip);
	writeconfig(stderr, &v);
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
		case 'a':	/* analyze VGA registers */
			break;
		case 'd':
			getmode(&v, chip);
			writeconfig(stderr, &v);
			break;
		case 'w':	/* write config to stdout */
			writeconfig(stdout, &v);
			break;
		case 'q':
		case 'x':
			exits("");
		case 'A':
			doreg(arin, arout, &line[1]);
			break;
		case 'C':
			doreg(crin, crout, &line[1]);
			break;
		case 'K':	/* setup clock */
			sscanf(&line[1], "%d", &i);
			if (i > 7 /*tseng*/)
				fprintf(stderr, "Clock too large\n");
			else {
				getmode(&v, v.type);
				setclock(&v, i);
			}
			break;
		case 'S':
			doreg(srin, srout, &line[1]);
			break;
		case '+':		/* increment the vsync/hsync field */
			outb(EMISCW, (inb(0x3cc) + 0x40) & 0xff);
			break;
		default:
			fprintf(stderr, "Bad command\n");
		}
	} while(1);
	return "";
}
