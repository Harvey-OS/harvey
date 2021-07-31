/* External procedures for dvitype.
Originally written by: Howard Trickey, 2/19/83 (adapted from TeX's ext.c).
Common procedures to texware.c removed: Karl Berry, 11/30/89.
*/

#include <stdio.h>
#include "site.h"

#define	TRUE	1
#define	FALSE	0

#ifdef	ANSI
void setpaths(void);
int testaccess(int,int);
void packrealnameoffile(char * *);
extern void exit(int);
extern char *getenv(char*);
extern int creat(char *, int);
extern int access(char *, int);
extern int close(int);
#else /* ! ANSI */
extern char *getenv();
void packrealnameoffile();
#endif /* ANSI */

#ifdef MS_DOS
#define PATH_DELIM ';'
#else
#define PATH_DELIM ':'
#endif

static char *fontpath;

/*
 * setpaths is called to set up the pointer fontpath
 * as follows:  if the user's environment has a value for TEXFONTS
 * then use it;  otherwise, use defaultfontpath.
 */
void setpaths()
{
	register char *envpath;
	
	if ((envpath = getenv("TEXFONTS")) != NULL)
	    fontpath = envpath;
	else
	    fontpath = TEXFONTS;
}

extern char curname[],realnameoffile[]; /* these have size FILENAMESIZE. */

/*
 *	testaccess(amode,filepath)
 *
 *  Test whether or not the file whose name is in the global curname
 *  can be opened for reading (if mode=READACCESS)
 *  or writing (if mode=WRITEACCESS).
 *
 *  The filepath argument is one of the ...FILEPATH constants defined below.
 *  If the filename given in curname does not begin with '/', we try 
 *  prepending all the ':'-separated* areanames in the appropriate path to the
 *  filename until access can be made, if it ever can.
 *
 *  The realnameoffile global array will contain the name that yielded an
 *  access success.
 *
 *  This now uses the constant PATH_DELIM #define'd above as DOS uses a ';'
 *  to separate path elements.
 */

#define READACCESS 4
#define WRITEACCESS 2

#define NOFILEPATH 0
#define FONTFILEPATH 3

testaccess(amode,filepath)
int amode,filepath;
{
    register boolean ok;
    register char *p;
    char *curpathplace;
    int f;
    
    switch(filepath) {
	case NOFILEPATH: curpathplace = NULL; break;
	case FONTFILEPATH: curpathplace = fontpath; break;
	}
    if (curname[0]=='/')	/* file name has absolute path */
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
	    ok = (char) (f >= 0);
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

/*
 * packrealnameoffile(cpp) makes realnameoffile contain the directory at *cpp,
 * followed by '/', followed by the characters in curname up until the
 * first blank there, and finally a '\0'.  The cpp pointer is left pointing
 * at the next directory in the path.
 * But: if *cpp == NULL, then we are supposed to use curname as is.
 */

void
packrealnameoffile(cpp)
    char **cpp;
{
    register char *p,*realname;
    
    realname = realnameoffile;
    if ((p = *cpp)!=NULL) {
	while ((*p != PATH_DELIM) && (*p != '\0')) {
	    *realname++ = *p++;
	    if (realname == &(realnameoffile[FILENAMESIZE-1]))
		break;
	    }
	if (*p == '\0') *cpp = NULL; /* at end of path now */
	else *cpp = p+1; /* else get past PATH_DELIM */
	*realname++ = '/';  /* separate the area from the name to follow */
	}
    /* now append curname to realname... */
    p = curname + 1;
    while (*p != ' ') {
	if (realname >= &(realnameoffile[FILENAMESIZE-1])) {
	    (void) fprintf(stderr,"! Full file name is too long\n");
	    break;
	    }
	*realname++ = *p++;
	}
    *realname = '\0';
}


extern FILE *dvityout;

/* Print real number r in format n:m on the DVItype output file. */
void dviprintreal(r, n, m)
double r;
int n,m;
{
    char fmt[50];

    (void) sprintf(fmt, "%%%d.%df", n, m);
    (void) fprintf(dvityout, fmt, r);
}
