/*
 * Hand-coded routines for C TeX.
 * This module should contain any system-dependent code.
 *
 * This code was written by Tim Morgan, drawing from other Unix
 * ports of TeX.
 */

#define	CATCHINT		/* Catch ^C's */

#define	EXTERN			/* Actually instantiate globals here */
#include "texd.h"

#ifndef	MAXPATHLENGTH		/* This is now defined in ../site.h */
#define	MAXPATHLENGTH		1024
#endif

/* Various include files we'll need */
#ifdef	CATCHINT
#include <signal.h>
#endif
#ifdef	BSD
#include <sys/time.h>
#include <sys/wait.h>
#include <sys/file.h>
#include <fcntl.h>
#else
#include <time.h>
#endif
#if defined(BSD)||defined(SYSV)
#include <sgtty.h>
#endif

static char *texeditvalue = EDITOR;

#ifdef _POSIX_SOURCE
#define	index	strchr
#define	rindex	strrchr
#else
#ifdef	SYSV		/* Sys V compatibility */
#define	index	strchr
#define	rindex	strrchr
extern int sprintf();
#else
extern char *sprintf();
#endif
#endif

/* C library stuff that we're going to use */
#ifndef	BSD
#include <string.h>
#else
#include <strings.h>
#endif
extern char *malloc(), *index(), *getenv();

/* Local stuff */
static int gargc;
static char **gargv;
static char texformats[MAXPATHLENGTH]=TEXFORMATS;
static char texinputs[MAXPATHLENGTH]=TEXINPUTS;
static char texfonts[MAXPATHLENGTH]=TEXFONTS;
static char texpool[MAXPATHLENGTH]=TEXPOOL;
static char my_realnameoffile[FILENAMESIZE];


#ifdef	CATCHINT
/* If we're catching ^C, come here when one comes in. */
static SIGNAL_HANDLER_RETURN_TYPE catchint()
{
    interrupt = 1;
    (void) signal(SIGINT, catchint);
}
#endif

/* Do the obvious */
get_date_and_time(minutes, day, month, year)
integer *minutes, *day, *month, *year;
{
    long clock, time();
    struct tm *tmptr, *localtime();

    clock = time((long *) 0);
    tmptr = localtime(&clock);

    *minutes = tmptr->tm_hour * 60 + tmptr->tm_min;
    *day = tmptr->tm_mday;
    *month = tmptr->tm_mon + 1;
    *year = tmptr->tm_year + 1900;
#ifdef	CATCHINT
    (void) signal(SIGINT, catchint);
#endif
}

#ifdef	INITEX
/* Same as in Pascal --- return TRUE if EOF or next char is newline */
eoln(f)
FILE *f;
{
    register int c;

    if (feof(f)) return(true);
    c = getc(f);
    if (c != EOF)
	(void) ungetc(c, f);
    return (c == '\n' || c == EOF);
}
#endif	/* INITEX */

static void copy_path(ptr, override)
char *ptr, *override;
{
    if (override) {
	if (strlen(override) >= MAXPATHLENGTH) {
	    (void) fprintf(stderr, "Path too long\n");
	    exit(1);
	}
	(void) strcpy(ptr, override);
    }
}


/* Initialize path variables for loading fonts and things */
setpaths()
{
    copy_path(texformats, getenv("TEXFORMATS"));
    copy_path(texinputs, getenv("TEXINPUTS"));
    copy_path(texfonts, getenv("TEXFONTS"));
    copy_path(texpool, getenv("TEXPOOL"));
}

/*
 *  The following procedure is due to sjc@s1-c.
 *
 *	calledit(filename, fnstart, fnlength, linenumber)
 *
 *  TeX82 can call this to implement the 'e' feature in error-recovery
 *  mode, invoking a text editor on the erroneous source file.
 *  
 *  You should pass to "filename" the first character of the packed array
 *  containing the filename, and to "fnstart" and "fnlength" the index and
 *  length of the filename as it appears inside that array.
 *  
 *  Ordinarily, this invokes "/usr/ucb/vi". If you want a different
 *  editor, create a shell environment variable TEXEDIT containing
 *  the string that invokes that editor, with "%s" indicating where
 *  the filename goes and "%d" indicating where the decimal
 *  linenumber (if any) goes. For example, a TEXEDIT string for a
 *  variant copy of "vi" might be:
 *  
 *	setenv TEXEDIT "/usr/local/bin/vi +%d %s"
 *  
 */

calledit(filename, fnstart, fnlength, linenumber)
ASCIIcode *filename;
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

    /* Replace default with environment variable if possible.
     * We can get away with a simple assignment because we don't call
     * getenv() again.
     */
    if (NULL != (temp = getenv("TEXEDIT")))
	texeditvalue = temp;

    /* Make command string containing envvalue, filename, and linenumber */
    if (NULL ==
	(command = (char *) malloc((unsigned) (strlen(texeditvalue) + fnlength
				   + 25)))) {
	(void) fprintf(stderr, "! Not enough memory to issue editor command\n");
	exit(1);
    }
    temp = command;
    while ((c = *texeditvalue++) != '\0') {
	if (c == '%') {
	    switch (c = *texeditvalue++) {
	    case 'd':
		if (ddone) {
		    (void) fprintf(stderr,
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
		    (void) fprintf(stderr,
		      "! Filename cannot appear twice in editor command\n");
		    exit(1);
		}
		i = 0;
		while (i < fnlength)
		    *temp++ = filename[i++];
		sdone = 1;
		break;
	    case '\0':
		*temp++ = '%';
		texeditvalue--;	/* Back up to \0 to force termination. */
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
    if (system(command))
	(void) fprintf(stderr, "! Trouble executing command %s\n", command);

    /* Quit, indicating TeX found an error */
    exit(1);
}


/*
 * Change a Pascal-like filename into a C-like string.
 * If running on a non-ASCII system, will have to use xchr[]
 * array to convert from internal character set (in the "nameoffile"
 * array) into the enternal character set (in "realnameoffile").
 * "nameoffile" in TeX is indexed starting at 1, but by the time
 * we're called, it has already been bumped up by one.
 */
static packrealnameoffile(prefix, nameoffile)
char *prefix, *nameoffile;
{
    register char *cp, *p, *q;

    p = prefix;
    for (q = my_realnameoffile; p && *p && *p != ':'; *q++ = *p++);
    if (prefix && *prefix)
	*q++ = '/';
    for (cp = nameoffile, p = realnameoffile + 1; *cp != ' ' && *cp;)
#ifdef	NONASCII
	*q++ = xchr[*p++ = *cp++];
#else
	*q++ = (*p++ = *cp++);
#endif
    *p = ' ';
    *q = '\0';
}

#ifdef	BSD
/*
 * This code is from Chris Torek:
 *
 * The following is an internal-use-only routine that makes a core
 * dump without any sort of error status.  It is used only when
 * making a production TeX from a virtex, and is triggered by a magic
 * file name requested as \input.
 *
 * This is what is known in computing circles as a hack.
 */
static void funny_core_dump()
{
	int pid, w;
	union wait status;

	switch (pid = vfork()) {

	case -1:		/* failed */
		perror("vfork");
		exit(-1);
		/*NOTREACHED*/

	case 0:			/* child */
		(void) signal(SIGQUIT, SIG_DFL);
		(void) kill(getpid(), SIGQUIT);
		(void) write(2, "how did we get here?\n", 21);
		_exit(1);
		/*NOTREACHED*/

	default:		/* parent */
		while ((w = wait(&status)) != pid && w != -1)
			;
		if (status.w_coredump)
			exit(0);
		(void) write(2, "attempt to dump core failed\n", 28);
		exit(1);
	}
}
#endif /* BSD */

/* Open an input file f */
Openin(f, pathspec)
FILE **f;
int pathspec;
{
    register char *prefix, *nprefix;
    register char *cp, *tex_name;

#ifdef	BSD
    if (pathspec == inputpathspec &&
	strncmp(nameoffile+1, "HackyInputFileNameForCoreDump.tex", 33) == 0)
	funny_core_dump();
#endif	/* BSD */
    for (tex_name = &nameoffile[1]; *tex_name == ' '; ++tex_name);

    switch (pathspec) {
    case inputpathspec:
    case readpathspec:
	prefix = texinputs;
	break;
    case fontpathspec:
	prefix = texfonts;
	break;
    case poolpathspec:
	prefix = texpool;
	break;
    case fmtpathspec:
	prefix = texformats;
	break;
    default:			/* "Can't" happen */
	(void) fprintf(stderr, "INTERNAL CONFUSION---IMPLEMENTATION ERROR\n");
	exit(1);
    }
    if (*tex_name == '/') {	/* Absolute pathname-don't do path searching */
	for (cp = tex_name; *cp && *cp != ' '; ++cp);
	*cp = '\0';
	if (*f = fopen(tex_name, "r")) {
	    (void) strcpy(realnameoffile + 1, tex_name);
	    (void) strcat(realnameoffile + 1, " ");
	    if (pathspec == fontpathspec)
		tfmtemp = getc(*f);	/* Simulate readahead on TFM files */
	    return (1);
	}
	return (0);
    }
    /* Else relative pathname */
    while (prefix) {
	packrealnameoffile(prefix, tex_name);
	*f = fopen(my_realnameoffile, "r");
	if (*f) {
	    (void) strcpy(realnameoffile + 1, my_realnameoffile);
	    (void) strcat(realnameoffile + 1, " ");
	    if (pathspec == fontpathspec)
		tfmtemp = getc(*f);	/* Simulate Pascal readahead */
	    return (1);
	}
	nprefix = index(prefix, ':');
	if (nprefix)
	    prefix = nprefix + 1;
	else
	    break;
    }
    return (0);
}

/* Open an output file f */
Openout(f)
FILE **f;
{
    register char *tex_name;
    int result;

    for (tex_name = &nameoffile[1]; *tex_name == ' '; ++tex_name);
    packrealnameoffile((char *) NULL, tex_name);
    *f = fopen(my_realnameoffile, "w");
    return (*f != NULL);
}


/*
 * Read a line of input as efficiently as possible while still
 * looking like Pascal.
 * Sets last==first and returns FALSE if eof is hit.
 * Otherwise, TRUE is returned and either last==first or buffer[last-1]!=' ';
 * that is, last == first + length(line except trailing whitespace).
 */
boolean zinputln(f)
FILE *f;
{
    register int i;

    last = first;
#ifdef	BSD
    if (f == stdin) clearerr(stdin);
#endif
    while ( last < bufsize && ((i = getc(f)) != EOF) && i != '\n') {
#ifdef	NONASCII
	buffer[last++] = i;
#else
	buffer[last++] = (i > 255 || i < 0)?' ':i;
#endif
    }
    if (i == EOF && last == first)
	return(false);
    if (i != EOF && i != '\n') {
	/* We didn't get whole line because of lack of buffer space */
	(void) fprintf(stderr, "\nUnable to read an entire line---bufsize=%d\n",
	    bufsize);
	(void) fprintf(stderr, "Please ask a wizard to enlarge me.\n");
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

/*
 * Clear any pending terminal input.
 * The usual way to do this in Unix is to switch the terminal to get
 * the current tty flags, set RAW mode, then switch back to the original
 * setting.  If the user isn't in COOKED mode, though, this won't work.
 * At least, it leaves his terminal in its original mode.
 */
clearterminal()
{
#ifdef	BSD
    int arg = FREAD;

    (void) ioctl(fileno(stdout), TIOCFLUSH, &arg);
#endif	/* BSD */
}


/*
 * Cancel any output cancellation (^O) by the user.  This is 4.2BSD code.
 * Systems which don't have this feature might want to #define wakeupterminal
 * to do nothing, rather than call this empty routine, but it's only called
 * when about to read from the terminal, so the extra time (compared with
 * the human's response time) is minimal.
 */
wakeupterminal()
{
#ifdef	BSD
    int i = LFLUSHO;

    (void) ioctl(fileno(stdout), TIOCLBIC, (struct sgttyb *) &i);
#endif	/* BSD */
}

static char *progname = NULL;

/*
 * "Open the terminal for input".  Actually, copy any command-line
 * arguments for processing.  If nothing is available, or we've
 * been called already (and hence, gargc==0), we return with last==first.
 */
topenin()
{
    register int i;

    buffer[first] = '\0';	/* So the first strcat will work */

#ifndef	INITEX
    /* If no format file specified, use progname as the default */
    if ((gargc < 2 || gargv[1][0] != '&') && progname != NULL)
	(void) sprintf((char *) &buffer[first], "&%s ", progname);
    progname = NULL;
#endif	/* ! INITEX */

    if (gargc > 1) {	/* There are command-line arguments */
	for (i=1; i<gargc; i++) {
	    (void) strcat((char *) &buffer[first], gargv[i]);
	    (void) strcat((char *) &buffer[first], " ");
	}
	gargc = 0;	/* Don't do this again */
    }

    /* Make last index 1 past the last non-blank character in buffer[] */
    for (last=first; buffer[last]; ++last);
    for (--last; last >= first && buffer[last] == ' '; --last);
    ++last;
}


/*
 * Get things going under Unix: set up for rescanning the command line,
 * then call TeX's main body.
 */
main(argc, argv)
int argc;
char *argv[];
{
#ifndef	INITEX
    if (readyalready != 314159) {
	if ((progname = rindex(argv[0], '/')) == NULL)
	    progname = argv[0];
	else
	    ++progname;
	if (strcmp(progname, "virtex") == 0)
	    progname = NULL;
    }
#endif	/* ! INITEX */
    gargc = argc;
    gargv = argv;
    texbody();
    /*NOTREACHED*/
} 


#ifdef	DEBUG
getint()
{
    int i;

    if (fscanf(stdin, "%d", &i)) return(i);
    return(0);
}
#endif	/* DEBUG */

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
