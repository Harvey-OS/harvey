/*
 * External procedures for all Metafontware programs.
 *
 * Pierre A. MacKay
 * The various routines consolidated into this package 
 * were produced over the past six years, by a number
 * of contributors.  Howard Trickey and Tim Morgan
 * are the principal authors.
 */

#include "mfware.h"

#ifdef _POSIX_SOURCE
#include <string.h>
#else
#ifdef	SYSV
#include <string.h>
extern int sprintf();
#else
#include <strings.h>
extern char *sprintf();
#endif
#endif

#define	TRUE	1
#define	FALSE	0

integer argc;
char *texfontpath;
char *texinputpath;
char *gffontpath;
char *pkfontpath;
extern char *getenv();

static char **gargv;
argv(n, buf)
int n;
char buf[];
{
    (void) strcpy(buf+1, gargv[n]);
    (void) strcat(buf+1, " ");
}

main(ac, av)
char *av[];
{
    argc = ac;
    gargv = av;
    main_body();
    exit(0);
}

/* Same routine as index() aka strchr() */
#ifndef	SYSV
char *char_index(cp, c)
char *cp, c;
{
    while (*cp != c && *cp) ++cp;
    if (*cp) return(cp);
    return(0);
}
#else	/* not SYSV */
#define	char_index	strchr
#endif

/* Open a file; don't return if error */
FILE *openf(name, mode)
char *name, *mode;
{
    FILE *result;
    char *cp;

    /* Sometimes the filenames will have a trailing ' ' */
    cp = char_index(name, ' ');
    if (cp) *cp = '\0';

    result = fopen(name, mode);
    if (result) return(result);
    perror(name);
    exit(1);
    /*NOTREACHED*/
}

/* Print real number r in format n:m. */

printreal(r, n, m)
double r;
int n,m;
{
    char fmt[50];

    (void) sprintf(fmt, "%%%d.%df", n, m);
    (void) printf(fmt, r);
}

/* Print string s in format :n. */
printstring(s, n)
char *s;
int n;
{
    fwrite (s+1, 1, n, stdout);
}

/* Return true on end of line or eof of file, else false */
eoln(f)
FILE *f;
{
    register int c;

    if (feof(f)) return(1);
    c = getc(f);
    if (c != EOF)
	(void) ungetc(c, f);
    return (c == '\n' || c == EOF);
}

#ifdef	ANSI
integer zround(double f)
#else
integer zround(f)
double f;
#endif
{
    if (f >= 0.0) return(f + 0.5);
    return(f - 0.5);
}



/*
 * Return true if we're at the end of the file, else false.
 */
testeof(file)
FILE *file;
{
    int ch;

    if (feof(file))
	return(true);
    if ((ch = getc(file)) == EOF)
	return(true);
    (void) ungetc(ch,file); /* yuck */
    return(false);
}

/* Read an integer in from file f; read past the subsequent end of line */
#ifdef	ANSI
integer inputint(FILE *f)
#else
integer inputint(f)
FILE *f;
#endif
{
    char buffer[20];

    if (fgets(buffer, sizeof(buffer), f)) return(atoi(buffer));
    return(0);
}

zinput_3ints(a,b,c)
integer *a, *b, *c;
{
    while (scanf("%ld %ld %ld\n", a, b, c) != 3)
	(void) fprintf(stderr, "Please enter three integers\n");
}

/*
 * setpaths is called to set up the pointer fontpath
 * as follows:  if the user's environment has a value for 
 * one of: TEXFONTS, PKFONTS, GFFONTS
 * then use it;  otherwise, use defaultfontpath.
 * TEXFONTS is the traditional place for TFM files, and
 * used to be used indiscriminately for raster files as well.
 * It now seems desirable to separate these, although not all sites do.
 */
setpaths()
{
	register char *envpath;
	
	if ((envpath = getenv("TEXFONTS")) != NULL)
	    texfontpath = envpath;
	else
	    texfontpath = TEXFONTS;
            
        if ((envpath = getenv("TEXINPUTS")) != NULL)
	    texinputpath = envpath;
	else
	    texinputpath = TEXINPUTS;

        if ((envpath = getenv("GFFONTS")) != NULL)
	    gffontpath = envpath;
	else
	    gffontpath = TEXFONTS;
            
	if ((envpath = getenv("PKFONTS")) != NULL)
	    pkfontpath = envpath;
	else
	    pkfontpath = TEXFONTS;

}

extern char curname[],realnameoffile[]; 
	/* should have length FILENAMESIZE as given in site.h */

/*
 *	testaccess(amode,filepath)
 *
 *  Test whether or not the file whose name is in the global curname
 *  can be opened for reading (if mode=READACCESS)
 *  or writing (if mode=WRITEACCESS).
 *
 *  The filepath argument is one of the ...FILEPATH constants defined below.
 *  If the filename given in curname does not begin with '/', we try 
 *  prepending all the ':'-separated areanames in the appropriate path to the
 *  filename until access can be made, if it ever can.
 *
 *  The realnameoffile global array will contain the name that yielded an
 *  access success.
 */

#define READACCESS 4
#define WRITEACCESS 2

#define NOFILEPATH 0
#define TEXFONTFILEPATH 3
#define TEXINPUTFILEPATH 2
#define GFFONTFILEPATH 4
#define PKFONTFILEPATH 5

/*
 * packrealnameoffile(cpp) makes realnameoffile contain the directory at *cpp,
 * followed by '/', followed by the characters in curname up until the
 * first blank there, and finally a '\0'.  The cpp pointer is left pointing
 * at the next directory in the path.
 * But: if *cpp == NULL, then we are supposed to use curname as is.
 */
static packrealnameoffile(cpp)
    char **cpp;
{
    register char *p,*realname;
    
    realname = realnameoffile;
    if ((p = *cpp)!=NULL) {
	while ((*p != ':') && (*p != '\0')) {
	    *realname++ = *p++;
	    if (realname == &(realnameoffile[FILENAMESIZE-1]))
		break;
	    }
	if (*p == '\0') *cpp = NULL; /* at end of path now */
	else *cpp = p+1; /* else get past ':' */
	*realname++ = '/';  /* separate the area from the name to follow */
	}
    /* now append curname to realname... */
    p = curname + 1;
    while (*p != ' ') {
	if (realname >= &(realnameoffile[FILENAMESIZE-1])) {
	    (void) fprintf(stderr,"! Full file name `%s' is too long.\n",
                           realname);
	    break;
	    }
	*realname++ = *p++;
	}
    *realname = '\0';
}

testaccess(amode,filepath)
int amode,filepath;
{
    register boolean ok;
    register char *p;
    char *curpathplace;
    int f;
    
    switch(filepath) {
	case NOFILEPATH: curpathplace = NULL; break;
	case TEXFONTFILEPATH: curpathplace = texfontpath; break;
        case TEXINPUTFILEPATH: curpathplace = texinputpath; break;
	case GFFONTFILEPATH: curpathplace = gffontpath; break;
	case PKFONTFILEPATH: curpathplace = pkfontpath; break;
	}
    if (curname[1]=='/')	/* file name has absolute path */
	curpathplace = NULL;
    do {
	packrealnameoffile(&curpathplace);
	if (amode==READACCESS)
	    /* use system call "access" to see if we could read it */
	    if (access(realnameoffile,READACCESS)==0) ok = TRUE;
	    else ok = FALSE;
	else {
	    /* WRITEACCESS: use creat to see if we could create it, but close
	    the file again if we're OK, to let pc open it for real */
	    f = creat(realnameoffile,0666);
	    ok = (f >= 0);
	    if (ok)
		(void) close(f);
	    }
    } while (!ok && curpathplace != NULL);
    if (ok) {  /* pad realnameoffile with blanks, as Pascal wants */
	for (p = realnameoffile; *p != '\0'; p++)
	    /* nothing: find end of string */ ;
	while (p < &(realnameoffile[FILENAMESIZE]))
	    *p++ = ' ';
	}
    return (ok);
}
