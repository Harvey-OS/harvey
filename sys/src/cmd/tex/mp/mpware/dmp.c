/* This program reads device-independent troff output files,
   and converts them into a symbolic form understood by MetaPost.  Some
   of the code was borrowed from DVItoMP.  It understands all the D? graphics
   functions that dpost does but it ignores `x X' device control functions
   such as `x X SetColor:...', `x X BeginPath:', and `x X DrawPath:...'.

   The output file is a sequence of MetaPost picture expressions, one for every
   page in the input file.  It makes no difference where the input file comes
   from, but it is intended to process the result of running eqn and troff on
   the output of MPtoTR.  Such a file contains one page for every btex...etex
   block in the original input.  This program then creates a corresponding
   sequence of MetaPost picture expressions for use as an auxiliary input file.
   Since MetPost expects such files to have the extension .MPX, the output
   is sometimes called an `MPX' file.

   The |banner| string defined here should be changed whenever this program
   gets modified.
*/
char *banner="% Written by DMP, Version 0.0";	/* first line of output */


#include <stdio.h>
#include <math.h>
#include <ctype.h>
#include <site.h>

char *getenv();
char *calloc();
void exit();

#ifndef NULL
#define NULL ((char*) 0)
#endif

#ifndef PI
#define PI  3.14159265358979323846
#endif

#define POOLMAX	5000	/* total characters in all font and char names */
#define FCOUNT	100	/* maximum number of fonts */
#define SHIFTS	100	/* maximum number of characters with special shifts */
#define line_length 79	/* maximum output line length (must be at least 60) */
#define Hprime	307	/* much bigger than max(chars/font,fonts/job) */
#define MAXCHARS 256	/* character codes fall in the range 0..MAXCHARS-1 */
#define LLENGTH 1024	/* one more than maximum line length for troff output */

#define is_specchar(c)	(c<=2)	/* does charcode c identify a special char? */
#define LWscale	0.03	/* line width for graphics as a fraction of pointsize */
#define YCORR 12.0	/* V coordinate of reference point in (big) points */

char strpool[POOLMAX];		/* text for font names and char names */
int poolsize = 0;		/* chars used so far in strpool */

char *texname[FCOUNT];		/* TeX names (null-terminated strings) */
int font_num[FCOUNT];		/* external font numbers */
float font_design_size[FCOUNT];	/* design size in TeX points */
struct hcell *charcodes[FCOUNT];/* hash tables for translating char names */
int next_specfnt[FCOUNT];	/* used to link special fonts together */
float charwd[FCOUNT][MAXCHARS];	/* width/ptsize indexed [font num][char code] */
int nfonts;			/* no. of internal font nums (texname indices)*/
int shiftchar[SHIFTS];		/* charcode of character to shift, else -1 */
float shifth[SHIFTS],shiftv[SHIFTS];	/* shift vals/fontsize (y is upward) */
int shiftptr = 0;		/* number of entries in shift tables */
int shiftbase[FCOUNT];		/* initial index into shifth,shiftv,shiftchar */
int specfnt = FCOUNT;		/* int. num. of first special font (or FCOUNT)*/
int *specf_tail = &specfnt;	/* tail of specfnt list (*specf_tail==FCOUNT) */
FILE *trf;			/* the input file (troff output) */
FILE *mpxf;			/* the output file */
struct hcell *trfonts;		/* hash tab for internal nums of troff fonts */
float unit = 0.0;		/* (big) points per troff unit (0 when unset)*/
int h,v;			/* current position in tr. units (v downward)*/
float cursize;			/* current type size in (big) points */
int curfont;			/* internal number for current font */
float Xslant;			/* degrees additional slant for all fonts */
float Xheight;			/* yscale fonts to this height if nonzero */
char *dbname = "trfonts.map";	/* file for table of troff & TFM font names */
char *adjname = "trchars.adj";	/* file for character shift amounts */
char *tfmpath = TEXFONTS;	/* directories to search for TFM files */
char *dbpath = MPPOOL;		/* directories for font map data, spec chars */
char *trpath = "/usr/lib/font/devpost";	/* search path for troff font data */
int lnno = 0;			/* line num. in troff output file (our input) */


void quit(msg1,msg2,msg3)
    char *msg1, *msg2, *msg3;
{
    fprintf(stderr,"DMP abort at troff output line %d:\n%s%s%s\n",
	lnno, msg1, msg2, msg3);
    exit(1);
}

void warn(msg1,msg2,msg3)
    char *msg1, *msg2, *msg3;
{
    fprintf(stderr,"DMP warning at troff output line %d:\n%s%s%s\n",
	lnno, msg1, msg2, msg3);
}

void add_to_pool(c)
    char c;
{
    if (poolsize==POOLMAX) quit("Need to increase POOLMAX","","");
    else strpool[poolsize++] = c;
}




/**************************************************************
			Hash tables
***************************************************************/

typedef struct hcell {
    char* lab;
    int datum;
} Hcell;

#define new_htab   (Hcell*) calloc((unsigned)Hprime, (unsigned)sizeof(Hcell))


int hash(s)
    register char *s;
{
    register r;
    for(r=0; *s!=0; s++) {
	r = (r<<1) + *s;
	while (r>=Hprime) r-=Hprime;
    }
    return r;
}


/* Find or insert the given string in the hash table and return the address
   of the datum. 
*/
Hcell *failure;			/* null unless last hfind failed (used below) */

int *hfind(s,htab)
    char *s;
    Hcell* htab;
{
    register Hcell *p;
    register cnt = Hprime;
    failure = (Hcell *) 0;
    p = &htab[hash(s)];
    do {
	if (p->lab==NULL) {
	    failure = p;
	    if (s<&strpool[0] || s>=&strpool[POOLMAX]) {
		p->lab = &strpool[poolsize];
		do add_to_pool(*s); while (*s++!='\0');
	    } else p->lab = s;
	    return &p->datum;
	}
	if (strcmp(s, p->lab)==0)
	    return &p->datum;
	if (p==htab) p= &htab[Hprime-1];
	else p--;
    } while (--cnt >= 0);
    quit("Need to increase Hprime","","");
    return (int*)0;		/* keep lint happy */
}


/* If the last hfind() failed, undo the insertion and return zero (FALSE).
*/
int hfound()
{
    if (failure==(Hcell *)0) return 1;
    failure->lab = NULL;
    return 0;
}



/**************************************************************
			Search Paths
***************************************************************/

#ifdef MS_DOS
#define PATH_DELIM ';'
#else
#define PATH_DELIM ':'
#endif

void setpaths()
{
	char *envpath;
	
	if ((envpath = getenv("TEXFONTS")) != NULL)
	    tfmpath = envpath;
	if ((envpath = getenv("MPPOOL")) != NULL)
	    dbpath = envpath;
}

FILE *fsearch(nam, ext, path)
    char *nam, *ext, *path;
{
    FILE *f;
    char *s, *p;

    if (nam[0]=='/')	/* file name has absolute path */
	path = NULL;
    do {
	s = &strpool[poolsize];
	if (path!=NULL) {
	    while ((*path!=PATH_DELIM) && (*path!='\0'))
		add_to_pool(*path++);
	    if (*path=='\0') path=NULL; else path++;
	    add_to_pool('/');
	}
	for (p=nam; *p!='\0'; p++) add_to_pool(*p);
	for (p=ext; *p!='\0'; p++) add_to_pool(*p);
	add_to_pool('\0');
	f = fopen(s, "r");
	poolsize = s - &strpool[0];
    } while (f==NULL && path!=NULL);
    if (f==NULL) quit("Cannot find ",nam,ext);
    return f;
}



/**************************************************************
			Scanning Numbers
***************************************************************/

/* The standard functions atoi(), atof(), and sscanf() provide ways of reading
   numbers from strings but they give no indication of how much of the string
   is consumed.  These homemade versions don't parse scientific notation.
*/
char *arg_tail;		/* char after the number just gotten; NULL on failure */


int get_int(s)
    register char *s;
{
    register i, d, neg;
    if (s==NULL) goto bad;
    for (neg=0;; s++)
	if (*s=='-') neg=!neg;
	else if (*s!=' ' && *s!='\t') break;
    if (i= *s-'0', 0>i||i>9) goto bad;
    while (d= *++s-'0', 0<=d&&d<=9)
	i = 10*i + d;
    arg_tail = s;
    return neg ? -i : i;
bad:arg_tail = NULL;
    return 0;
}


/* Troff output files contain few if any non-integers, but this program is
   prepared to read floats whenever they seem reasonable; i.e., when the
   number is not being used for character positioning.  (For non-PostScript
   applications h and v are usually in pixels and should be integers.)
*/
float get_float(s)
    register char *s;
{
    register d, neg, digits;
    register float x, y;

    digits = 0;
    if (s!=NULL) {
	for (neg=0;; s++)
	    if (*s=='-') neg=!neg;
	    else if (*s!=' ' && *s!='\t') break;
	x = 0.0;
	while (d= *s-'0', 0<=d&&d<=9) {
	    x = 10.0*x + d;
	    digits++; s++;
	}
	if (*s=='.') {
	    y = 1.0;
	    while (d= *++s-'0', 0<=d&&d<=9) {
		y /= 10.0;
		x += y*d;
		digits++;
	    }
	}
    }
    if (digits==0) {
	arg_tail = NULL;
	return 0.0;
    }
    arg_tail = s;
    return neg ? -x : x;
}



/**************************************************************
		Reading Initialization Files
***************************************************************/

/* Read the database file, reserve internal font numbers and set
   the texname[] entries.  Each line in the database file contains
   <troff-name>\t,PostScript-name>\t<TeX-name>
   or just <troff-name>\t,PostScript-name> if the TeX name matches the
   PostScript name. ("\t" means one or more tabs.)
*/

void read_fmap(dbase)
    char *dbase;
{
    FILE *fin;
    int c;			/* last character read */
    char *nam;			/* a font name being read */

    nfonts = 0;
    fin = fsearch(dbase,"",dbpath);
    trfonts = new_htab;
    while ((c=getc(fin))!=EOF) {
	if (nfonts==FCOUNT) quit("Need to increase FCOUNT","","");
	nam = &strpool[poolsize];
	for (;c!='\t'; c=getc(fin)) add_to_pool(c);
	add_to_pool(0);
	*hfind(nam, trfonts) = nfonts;
	texname[nfonts] = &strpool[poolsize];
	do {
	    poolsize = texname[nfonts] - strpool;
	    do c=getc(fin); while (c=='\t');
	    for (;c!='\t' && c!='\n'; c=getc(fin))
		add_to_pool(c);
	    add_to_pool(0);
	} while (c=='\t');
	font_num[nfonts] = -1;		/* indicate font is not mounted */
	nfonts++;
    }
    fclose(fin);
}


/* Some characters need their coordinates shifted in order to agree with
   troff's view of the world.  Logically, this information belongs in the
   font description files but it actually resides in a PostScript prolog
   that the troff output processor dpost reads.  Since that file is in
   PostScript and subject to change, we read the same information from
   a small auxiliary file that gives shift amounts relative to the font
   size with y upward.
*/
void read_char_adj(adjfile)
    char *adjfile;
{
    FILE* fin;
    char buf[200];
    int i;

    fin = fsearch(adjfile, "", dbpath);
    for (i=0; i<nfonts; i++)
	shiftbase[i] = 0;
    while (fgets(buf,200,fin)!=NULL) {
	if (shiftptr==SHIFTS-1) quit("Need to increase SHIFTS","","");
	if (buf[0]!=' ' && buf[0]!='\t') {
	    for (i=0; buf[i]!='\0'; i++)
		if (buf[i]=='\n') buf[i]='\0';
	    shiftchar[shiftptr++] = -1;
	    shiftbase[*hfind(buf,trfonts)] = shiftptr;
	    if (!hfound()) quit(adjfile," refers to unknown font ",buf);
	} else {
	    shiftchar[shiftptr] = get_int(buf);
	    shifth[shiftptr] = get_float(arg_tail);
	    shiftv[shiftptr] = -get_float(arg_tail);
	    if (arg_tail==NULL) quit("Bad shift entry : \"",buf,"\"");
	    shiftptr++;
	}
    }
    shiftchar[shiftptr++] = -1;
    fclose(fin);
}


/**************************************************************
			Reading Font Files
***************************************************************/


/* Read the TFM file for the font with internal number f, updating the
   data structures appropriately.  We get the character widths out of the
   tfm file instead of the troff font description file because they don't
   because the latter source reflects alterations made only by dpost (the
   troff output driver that is bypassed when using MetaPost).
*/
void read_tfm(f)
{
    FILE* tf;
    long a = 0;
    int sizes[5];	/* file & header lengths, bc, ec, words in wd table */
    long wd[256];	/* the width table (font size relative, scaled 2^20) */
    int i, j;

    tf = fsearch(texname[f], ".tfm", tfmpath);
    for (i=0; i<5; i++) {
	sizes[i] = getc(tf);
	sizes[i] = (sizes[i]<<8) | (getc(tf) & 0377);
    }
    if (sizes[1]<2 || sizes[2]<0 || sizes[3]<sizes[2]-1 || sizes[3]>255
	    || sizes[0]<sizes[1]+sizes[3]-sizes[2]+sizes[4]+7)
	quit("Bad tfm file: ",texname[f],".tfm");
    fseek(tf,28L,0);
    for (i=0; i<4; i++)
	a = (a<<8) | (long) (getc(tf) & 0377);
    font_design_size[f] = ((float) a)/1048576.0;
    fseek(tf, (long)(28+4*(sizes[1]+sizes[3]-sizes[2])), 0);
    for (i=0; i<sizes[4]; i++) {
	wd[i] = 0L;
	for (j=0; j<4; j++)
	    wd[i] = (wd[i]<<8) | (long) (getc(tf) & 0377);
    }
    fseek(tf, (long)(24+4*sizes[1]), 0);
    for (i=sizes[2]; i<=sizes[3]; i++) {
	charwd[f][i] = wd[getc(tf) & 0377]/1048576.0/unit;
	fseek(tf, 3L, 1);
    }
    fclose(tf);
}


/* Given one line from the character description file for the font with
   internal number f, save the appropriate data in the charcodes[f] table.
   A return value of zero indicates a syntax error.
*/
int scan_desc_line(f, lin)
    char *lin;
{
    static lastcode;
    char *s;

    s = &strpool[poolsize];
    while (*lin!=' ' && *lin!='\t' && *lin!='\0')
	add_to_pool(*lin++);
    add_to_pool('\0');
    while (*lin==' ' || *lin=='\t') lin++;
    if (*lin=='"')
	*hfind(s,charcodes[f]) = lastcode;
    else {
	(void) get_float(lin);
	(void) get_int(arg_tail);
	lastcode = get_int(arg_tail);
	if (arg_tail==NULL) return 0;
	*hfind(s,charcodes[f]) = lastcode;
	if (lastcode<0 || lastcode>=MAXCHARS) return 0;
    }
    return 1;
}

/* Read the font description file for the font with the given troff name
   and update the data structures.  The result is the internal font number.
*/
int read_fontdesc(nam)
    char *nam;			/* troff name */
{
    char buf[200];
    FILE* fin;			/* input file */
    int f;			/* internal font number */

    if (unit==0.0) quit("Resolution is not set soon enough","","");
    f = *hfind(nam, trfonts);
    if (!hfound())
	quit("Font was not in map file","","");
    fin = fsearch(nam, "", trpath);
    for (;;) {
	if (fgets(buf,200,fin)==NULL)
	    quit("Description file for ",nam," ends unexpectedly");
	if (strncmp(buf,"special",7)==0) {
	    *specf_tail = f;
	    next_specfnt[f] = FCOUNT;
	    specf_tail = &next_specfnt[f];
	} else if (strncmp(buf,"charset",7)==0)
	    break;
    }
    charcodes[f] = new_htab;
    while (fgets(buf,200,fin)!=NULL)
	if (scan_desc_line(f, buf) == 0)
	    quit(nam," has a bad line in its description file: ",buf);
    fclose(fin);
    return f;
}



/**************************************************************
		Low Level Output Routines
***************************************************************/

/* One of the basic output operations is to write a \MP\ string expression for
   a sequence of characters to be typeset.  The main difficulties are that such
   strings can contain arbitrary eight-bit bytes and there is no fixed limit on
   the length of the string that needs to be produced.  In extreme cases this
   can lead to expressions such as
	char7&char15\&char31&"?FWayzz"
	&"zzaF"&char15&char3&char31
	&"Nxzzzzzzzwvtsqo"

   A global variable state keeps track of the output process.
   When state=normal we have begun a quoted string and the next character
   should be a printable character or a closing quote.  When state=special
   the last thing printed was a `char' construction or a closing quote
   and an ampersand should come next.  The starting condition state=initial
   is a lot like state=special, except no ampersand is required.
*/
#define special	0	/* the state after printing a `char' expression */
#define normal	1	/* the state value in a quoted string */
#define initial	2	/* initial state */

int state = initial;
int print_col = 0;	/* there are at most this many characters on the current line */


/* To print a string on the MPX file, initialize print_col, ensure that
   state=initial, and pass the characters one-at-a-time to print_char.
*/
void print_char(cc)
    char cc;
{
    int printable;	/* nonzero if it is safe to print c */
    int l;		/* number of chars in c or the `char' expression */
    int c;		/* equal to cc mod 256, but always positive */

    c = cc&0377;
    printable = isprint(c);
    if (printable) l=1;
    else if (c<10) l=5;
    else if (c<100) l=6;
    else l=7;
    if (print_col+l>line_length-2) {
	if (state==normal) {
	    putc('"',mpxf);
	    state = special;
	}
	putc('\n',mpxf);
	print_col = 0;
    }
    if (state==normal)
	if (printable) putc(c,mpxf);
	else {
	    fprintf(mpxf,"\"&char%d",c);
	    print_col += 2;
	}
    else {
	if (state==special) {
	    putc('&',mpxf);
	    print_col++;
	}
	if (printable) {
	    fprintf(mpxf,"\"%c",c);
	    print_col++;
	} else
	    fprintf(mpxf,"char%d",c);
    }
    print_col += l;
    state = printable ? normal : special;
}


/* The end_char_string procedure gets the string ended properly and ensures
   that there is room for |l| more characters on the output line.
*/
void end_char_string(l)
{
    while (state>special) {
	putc('"',mpxf);
	print_col++;
	state--;
    }
    if (print_col+l>line_length) {
	putc('\n',mpxf);
	print_col = 0;
    }
    state = initial;		/* get ready to print the next string */
}



/**************************************************************
		Page and Character Output
***************************************************************/

char font_used[FCOUNT];		/* nonzero for fonts used on this page */
int fonts_used;			/* nonzero if any font been used on this page */
int graphics_used;		/* nonzero if any graphics seen on this page */
float str_h1,str_v;		/* corrected start pos for current out string */
float str_h2;			/* where the current output string ends */
int str_f;			/* internal font num for cur. output string */
float str_size;			/* point size for this text string */


/* Before using any fonts we need to define a MetaPost macro for typesetting
   character strings.
*/
void prepare_font_use()
{
    int k;

    for (k=0;k<nfonts;k++) font_used[k]=0;
    fonts_used = 1;
    fprintf(mpxf,"string n[];\n");
    fprintf(mpxf,"vardef s(expr t,m,x,y) =\n");
    fprintf(mpxf,"  addto p also t scaled(m*1.00375) shifted(x,y); enddef;\n");
}


/* Do what is necessary when the font with internal number f is used for the
   first time on a page.
*/
void first_use(f)
{
    font_used[f] = 1;
    fprintf(mpxf, "n%d=\"%s\";\n", font_num[f], texname[f]);
}


/* Print any transformations required by the current Xslant and Xheight
   settings.
*/
void slant_and_ht()
{
    int i = 0;

    if (Xslant!=0.0) {
	fprintf(mpxf," slanted%.5f",Xslant);
	i++;
    }
    if (Xheight!=cursize && Xheight!=0.0 && cursize!=0.0) {
	fprintf(mpxf," yscaled%.4f", Xheight/cursize);
	i++;
    }
    if (i>0) fprintf(mpxf, "\n ");
}


/* We maintain the invariant that str_f is -1 when there is no output string
   under construction.
*/
void finish_last_char()
{
    float m,x,y;	/* font scale, MetaPost coords of reference point */

    if (str_f>=0) {
	m = str_size/font_design_size[str_f];
	x = str_h1*unit;
	y = YCORR-str_v*unit;
	if (fabs(x)>=4096.0 || fabs(y)>=4096.0 || m>=4096.0 || m<0) {
	    warn("text out of range ignored","","");
	    end_char_string(67);
	}
	else end_char_string(47);
	fprintf(mpxf,")infont n%d", font_num[str_f]);
	slant_and_ht();
	fprintf(mpxf,",%.5f,%.4f,%.4f);\n", m, x,y);
	str_f = -1;
    }
}


/* Output character number c in the font with internal number f.
*/
void set_num_char(f,c)
{
    float hh, vv;		/* corrected versions of h, v */
    int i;

    hh = h;
    vv = v;
    for (i=shiftbase[f]; shiftchar[i]>=0; i++)
	if (shiftchar[i]==c) {
	    hh += (cursize/unit)*shifth[i];
	    vv += (cursize/unit)*shiftv[i];
	    break;
	}
    if (c==0) quit("attempt to typeset an invalid character","","");
    if (hh-str_h2>=1.0 || str_h2-hh>=1.0 || vv-str_v>=1.0 || str_v-vv>=1.0
	    || f!=str_f || cursize!=str_size) {
	if (str_f>=0) finish_last_char();
	else if (!fonts_used)
	    prepare_font_use();	/* first font usage on this page */
	if (!font_used[f])
	    first_use(f);	/* first use of font f on this page */
	fprintf(mpxf,"s((");
	print_col = 3;
	str_f=f; str_v=vv; str_h1=hh;
	str_size = cursize;
    }
    print_char(c);
    str_h2 = hh + cursize*charwd[f][c];
}


/* The following initialization and clean-up is required.
*/
void start_picture()
{
    fonts_used = graphics_used = 0;
    str_f = -1;
    str_v = 0.0;
    str_h2 = 0.0;
    str_size = 0.0;
    fprintf(mpxf,"begingroup save C,D,p,s,n; picture p; p=nullpicture;\n");
}

void stop_picture()
{
    if (str_f>=0) finish_last_char();
    fprintf(mpxf,"p endgroup\n");
}



/**************************************************************
			Special Characters
***************************************************************/

/* Given the troff name of a special character, this routine finds its
   definition and copies it to the MPX file.  It also finds the name of
   the vardef macro, puts it in the string pool, and index where the
   string starts.  The name should be C.<something>.
*/
char specintro[] = "vardef ";		/* MetaPost name follows this */
#define speci 7				/* length of the above string */

int copy_spec_char(cname)
    char *cname;
{
    int k = 0;				/* how much of specintro so far */
    FILE *deff;
    int c, s;

    deff = fsearch("charlib/",cname, dbpath);
    while (k<speci) {
	if ((c=getc(deff))==EOF)
	    quit("No vardef in charlib/",cname,"");
	putc(c, mpxf);
	if (c==specintro[k]) k++; else k=0;
    }
    s = poolsize;
    while ((c=getc(deff))!='(') {
	if (c==EOF) quit("vardef in charlib/",cname," has no arguments");
	putc(c, mpxf);
	add_to_pool(c);
    }
    putc(c, mpxf);
    add_to_pool('\0');
    while ((c=getc(deff))!=EOF)
	putc(c, mpxf);
    return s;
}


/* When given a character name instead of a number, we need to check if
   it is a special character and download the definition if necessary.
   If the character is not in the current font we have to search the special
   fonts.
*/
Hcell *spec_tab = (Hcell*)0;

void set_char(cname)
    char *cname;
{
    int f, c, *flagp;

    if (*cname==' '||*cname=='\t') return;
    f = curfont;
    c = *hfind(cname, charcodes[f]);
    if (!hfound()) {
	for (f=specfnt; f!=FCOUNT; f=next_specfnt[f]) {
	    c = *hfind(cname, charcodes[f]);
	    if (hfound()) goto out;
	}
	quit("There is no character ",cname,"");
    }
out:if (!is_specchar(c)) set_num_char(f,c);
    else {
	if (str_f>=0) finish_last_char();
	if (!fonts_used) prepare_font_use();
	if (!font_used[f]) first_use(f);
	if (spec_tab==(Hcell*)0)
	    spec_tab = new_htab;
	flagp = hfind(cname, spec_tab);
	if (*flagp==0)
	    *flagp = copy_spec_char(cname);	/* this won't be zero */
	fprintf(mpxf, "s(%s(n%d)", &strpool[*flagp], font_num[f]);
	slant_and_ht();
	fprintf(mpxf, ",%.5f,%.4f,%.4f);\n",
	    cursize/font_design_size[f], h*unit, YCORR-v*unit);
    }
}



/**************************************************************
			Font Definitions
***************************************************************/

/* Mount the font with troff name nam at external font number n and read any
   necessary font files.
*/
void do_font_def(n, nam)
    char *nam;
{
    int f, k;

    f = *hfind(nam, trfonts);
    if (!hfound())
	quit("Font ",nam," was not in map file");
    if (font_design_size[f]==0) {
	read_fontdesc(nam);
	read_tfm(f);
    }
    for (k=0; k<nfonts; k++)
	if (font_num[k]==n) font_num[k]= -1;
    font_num[f] = n;
}



/**************************************************************
		Time on `makepath pencircle'
***************************************************************/

#define Speed	((float) (PI/4.0))

/* Given the control points of a cubic Bernstein polynomial, evaluate
   it at t.
*/
float Beval(xx, t)
    float *xx, t;
{
    float zz[4];
    register i, j;
    for (i=0; i<=3; i++) zz[i]=xx[i];
    for (i=3; i>0; i--)
	for (j=0; j<i; j++)
	    zz[j] += t*(zz[j+1]-zz[j]);
    return zz[0];
}


/* Find the direction angle at time t on the path `makepath pencircle'.
   The tables below give the Bezier control points for MetaPost's cubic
   approximation to the first octant of a unit circle.
*/
float xx[4] = {1.0, 1.0, 0.8946431597, 0.7071067812};
float yy[4] = {0.0, 0.2652164899, 0.5195704026, 0.7071067812};

float circangle(t)
    float t;
{
    float ti;

    ti = floor(t);
    t -= ti;
    return (float) atan(Beval(yy,t)/Beval(xx,t)) + ti*Speed;
}


/* Find the spline parameter where `makepath pencircle' comes closest to
   (cos(a)/2,sin(a)/2).
*/
float circtime(a)
    float a;
{
    int i;
    float t;
    t = a/Speed;
    for (i=2; --i>=0;)
	t += (a - circangle(t))/Speed;
    return t;
}



/**************************************************************
			Troff Graphics
***************************************************************/

float gx, gy;		/* current point for graphics (init. (h,YCORR/unit-v) */

void prepare_graphics()
{
    fprintf(mpxf,"vardef D(expr d)expr q =\n");
    fprintf(mpxf," addto p doublepath q withpen pencircle scaled d; enddef;\n");
    graphics_used = 1;
}


/* This function prints the current position (gx,gy).  Then if it can read dh dv
   from string s, it increments (gx,gy) and prints "--".  By returning the rest
   of the string s or NULL if nothing could be read from s, it provides the
   argument for the next iteration.
*/
char *do_line(s)
    char *s;
{
    float dh, dv;

    fprintf(mpxf, "(%.3f,%.3f)", gx*unit, gy*unit);
    dh = get_float(s);
    dv = get_float(arg_tail);
    if (arg_tail==NULL) return NULL;
    gx += dh;
    gy -= dv;
    fprintf(mpxf,"--\n");
    return arg_tail;
}


/* Function spline_seg() reads two pairs of (dh,dv) increments and prints the
   corresponding quadratic B-spline segment, leaving the ending point to be
   printed next time.  The return value is the string with the first (dh,dv)
   pair lopped off.  If only one pair of increments is found, we prepare to
   terminate the iteration by printing last time's ending point and returning
   NULL.
*/
char * spline_seg(s)
    char *s;
{
    float dh1, dv1, dh2, dv2;

    dh1 = get_float(s);
    dv1 = get_float(arg_tail);
    if (arg_tail==NULL) quit("Missing spline increments","","");
    s = arg_tail;
    fprintf(mpxf, "(%.3f,%.3f)", (gx+.5*dh1)*unit, (gy-.5*dv1)*unit);
    gx += dh1;
    gy -= dv1;
    dh2 = get_float(s);
    dv2 = get_float(arg_tail);
    if (arg_tail==NULL) return NULL;
    fprintf(mpxf, "..\ncontrols (%.3f,%.3f) and (%.3f,%.3f)..\n",
	(gx-dh1/6.0)*unit, (gy+dv1/6.0)*unit, (gx+dh2/6.0)*unit,
	(gy-dv2/6.0)*unit);
    return s;
}


/* Draw an ellipse with the given major and minor axes.
*/
void do_ellipse(a, b)
    float a, b;
{
    fprintf(mpxf, "makepath(pencircle xscaled %.3f\n yscaled %.3f",
	a*unit, b*unit);
    fprintf(mpxf, " shifted (%.3f,%.3f));\n", (gx+.5*a)*unit, gy*unit);
    gx += a;
}


/* Draw a counter-clockwise arc centered at (cx,cy) with initial and final radii
   (ax,ay) and (bx,by) respectively.
*/
void do_arc(cx, cy, ax, ay, bx, by)
    float cx, cy, ax, ay, bx, by;
{
    float t1, t2;

    t1 = circtime(atan2(ay, ax));
    t2 = circtime(atan2(by, bx));
    if (t2 < t1) t2 += 8.0;
    fprintf(mpxf, "subpath (%.5f,%.5f) of\n", t1, t2);
    fprintf(mpxf, " makepath(pencircle scaled %.3f shifted (%.3f,%.3f));\n",
	2.0*sqrt(ax*ax+ay*ay)*unit, cx*unit, cy*unit);
    gx = cx + bx;
    gy = cy + by;
}



/* string s is everything following the initial `D' in a troff graphics command.
*/
void do_graphic(s)
    char *s;
{
    float h1, v1, h2, v2;
    gx = (float) h;
    gy = YCORR/unit - ((float) v);
    if (!graphics_used) prepare_graphics();
    fprintf(mpxf,"D(%.4f) ", LWscale*cursize);
    switch (*s++) {
    case 'c':
	h1 = get_float(s);
	if (arg_tail==NULL) quit("Bad argument in ",s-2,"");
	do_ellipse(h1,h1);
	break;
    case 'e':
	h1 = get_float(s);
	v1 = get_float(arg_tail);
	if (arg_tail==NULL) quit("Bad argument in ",s-2,"");
	do_ellipse(h1,v1);
	break;
    case 'A':
	fprintf(mpxf, "reverse ");
	/* fall through */
    case 'a':
	h1 = get_float(s);
	v1 = get_float(arg_tail);
	h2 = get_float(arg_tail);
	v2 = get_float(arg_tail);
	if (arg_tail==NULL) quit("Bad argument in ",s-2,"");
	do_arc(gx+h1, gy-v1, -h1, v1, h2, -v2);
	break;
    case 'l': case 'p':
	while (s!=NULL)
	    s = do_line(s);
	fprintf(mpxf, ";\n");
	break;
    case 'q':
	do s = spline_seg(s); while (s!=NULL);
	fprintf(mpxf, ";\n");
	break;
    case '~':
	fprintf(mpxf, "(%.3f,%.3f)--", gx*unit, gy*unit);
	do s = spline_seg(s); while (s!=NULL);
	fprintf(mpxf, "--(%.3f,%.3f);\n", gx*unit, gy*unit);
	break;
    default:
	quit("Unknown drawing function",s-2,"");
    }
    h = (int) floor(gx+.5);
    v = (int) floor(YCORR/unit+.5-gy);
}



/**************************************************************
		Interpreting Troff Output
***************************************************************/

void change_font(f)
{
    for (curfont=0; curfont<nfonts; curfont++)
	if (font_num[curfont]==f) return;
    quit("Bad font setting","","");
}


/* String s0 is everything following the initial `x' in a troff device control
   command.  A zero result indicates a stop command.
*/
int do_x_cmd(s0)
    char *s0;
{
    float x;
    int n;
    char *s;

    s = s0;
    while (*s==' ' || *s=='\t') s++;
    switch (*s++) {
    case 'r':
	if (unit!=0.0) quit("Attempt to reset resolution","","");
	while (*s!=' ' && *s!='\t') s++;
	unit = get_float(s);
	if (unit<=0.0) quit("Bad resolution: x",s0,"");
	unit = 72.0/unit;
	break;
    case 'f':
	while (*s!=' ' && *s!='\t') s++;
	n = get_int(s);
	if (arg_tail==NULL) quit("Bad font def: x",s0,"");
	s = arg_tail;
	while (*s==' ' || *s=='\t') s++;
	do_font_def(n, s);
	break;
    case 's':
	return 0;
    case 'H':
	while (*s!=' ' && *s!='\t') s++;
	Xheight = get_float(s);
	if (Xheight==cursize) Xheight=0.0;
	break;
    case 'S':
	while (*s!=' ' && *s!='\t') s++;
	Xslant = get_float(s)*(PI/180.0);
	x = cos(Xslant);
	if (-1e-4<x && x<1e-4) quit("Excessive slant","","");
	Xslant = sin(Xslant)/x;
	break;
    default:
	/* do nothing */;
    }
    return 1;
}


/* This routine reads commands from the troff output file up to and including
   the next `p' or `x s' command.  It also calls set_num_char() and set_char()
   to generate output when appropriate.  A zero result indicates that there
   are no more pages to do.
*/
int do_page()
{
    char buf[LLENGTH];
    char a, *c, *cc;

    h = v = 0;
    while (fgets(buf,LLENGTH,trf)!=NULL) {
	for (c=buf; *c!='\n'; c++)
	    if (*c=='\0') quit("Need to increase LLENGTH","","");
	*c = '\0';
	lnno++;
	c = buf;
	while (*c!='\0') {
	    switch(*c) {
	    case ' ': case '\t': case 'w':
		c++;
		break;
	    case 's':
		cursize = get_float(c+1);
		goto iarg;
	    case 'f':
		change_font(get_int(c+1));
		goto iarg;
	    case 'c':
		if (c[1]=='\0') quit("Bad c command in troff output","","");
		cc = c+2;
		goto set;
	    case 'C':
		cc=c; do cc++; while (*cc!=' ' && *cc!='\t' && *cc!='\0');
		goto set;
	    case 'N':
		set_num_char(curfont, get_int(c+1));
		goto iarg;
	    case 'H':
		h = get_int(c+1);
		goto iarg;
	    case 'V':
		v = get_int(c+1);
		goto iarg;
	    case 'h':
		h += get_int(c+1);
		goto iarg;
	    case 'v':
		v += get_int(c+1);
		goto iarg;
	    case '0': case '1': case '2': case '3': case '4':
	    case '5': case '6': case '7': case '8': case '9':
		if (c[1]<'0' || c[1]>'9' || c[2]=='\0')
		    quit("Bad nnc command in troff output","","");
		h += 10*(c[0]-'0') + c[1]-'0';
		c++;
		cc = c+2;
		goto set;
	    case 'p':
		return 1;
	    case 'n':
		(void) get_int(c+1);
		(void) get_int(arg_tail);
		goto iarg;
	    case 'D':
		do_graphic(c+1);
		goto eoln;
	    case 'x':
		if (!do_x_cmd(c+1)) return 0;
		goto eoln;
	    case '#':
		goto eoln;
	    default:
		quit("Bad command in troff output","","");
	    }
	    continue;
	set:a= *cc; *cc='\0';
	    set_char(++c);
	    c = cc;
	    *c = a;
	    continue;
	iarg:c = arg_tail;
	}
    eoln:/* do nothing */;
    }
    return 0;
}


/**************************************************************
			Main Program
***************************************************************/

void usage(s)
    char *s;
{
    fprintf(stderr, "Usage: %s [ input_file [ mpx_file ]]\n", s);
    exit(2);
}


void main(argc, argv)
    char *argv[];
{
    int more;

    setpaths();
    trf = stdin;
    mpxf = stdout;
    if (argc>3) usage(argv[0]);
    if (argc>1) {
	trf = fopen(argv[1], "r");
	if (trf==(FILE*)0) quit("Cannot read ",argv[1],"");
	if (argc>2) {
	    mpxf = fopen(argv[2], "w");
	    if (mpxf==(FILE*)0) quit("Cannot write ",argv[2],"");
	}
    }
    fprintf(mpxf, "%s\n", banner);
    read_fmap(dbname);
    read_char_adj(adjname);
    if (do_page()) {
	do {
	    h=0; v=0;
	    Xslant = Xheight = 0.0;
	    start_picture();
	    more = do_page();
	    stop_picture();
	    fprintf(mpxf,"mpxbreak\n");
	} while (more);
    }
    exit(0);
}

