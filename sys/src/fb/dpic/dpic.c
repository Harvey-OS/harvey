/*
 * dpic - troff post-processor for anti-aliased scan-conversion into picture files
 * output language from troff:
 * all numbers are character strings
 *
 * sn	size in points
 * fn	font as number from 1-n
 * cx	ascii character x
 * Cxyz	funny char xyz. terminated by white space
 * Nn   character at position n of current font
 * Hn	go to absolute horizontal position n
 * Vn	go to absolute vertical position n (down is positive)
 * hn	go n units horizontally (relative)
 * vn	ditto vertically
 * nnc	move right nn, then print c (exactly 2 digits!)
 * 		(this wart is an optimization that shrinks output file size
 * 		 about 35% and run-time about 15% while preserving ascii-ness)
 * Dt ...\n	draw operation 't':
 * 	Dl x y		line from here by x,y
 * 	Dc d		circle of diameter d with left side here
 * 	De x y		ellipse of axes x,y with left side here
 *	Da x1 y1 x2 y2	arc counter-clockwise from current point (x, y) to
 *			(x + x1 + x2, y + y1 + y2)
 * 	D~ x y x y ...	wiggly line by x,y then x,y ...
 * nb a	end of line (information only -- no action needed)
 * 	b = space before line, a = after
 * w    word space (information only -- no action needed)
 * p	new page begins -- set v to 0
 * #...\n	comment
 * x ...\n	device control functions:
 * 	x i	init
 * 	x T s	name of device is s
 * 	x r n h v	resolution is n/inch
 * 		h = min horizontal motion, v = min vert
 * 	x p	pause (can restart)
 * 	x s	stop -- done forever
 * 	x t	generate trailer
 * 	x f n s	font position n contains font s
 * 	x H n	set character height to n
 * 	x S n	set slant to N
 *	x X ...	copy through from troff
 *		x X color r g b a	set type color
 *		x X bgcolor r g b a	set background color
 *		x X clear		clear the page to the background color
 *		x X picfile name x y	copy a picture file
 *		x X clrwin x0 y0 x1 y1	clear a window to the background color
 *		x X border x0 y0 x1 y1	draw a border around a window in the foreground color
 * 
 * 	Subcommands like "i" are often spelled out like "init".
 */
#include "ext.h"
#define FONTDIR	"/sys/lib/troff/font"	/* directory containing dev directories */
#define	DEVNAME	"202"			/* name of the target printer */
typedef struct{
	char *str;			/* where the string is stored */
	int dx;				/* width of a space */
	int spaces;			/* number of space characters */
	int start;			/* horizontal starting position */
	int width;			/* and its total width */
}Line;
char *fontdir=FONTDIR;			/* font table directories */
char *realdev=DEVNAME;			/* use these width tables */
char devname[20]="";			/* job formatted for this device */
int smnt;				/* special fonts start here */
int devres;				/* device resolution */
int unitwidth;				/* and unitwidth - from DESC file */
int nfonts=0;				/* number of font positions */
int curfont=0, size=0;			/* current font position and size */
int hpos=0, vpos=0;			/* where troff wants to be */
int fontheight=0;			/* points from x H ... */
int fontslant=0;			/* angle from x S ... */
double res;				/* resolution assumed in input file */
float widthfac=1.0;			/* for emulation = res/devres */
int seenpage=0;				/* expect fonts are now all mounted */
int gotspecial=0;			/* append special fonts - emulation */
char **argv;
int argc;
int pagenum=-1;				/* current page number, -1 means no output */
int ignore=0;				/* what we do with FATAL errors */
long lineno=0;				/* line number */
char *prog_name="gok";			/* and program name - for errors */
int xmin=0, ymin=0;
int pagewid=640, pagehgt=480;
double dpi=100.;
void main(int ac, char *av[]){
	int y;
	FILE *fp;
	argc=ac;
	argv=av;
	prog_name=argv[0];
	argc=getflags(argc, argv, "o:1[list]w:4[x0 y0 x1 y1]d:1[dpi]F:1[fontdir]T:1[typesetter]Is:1[stem]");
	if(argc<1) usage("[file ...]");
	if(flag['o']) out_list(flag['o'][0]);
	if(flag['F']) fontdir=flag['F'][0];
	if(flag['T']) realdev=flag['T'][0];
	if(flag['I']) ignore=1;
	if(flag['w']){
		xmin=atoi(flag['w'][0]);
		ymin=atoi(flag['w'][1]);
		pagewid=atoi(flag['w'][2])-xmin;
		pagehgt=atoi(flag['w'][3])-ymin;
		if(pagehgt<=0 || pagewid<=0){
			fprintf(stderr, "%s: bad window\n", argv[0]);
			exits("bad window");
		}
	}
	page=emalloc(pagehgt*sizeof(Rgba *));
	page[0]=emalloc(pagehgt*pagewid*sizeof(Rgba));
	for(y=1;y!=pagehgt;y++) page[y]=page[y-1]+pagewid;
	image=emalloc(pagehgt*sizeof(float *));
	image[0]=emalloc(pagehgt*pagewid*sizeof(float));
	for(y=1;y!=pagehgt;y++) image[y]=image[y-1]+pagewid;
	enter=emalloc((pagehgt+1)*sizeof(Edge));
	span=emalloc((pagehgt+1)*sizeof(Span));
	if(flag['d']){
		dpi=atof(flag['d'][0]);
		if(dpi<=0){
			fprintf(stderr, "%s: bad dpi\n", argv[0]);
			exits("bad dpi");
		}
	}
	if(argc<=1)
		conv(stdin);
	else
		while(--argc>0){
			if((fp=fopen(*++argv, "r"))==0)
				error("can't open %s", *argv);
			conv(fp);
			fclose(fp);
		}
	exits("");
}
void conv(FILE *fp){
	int c, m, n, n1, m1;
	char str[50];
	t_page(-1);			/* do output only after a page command */
	lineno=1;
	while((c=getc(fp))!=EOF){
		switch (c) {
		case '\n':		/* just count this line */
			lineno++;
			break;
		case ' ':		/* when input is text */
		case 0:			/* occasional noise creeps in */
			break;
		case '0': case '1': case '2': case '3': case '4':
		case '5': case '6': case '7': case '8': case '9':
				/* two motion digits plus a character */
			hmot((c-'0')*10 + getc(fp)-'0');
			put1(getc(fp));
			break;
		case 'c':		/* single ascii character */
			put1(getc(fp));
			break;

		case 'C':		/* special character */
			fscanf(fp, "%s", str);
			put1(chindex(str));
			break;
		case 'N':		/* character at position n */
			fscanf(fp, "%d", &m);
			oput(m);
			break;
		case 'D':		/* drawing functions */
			switch((c=getc(fp))){
			case 'l':	/* draw a line */
				fscanf(fp, "%d %d %c", &n, &m, &n1);
				drawline(n, m);
				break;
			case 'c':	/* circle */
				fscanf(fp, "%d", &n);
				drawcirc(n);
				break;
			case 'e':	/* ellipse */
				fscanf(fp, "%d %d", &m, &n);
				drawellip(m, n);
				break;
			case 'a':	/* counter-clockwise arc */
				fscanf(fp, "%d %d %d %d", &n, &m, &n1, &m1);
				drawarc(n, m, n1, m1);
				break;
			case 'q':	/* spline without end points */
				drawspline(fp, 1);
				lineno++;
				break;
			case '~':		/* wiggly line */
				drawspline(fp, 2);
				lineno++;
				break;
			default:
				error("unknown drawing function %c", c);
				break;
			}
			break;
		case 's':			/* use this point size */
			fscanf(fp, "%d", &size);/* ignore fractional sizes */
			break;

		case 'f':			/* use font mounted here */
			fscanf(fp, "%s", str);
			setfont(t_font(str));
			break;
		case 'H':			/* absolute horizontal motion */
			fscanf(fp, "%d", &n);
			hgoto(n);
			break;
		case 'h':			/* relative horizontal motion */
			fscanf(fp, "%d", &n);
			hmot(n);
			break;
		case 'w':			/* word space */
			break;
		case 'V':			/* absolute vertical position */
			fscanf(fp, "%d", &n);
			vgoto(n);
			break;
		case 'v':			/* relative vertical motion */
			fscanf(fp, "%d", &n);
			vmot(n);
			break;
		case 'p':			/* new page */
			fscanf(fp, "%d", &n);
			t_page(n);
			break;
		case 'n':			/* end of line */
			while((c=getc(fp))!='\n' && c!=EOF);
			hgoto(0);
			lineno++;
			break;
		case '#':			/* comment */
			while((c=getc(fp))!='\n' && c!=EOF);
			lineno++;
			break;
		case 'x':			/* device control function */
			devcntrl(fp);
			lineno++;
			break;
		default:
			error("unknown input character %o %c", c, c);
		}
	}
	t_page(-1);				/* print the last page */
}
/*
 * Interpret device control commands, ignoring any we don't recognize. The
 * "x X ..." commands are a device dependent collection generated by troff's
 * \X'...' request.
 */
void devcntrl(FILE *fp){
	char str[50], buf[256], str1[100];
	int c, n;
	fscanf(fp, "%s", str);
	switch(str[0]){
	case 'f':				/* load font in a position */
		fscanf(fp, "%d %s", &n, str);
		fgets(buf, sizeof buf, fp);	/* in case there's a filename */
		ungetc('\n', fp);		/* fgets() goes too far */
		str1[0] = '\0';			/* in case there's nothing to come in */
		sscanf(buf, "%s", str1);
		loadfont(n, str, str1);
		break;
	case 'i':				/* initialize */
		t_init();
		break;
	case 'p':				/* pause */
		break;
	case 'r':				/* resolution assumed when prepared */
		fscanf(fp, "%lf", &res);
		break;
	case 's':				/* stop */
	case 't':				/* trailer */
		break;
	case 'H':				/* char height */
		fscanf(fp, "%d", &n);
		t_charht(n);
		break;
	case 'S':				/* slant */
		fscanf(fp, "%d", &n);
		t_slant(n);
		break;
	case 'T':				/* device name */
		fscanf(fp, "%s", devname);
		break;

	case 'X':				/* copy through - from troff */
		fscanf(fp, " %[^ \n]", str);
		fgets(buf, sizeof(buf), fp);
		ungetc('\n', fp);
		if(strcmp(str, "color")==0)
			setcolor(buf);
		if(strcmp(str, "bgcolor")==0)
			setbg(buf);
		else if(strcmp(str, "clear")==0)
			setclear();
		else if(strcmp(str, "picfile")==0)
			setpicfile(buf);
		else if(strcmp(str, "clrwin")==0)
			setclrwin(buf);
		else if(strcmp(str, "border")==0)
			setborder(buf);
		break;
    }
    while((c=getc(fp))!='\n' && c!=EOF);

}
/*
 * Load position m with font f. Font file pathname is *fontdir/dev*realdev/*f
 * or name, if name isn't empty.
 */
void loadfont(int m, char *f, char *name){
	char path[150];
	if(name[0]=='\0')
		sprintf(path, "%s/dev%s/%s", fontdir, realdev, f);
	else sprintf(path, "%s", name);
	if(mountfont(path, m)==-1) error("can't load %s at %d", path, m);
	if(smnt==0 && fmount[m]->specfont)
		smnt=m;
	if(m>nfonts){			/* got more positions */
		nfonts=m;
		gotspecial=0;
	}
}
void loadspecial(void){	/* Fix - later. */
	gotspecial = 1;
} 
void t_init(void){
	char path[150];
	static int initialized=0;
	if(!initialized){
		if(strcmp(devname, realdev)){
			sprintf(path, "%s/dev%s/DESC", fontdir, devname);
			if(checkdesc(path))
				realdev=devname;
		}
		sprintf(path, "%s/dev%s/DESC", fontdir, realdev);
		if(getdesc(path)==-1)
			error("can't open %s", path);
		nfonts=0;
		gotspecial=0;
		widthfac=(float)res/devres;
		initialized=1;
	}
	hpos=vpos=0;
	size=10;
}
void t_page(int pg){
	setpage(pagenum);
	if(pg>=0 && in_olist(pg)) pagenum=pg;
	else pagenum=-1;
	hpos=vpos=0;			/* get ready for the next page */
	seenpage=1;
}
/*
 * Converts the string *s into an integer and checks to make sure it's a legal
 * font position. Also arranges to mount all the special fonts after the last
 * legitimate font (by calling loadspecial()), provided it hasn't already been done.
 */
int t_font(char *s){
	int n;
	n = atoi(s);
	if(seenpage){
		if(n<0 || n>nfonts)
			error("illegal font position %d", n);
		if(!gotspecial)
			loadspecial();
	}
	return n;
}
/*
 * Use the font mounted at position m. Bounds checks are probably unnecessary.
 */
void setfont(int m){
	if(m<0 || m>MAXFONTS)
		error("illegal font %d", m);
	curfont=m;
}
void t_charht(int n){
	fontheight=n==size?0:n;
}
/*
 * Set slant to n degrees. Disable slanting if n is 0.
 */
void t_slant(int n){
	fontslant=n;
}
void xymove(int x, int y){
	hgoto(x);
	vgoto(y);
} 
/*
 * Print character c. ASCII if c < ALPHABET, otherwise it's special. Look for
 * c in the current font, then in others starting at the first special font.
 * Restore original font before leaving.
 */
void put1(int c){
	int i, j, k, code, ofont;
	k=ofont=curfont;
	if((i=onfont(c, k))==-1 && smnt>0)
		for(k=smnt, j=0;j<nfonts;j++, k=k%nfonts+1){
			if((i=onfont(c, k))!=-1){
				setfont(k);
				break;
			}
		}
	if(i!=-1 &&(code=fmount[k]->wp[i].code)!=0)
		oput(code);
	if(curfont!=ofont)
		setfont(ofont);
}
/*
 * Print the character whose code is c in the current font and size at the current position.
 */
void oput(int c){
	if(pagenum>=0)
		setchar(c, hpos, vpos);
}
