/*
 * MetaPost in C, system-dependent code
 *
 * (This version does NOT include improved versions of
 *  make_fraction and take_fraction.)
 */
#include <stdio.h>

#define	EXTERN			/* Instantiate data in this module */
#ifndef	INIMP			/* Keep lint happy */
#define	INIMP			/* Instantiate INIMP-specific data */
#endif
#include "mpd.h"		/* Includes "site.h" via "mp.h" */

#ifdef _POSIX_SOURCE
#define	index	strchr
#define rindex	strrchr
#else
#ifdef	SYSV
#define	index	strchr
extern int sprintf ();
#else
extern char *sprintf ();
#endif /* SYSV */
#endif /* _POSIX_SOURCE */

/* C library routines.  */
extern char *malloc (), *getenv ();
extern char *index (), *rindex ();

int argc;
static char **gargv;


#ifdef MS_DOS
#define PATH_DELIM ';'
#else
#define PATH_DELIM ':'
#endif



main(iargc, iargv)
int iargc;
char *iargv[];
{
    argc=iargc;
    gargv=iargv;
    main_body();
} 

/* Return the integer absolute value.  */
integer zabs(x)
integer x;
{
    if (x >= 0) return(x);
    return(-x);
}

/* Return the nth program argument into the string pointed at by s */
argv(n, s)
integer n; char *s;
{
    (void) sprintf(s+1, "%s ", gargv[n-1]);
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

/* Open a file.  Don't return unless successful.  */
FILE *Fopen(name, mode)
char *name, *mode;
{
    FILE *result;
    char *index(), *cp, my_name[filenamesize];
    register int i = 0;

    for (cp=name; *cp && *cp != ' ';)
	my_name[i++] = *cp++;
    my_name[i] = '\0';
    result = fopen(my_name, mode);
    if (!result) {
	perror(my_name);
	exit(1);
    }
    return(result);
}

#ifdef	sequent
/*
 * On a Sequent Balance system under Dynix 2.1.1, if u and v are unsigned
 * shorts or chars, then "(int) u - (int) v" does not perform a signed
 * subtraction.  Hence we have to call this routine to force the compiler
 * to treat u and v as ints instead of unsigned ints.  Sequent knows about
 * the problem, but they don't seem to be in a hurry to fix it.
 */
integer ztoint(x)
{
    return(x);
}
#endif


/*
 * Read a line of input as efficiently as possible while still
 * looking like Pascal.
 * Sets last=first and returns FALSE if eof is hit.
 * Otherwise, TRUE is returned and either last==first or buffer[last-1]!=' ';
 * that is, last == first + length(line except trailing whitespace).
 */
zinputln(f)
FILE *f;
{
    register int i;

    last = first;
#ifdef	BSD
    if (f==stdin) clearerr(stdin);
#endif
    while (last < bufsize && ((i = getc(f)) != EOF) && i != '\n') {
#ifdef	NONASCII
	buffer[last++] = i;
#else
	buffer[last++] = (i > 255 || i < 0) ? ' ' : i;
#endif
    }
    if (i == EOF && last == first)
	return(false);
    if (i != EOF && i != '\n') {
	/* We didn't get whole line because of lack of buffer space */
	(void) fprintf(stderr, "\nUnable to read an entire line---bufsize=%d\n",
	    bufsize);
	exit(1);
    }
    buffer[last] = ' ';
    if (last >= maxbufstack)
	maxbufstack = last;	/* Record keeping */
    /* Trim trailing whitespace by decrementing "last" */
    while (last > first && (buffer[last-1] == ' ' || buffer[last-1] == '\t'))
	--last;
    /* Now, either last==first or buffer[last-1] != ' ' (or tab) */
/*
 * If we're running on a system with ASCII characters like TeX's
 * internal character set, we can save some time by not executing
 * this loop.
 */
#ifdef	NOTASCII
    for (i = first; i <= last; i++)
	buffer[i] = xord[buffer[i]];
#endif
    return(true);
}



/**********************************************************
 The following is needed by MetaPost but not METAFONT
 **********************************************************/

#define CMDLENGTH 300

/*
 * Invoke makempx to make sure there is an up-to-date .mpx file
 * for a given .mp file
 *
 * John Hobby 3/14/90
 */

boolean callmakempx(mpname, mpxname)
char *mpname, *mpxname;
{
	char *cmd, *p, *q, *qlimit;
	char buf[CMDLENGTH];

	cmd = getenv("MPXCOMMAND");
	if (cmd==NULL) cmd=MPXCOMMAND;

	q = buf;
	qlimit = buf+CMDLENGTH-1;
	for (p=cmd; *p!=0; p++)
		if (q==qlimit) return 0; else *q++ = *p;
	*q++ = ' ';
	for (p=mpname+1; *p!=0 && *p!=' '; p++)
		if (q==qlimit) return 0; else *q++ = *p;
	*q++ = ' ';
	for (p=mpxname+1; *p!=0 && *p!=' '; p++)
		if (q==qlimit) return 0; else *q++ = *p;
	*q = 0;
	return system(buf)==0;
}



/**********************************************************
 The following is taken from cmf/MFlib/b_input.c
 **********************************************************/

/*
 * Byte Output routines for Metafont
 *
 * Tim Morgan  4/8/88
 */


#define aputc(x, f) \
    if ((eightbits)putc(((x) & 255), f) != ((x) & 255)) \
	perror("putc"), exit(1)

zbwrite2bytes(f, b)
FILE *f;
integer b;
{
    aputc((b >> 8), f);
    aputc((b & 255), f);
}

zbwrite4bytes(f, b)
FILE *f;
integer b;
{
    aputc((b >> 24), f);
    aputc((b >> 16), f);
    aputc((b >> 8), f);
    aputc(b, f);
}

zbwritebuf(f, buf, first, last)
FILE *f;
eightbits buf[];
integer first, last;
{
    if (!fwrite((char *) &buf[first], sizeof(buf[0]), (int)(last-first+1), f)) {
	perror("fwrite");
	exit(1);
    }
}



/**********************************************************
 The following is taken from cmf/MFlib/calledit.c
 **********************************************************/

/*
**  The following procedure is due to sjc@s1-c.
**
**	calledit(filename, fnlength, linenumber)
**
** Metafont can call this to implement the 'e' feature in error-recovery
** mode, invoking a text editor on the erroneous source file.
**
** You should pass to "filename" the first character of the packed array
** containing the filename, and to "fnlength" the size of the filename.
**
** Ordinarily, this invokes "/usr/ucb/vi". If you want a different
** editor, create a shell environment variable MPEDITOR containing
** the string that invokes that editor, with "%s" indicating where
** the filename goes and "%d" indicating where the decimal
** linenumber (if any) goes. For example, a MPEDITOR string for a
** variant copy of "vi" might be:
**  
**	setenv MPEDITOR "/usr/local/bin/vi +%d %s"
**  
*/

char *mfeditvalue = EDITOR;

calledit(filename, fnstart, fnlength, linenumber)
ASCIIcode filename[];
poolpointer fnstart;
integer fnlength, linenumber;
{
    char *temp, *command, c;
    int sdone, ddone, i;

    sdone = ddone = 0;
    filename += fnstart;

    /* Close any open input files */
    for (i=1; i <= inopen; i++)
	(void) fclose(inputfile[i]);

    /* Replace default with environment variable if possible. */
    if (NULL != (temp = (char *) getenv("MPEDITOR")))
	mfeditvalue = temp;

    /* Make command string containing envvalue, filename, and linenumber */
    if (! (command = malloc((unsigned) (strlen(mfeditvalue)+fnlength+25)))) {
	fprintf(stderr, "! Not enough memory to issue editor command\n");
	exit(1);
    }
    temp = command;
    while ((c = *mfeditvalue++) != '\0') {
	if (c == '%') {
	    switch (c = *mfeditvalue++) {
	    case 'd':
		if (ddone) {
		    fprintf(stderr,
		       "! Line number cannot appear twice in editor command\n");
		    exit(1);
		}
		(void) sprintf(temp, "%d", linenumber);
		while (*temp != '\0')
		    temp++;
		ddone = 1;
		break;
	    case 's':
		if (sdone) {
		    fprintf(stderr,
			"! Filename cannot appear twice in editor command\n");
		    exit(1);
		}
		i = 0;
		while (i < fnlength)
		    *temp++ = filename[i++]; /** mf/extra.c says ^0x80 here!? **/
		sdone = 1;
		break;
	    case '\0':
		*temp++ = '%';
		mfeditvalue--;	/* Back up to \0 to force termination. */
		break;
	    default:
		*temp++ = '%';
		*temp++ = c;
		break;
	    }
	}
	else
	    *temp++ = c;
    }
    *temp = '\0';

    /* Execute command. */
    if (system(command) != 0)
	fprintf(stderr, "! Trouble executing command `%s'.\n", command);

    /* Quit, indicating MetaPost had found an error before you typed "e". */
    exit(1);
}




/**********************************************************
 The following is taken from cmf/MFlib/dateandtime.c
 **********************************************************/


#include <signal.h>
#ifdef	BSD
#include <sys/time.h>
#else
#include <time.h>
#endif

extern	integer	interrupt;		/* defined in mf.p; non-zero means */
					/*  a user-interrupt request	   */
					/*  is pending			   */
/*
**  Gets called when the user hits his interrupt key.  Sets the global
**  "interrupt" nonzero, then sets it up so that the next interrupt will
**  also be caught here.
**
*/

void catchint()
{
    interrupt = 1;
    (void) signal(SIGINT, catchint);
}

/*
**  Stores minutes since midnight, current day, month and year into
**  *time, *day, *month and *year, respectively.
**
**  Also, set things up so that catchint() will get control on interrupts.
*/

void ydateandtime(minutes, day, month, year)
integer *minutes, *day, *month, *year;
{
    long clock, time();
    struct tm *tmptr, *localtime();
    static SIGNAL_HANDLER_RETURN_TYPE (*psig)();

    clock = time((long *) 0);
    tmptr = localtime(&clock);

    *minutes = tmptr->tm_hour * 60 + tmptr->tm_min;
    *day = tmptr->tm_mday;
    *month = tmptr->tm_mon + 1;
    *year = tmptr->tm_year + 1900;

    if ((psig = signal(SIGINT, catchint)) != SIG_DFL)
	(void) signal(SIGINT, psig);
}





/**********************************************************
 The following is a modified version of cmf/MFlib/paths.c
 **********************************************************/

#undef	close			/* We're going to use the Unix close call */

/*
** These routines reference the following global variables and constants
** from Metafont and its associated utilities. If the name or type of
** these variables is changed in the Pascal source, be sure to make them
** match here!
*/

#define READACCESS 4		/* args to test_access()	*/
#define WRITEACCESS 2

#define NOFILEPATH 0
#define TEXFONTFILEPATH 3
#define MFINPUTFILEPATH 6
#define MPINPUTFILEPATH 9
#define MPMEMFILEPATH 10
#define MPPOOLFILEPATH 11

/*
** Fixed arrays are used to hold the paths, to avoid any possible
** problems involving interaction of malloc and undump.
*/

char TeXfontpath[MAXPATHLENGTH] = TEXFONTS;
char MFinputpath[MAXPATHLENGTH] = MFINPUTS;
char MPinputpath[MAXPATHLENGTH] = MPINPUTS;
char MPmempath[MAXPATHLENGTH] = MPMEMS;
char MPpoolpath[MAXPATHLENGTH] = MPPOOL;

/*
** This copies at most n characters (including the null) from string s2
** to string s1, giving an error message for paths that are too long,
** identifying the PATH name.
*/
static void path_copypath(s1, s2, n, pathname)
    register char *s1, *s2, *pathname;
    register int n;
{
    while ((*s1++ = *s2++) != '\0')
	if (--n == 0) {
	    fprintf(stderr, "! Environment search path `%s' is too big.\n",
		pathname);
	    *--s1 = '\0';
	    return;		/* let user continue with truncated path */
	}
}

/* This returns a char* pointer to the string selected by the path
   specifier, or NULL if no path or an illegal path selected.
*/
static char *path_select(pathsel)
integer pathsel;
{
    char *pathstring = NULL;

    switch (pathsel) {
	case TEXFONTFILEPATH:
	    pathstring = TeXfontpath;
	    break;
	case MFINPUTFILEPATH:
	    pathstring = MFinputpath;
	    break;
	case MPINPUTFILEPATH:
	    pathstring = MPinputpath;
	    break;
	case MPMEMFILEPATH:
	    pathstring = MPmempath;
	    break;
	case MPPOOLFILEPATH:
	    pathstring = MPpoolpath;
	    break;
	default:
	    pathstring = NULL;
    }
    return(pathstring);
}

/*
**   path_buildname(filename, buffername, cpp)
**	makes buffername contain the directory at *cpp,
**	followed by '/', followed by the characters in filename
**	up until the first blank there, and finally a '\0'.
**	The cpp pointer is left pointing at the next directory
**	in the path.
**
**	But: if *cpp == NULL, then we are supposed to use filename as is.
*/
void path_buildname(filename, buffername, cpp)
    char *filename, *buffername;
    char **cpp;
{
    register char *p, *realname;
    
    realname = buffername;
    if ((p = *cpp) != NULL) {
	while ((*p != PATH_DELIM) && (*p != '\0')) {
	    *realname++ = *p++;
	    if (realname >= &(buffername[FILENAMESIZE-1]))
		break;
	}
	if (*p == '\0') *cpp = NULL;	/* at end of path now */
	else *cpp = p+1;		/* else get past PATH_DELIM */
	*realname++ = '/';	/* separate the area from the name to follow */
    }
    /* now append filename to buffername... */
    p = filename;
    if (*p == 0) p++;
    while (*p != ' ' && *p != 0) {
	if (realname >= &(buffername[FILENAMESIZE-1])) {
	    fprintf(stderr, "! Full file name is too long\n");
	    break;
	}
	*realname++ = *p++;
    }
    *realname = '\0';
}

/*
** setpaths is called to set up the default path arrays as follows:
** if the user's environment has a value for the appropriate value,
** then use it;  otherwise, leave the current value of the array
** (which may be the default path, or it may be the result of
** a call to setpaths on a previous run that was made the subject of
** an undump: this will give the maker of a preloaded Metafont the
** option of providing a new set of "default" paths.
**
** Note that we have to copy the string from the environment area, since
** that will change on the next run (which matters if this is for a
** preloaded Metafont).
*/

void setpaths()
{
    register char *envpath;
	
    if ((envpath = getenv("TEXFONTS")) != NULL)
	path_copypath(TeXfontpath,envpath,MAXPATHLENGTH,"TEXFONTS");
    if ((envpath = getenv("MFINPUTS")) != NULL)
	path_copypath(MFinputpath,envpath,MAXPATHLENGTH,"MFINPUTS");
    if ((envpath = getenv("MPINPUTS")) != NULL)
	path_copypath(MPinputpath,envpath,MAXPATHLENGTH,"MPINPUTS");
    if ((envpath = getenv("MPMEMS")) != NULL)
	path_copypath(MPmempath,envpath,MAXPATHLENGTH,"MPMEMS");
    if ((envpath = getenv("MPPOOL")) != NULL)
	path_copypath(MPpoolpath,envpath,MAXPATHLENGTH,"MPPOOL");
}

/*
**   testaccess(filename, realfilename, amode, filepath)
**
**	Test whether or not the file whose name is in filename
**	can be opened for reading (if mode=READACCESS)
**	or writing (if mode=WRITEACCESS).
**
**	The filepath argument is one of the ...FILEPATH constants
**	defined above.  If the filename given in filename does not
**	begin with '/', we try prepending all the PATH_DELIM-separated
**	directory names in the appropriate path to the filename until access
**	can be made, if it ever can.
**
**	The realfilename array will contain the name that
**	yielded an access success.
*/

int ztestaccess(filename, realfilename, amode, filepath)
    char *filename, *realfilename;
    integer amode, filepath;
{
    register boolean ok;
    register char *p;
    char *curpathplace;
    int f;

    filename++;
    realfilename++;

    curpathplace = (*filename == '/') ? NULL : path_select (filepath);

    do {
	path_buildname(filename, realfilename, &curpathplace);
	if (amode == READACCESS) {
	    /* use system call "access" to see if we could read it */
	    if (access(realfilename, 04) == 0) ok = true;
	    else ok = false;
	} else {
	    /* WRITEACCESS: use creat to see if we could create it, but close
	    the file again if we're OK, to let pc open it for real */
	    if ((f = creat(realfilename, 0666)) >= 0) {
		ok = true;
		(void) close(f);
	    } else
		ok = false;
	}
    } while (!ok && curpathplace != NULL);

    if (ok) {		/* pad realnameoffile with blanks, as Pascal wants */
	for (p = realfilename; *p != '\0'; p++)
	    ;			/* nothing: find end of string */
	while (p <= &(realfilename[FILENAMESIZE-1]))
	    *p++ = ' ';
    }

    return (ok);
}



#ifdef _POSIX_SOURCE
putw(int w, FILE *f)
{
	fwrite((char *)&w, 1, sizeof(w), f)!=sizeof(w);
}

int getw(FILE *f)
{
	int x;

	fread((char *)&x, sizeof(x), 1, f);
	return x;
}
#endif
