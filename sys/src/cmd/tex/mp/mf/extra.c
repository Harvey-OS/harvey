/* This file contains the C routines that implement most of the
   system-dependent parts of Metafont.

   This code was originally written by Tim Morgan, drawing from other
   Unix ports of Metafont.  */

#include <stdio.h>
#define	EXTERN	extern		/* Don't instantiate data.  */
#include "mfd.h"		/* Includes "site.h" via "mf.h".  */

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


/* Place to save the command line arguments.  */
static int gargc;
static char **gargv;

/* The name we are invoked as.  */
static char *progname = NULL;


/* ``Open the terminal for input.''  Actually, copy any commandline
   arguments for processing.  If nothing is available, or we've been
   called already (and hence, gargc==0), we return with last==first.  */

void
topenin ()
{
  register int i;

  buffer[first] = '\0';		/* So the first strcat will work. */

#ifndef	INIMF
  /* If no format file specified, use progname as the default */
  if ((gargc < 2 || gargv[1][0] != '&') && progname != NULL)
    (void) sprintf ((char *) &buffer[first], "&%s ", progname);
  progname = NULL;
#endif /* ! INIMF */

  if (gargc > 1)
    {				/* There are commandline arguments. */
      for (i = 1; i < gargc; i++)
	{
	  (void) strcat ((char *) &buffer[first], gargv[i]);
	  (void) strcat ((char *) &buffer[first], " ");
	}
      gargc = 0;		/* Don't do this again */
    }

  /* Make last index 1 past the last non-blank character in buffer[]. */
  for (last = first; buffer[last]; ++last) ;
  for (--last; last >= first && buffer[last] == ' '; --last) ;
  ++last;
}


/* The entry point.  Set up for rescanning the command line, then call
   Metafont's main body.  */

main (argc, argv)
     int argc;
     char *argv[];
{
#ifndef	INIMF
  if (readyalready != 314159)
    {
      if ((progname = rindex (argv[0], '/')) == NULL)
	progname = argv[0];
      else
	++progname;
      if (strcmp (progname, "virmf") == 0)
	progname = NULL;
    }
#endif /* !INIMF */
  gargc = argc;
  gargv = argv;
  main_body ();
  /*NOTREACHED*/
}


/* Return the integer absolute value.  */
integer 
zabs (x)
  integer x;
{
  if (x >= 0)
    return (x);
  return (-x);
}

/* Return the nth program argument into the string pointed at by s */
argv (n, s)
     integer n;
     char *s;
{
  (void) sprintf (s + 1, "%s ", gargv[n - 1]);
}

/* Return true on end of line or eof of file, else false */
eoln (f)
     FILE *f;
{
  register int c;

  if (feof (f))
    return (1);
  c = getc (f);
  if (c != EOF)
    (void) ungetc (c, f);
  return (c == '\n' || c == EOF);
}

/* Open a file.  Don't return unless successful.  */
FILE *
Fopen (name, mode)
     char *name, *mode;
{
  FILE *result;
  char *index (), *cp, my_name[FILENAMESIZE];
  register int i = 0;

  for (cp = name; *cp && *cp != ' ';)
    my_name[i++] = *cp++;
  my_name[i] = '\0';
  result = fopen (my_name, mode);
  if (!result)
    {
      perror (my_name);
      exit (1);
    }
  return (result);
}

#ifdef	sequent
/*
 * On a Sequent Balance system under Dynix 2.1.1, if u and v are unsigned
 * shorts or chars, then "(int) u - (int) v" does not perform a signed
 * subtraction.  Hence we have to call this routine to force the compiler
 * to treat u and v as ints instead of unsigned ints.  Sequent knows about
 * the problem, but they don't seem to be in a hurry to fix it.
 */
integer 
ztoint (x)
{
  return (x);
}

#endif


/*
 * Read a line of input as efficiently as possible while still
 * looking like Pascal.
 * Sets last=first and returns FALSE if eof is hit.
 * Otherwise, TRUE is returned and either last==first or buffer[last-1]!=' ';
 * that is, last == first + length(line except trailing whitespace).
 */
zinputln (f)
     FILE *f;
{
  register int i;

  last = first;
#ifdef	BSD
  if (f == stdin)
    clearerr (stdin);
#endif
  while (last < bufsize && ((i = getc (f)) != EOF) && i != '\n')
    {
#ifdef	NONASCII
      buffer[last++] = i;
#else
      buffer[last++] = (i > 255 || i < 0) ? ' ' : i;
#endif
    }
  if (i == EOF && last == first)
    return (false);
  if (i != EOF && i != '\n')
    {
      /* We didn't get whole line because of lack of buffer space */
      (void) fprintf (stderr, "\nUnable to read an entire line---bufsize=%d\n",
		      bufsize);
      exit (1);
    }
  buffer[last] = ' ';
  if (last >= maxbufstack)
    maxbufstack = last;		/* Record keeping */
  /* Trim trailing whitespace by decrementing "last" */
  while (last > first && (buffer[last - 1] == ' ' || buffer[last - 1] == '\t'))
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
  return (true);
}



/*
 * Byte output routines.
 *
 * Tim Morgan  4/8/88
 */

#define aputc(x, f) \
  if ((eightbits) putc(((x) & 255), f) != ((x) & 255)) \
	perror("putc"), exit(1)

zbwrite2bytes (f, b)
     FILE *f;
     integer b;
{
  aputc ((b >> 8), f);
  aputc ((b & 255), f);
}

zbwrite4bytes (f, b)
     FILE *f;
     integer b;
{
  aputc ((b >> 24), f);
  aputc ((b >> 16), f);
  aputc ((b >> 8), f);
  aputc (b, f);
}

zbwritebuf (f, buf, first, last)
     FILE *f;
     eightbits buf[];
     integer first, last;
{
  if (!fwrite ((char *) &buf[first], sizeof (buf[0]),
               (int) (last - first + 1), f))
    {
      perror ("fwrite");
      exit (1);
    }
}



/* Switch-to-editor routine.
   The following procedure is due to sjc@s1-c, and it doesn't work with
   the new string pool any more.  Most likely only a very simple change
   is needed.

   Metafont can call this to implement the 'e' feature in error-recovery
   mode, invoking a text editor on the erroneous source file.

   You should pass to "filename" the first character of the packed array
   containing the filename, and to "fnlength" the size of the filename.

   Ordinarily, this invokes what is defined in site.h. If you want a
   different editor, create a shell environment variable MFEDIT
   containing the string that invokes that editor, with "%s" indicating
   where the filename goes and "%d" indicating where the decimal
   linenumber (if any) goes. For example, a MFEDIT string for a variant
   copy of "vi" might be:

	setenv MFEDIT "/usr/local/bin/vi +%d %s"
*/

char *mfeditvalue = EDITOR;

calledit (filename, fnstart, fnlength, linenumber)
     ASCIIcode *filename;
     poolpointer fnstart;
     integer fnlength, linenumber;
{
  char *temp, *command, c;
  int sdone, ddone, i;

  sdone = ddone = 0;
  filename += fnstart;

  /* Close any open input files */
  for (i = 1; i <= inopen; i++)
    (void) fclose (inputfile[i]);

  /* Replace default with environment variable if possible. */
  if (NULL != (temp = (char *) getenv ("MFEDIT")))
    mfeditvalue = temp;

  /* Make command string containing envvalue, filename, and linenumber */
  if (!(command = malloc ((unsigned) (strlen (mfeditvalue) + fnlength + 25))))
    {
      fprintf (stderr, "! Not enough memory to issue editor command\n");
      exit (1);
    }
  temp = command;
  while ((c = *mfeditvalue++) != '\0')
    {
      if (c == '%')
	{
	  switch (c = *mfeditvalue++)
	    {
	    case 'd':
	      if (ddone)
		{
		  fprintf (stderr,
		   "! Line number cannot appear twice in editor command\n");
		  exit (1);
		}
	      (void) sprintf (temp, "%d", linenumber);
	      while (*temp != '\0')
		temp++;
	      ddone = 1;
	      break;
	    case 's':
	      if (sdone)
		{
		  fprintf (stderr,
		      "! Filename cannot appear twice in editor command\n");
		  exit (1);
		}
	      i = 0;
	      while (i < fnlength)
		*temp++ = filename[i++] ^ 0x80;
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
  if (system (command) != 0)
    fprintf (stderr, "! Trouble executing command `%s'.\n", command);

  /* Quit, indicating Metafont had found an error before you typed "e". */
  exit (1);
}




/* Interrupt handling and date/time routines.  */

#include <signal.h>
#ifdef	BSD
#include <sys/time.h>
#else
#include <time.h>
#endif

/* Defined in mf.p; nonzero means an interrupt request is pending.  */
extern integer interrupt;

/* Gets called when the user hits his interrupt key.  Sets the global
   ``interrupt'' nonzero, then sets it up so that the next interrupt will
   also be caught here.  */

void
catchint ()
{
  interrupt = 1;
  (void) signal (SIGINT, catchint);
}

/* Stores minutes since midnight, current day, month and year into
   *time, *day, *month and *year, respectively.

   It also sets things up so that catchint() will get control on
   interrupts.  */

void
ydateandtime (minutes, day, month, year)
     integer *minutes, *day, *month, *year;
{
  long clock, time ();
  struct tm *tmptr, *localtime ();
  static SIGNAL_HANDLER_RETURN_TYPE (*psig) ();

  clock = time ((long *) 0);
  tmptr = localtime (&clock);

  *minutes = tmptr->tm_hour * 60 + tmptr->tm_min;
  *day = tmptr->tm_mday;
  *month = tmptr->tm_mon + 1;
  *year = tmptr->tm_year + 1900;

  if ((psig = signal (SIGINT, catchint)) != SIG_DFL)
    (void) signal (SIGINT, psig);
}



/* Faster arithmetic.
 *	These functions account for a significant percentage of the processing
 *	time used by Metafont, and have been recoded in assembly language for
 *	some classes of machines.
 *
 *	User BEWARE!  These codings make assumptions as to how the Pascal
 *	compiler passes arguments to external subroutines, which should not
 *	be ignored if you take this code to a different environment than
 *	it was originally designed and tested on!  You have been warned!
 *
 *	function makefraction(p, q: integer): fraction;
 *		{ Calculate the function floor( (p * 2^28) / q + 0.5 )	}
 *		{ if both p and q are positive.  If not, then the value	}
 *		{ of makefraction is calculated as though both *were*	}
 *		{ positive, then the result sign adjusted.		}
 *		{ (e.g. makefraction ALWAYS rounds away from zero)	}
 *		{ In case of an overflow, return the largest possible	}
 *		{ value (2^32-1) with the correct sign, and set global	}
 *		{ variable "aritherror" to 1.  Note that -2^32 is 	}
 *		{ considered to be an illegal product for this type of	}
 *		{ arithmetic!						}
 *
 *	function takefraction(f: fraction; q: integer): integer;
 *		{ Calculate the function floor( (p * q) / 2^28 + 0.5 )	}
 *		{ takefraction() rounds in a similar manner as		}
 *		{   makefraction() above.				}
 *
 */

#define	EL_GORDO	0x7fffffff	/* 2^31-1		*/
#define	FRACTION_ONE	0x10000000	/* 2^28			*/
#define	FRACTION_HALF	0x08000000	/* 2^27			*/
#define	FRACTION_FOUR	0x40000000	/* 2^30			*/

/* This code makes Metafont fail the trap test on my Vax 11/750 running
   4.3bsd.  Be sure to try the trap test yourself if you turn it on.  */
#if defined(VAX4_2) || defined(VAX4_3) || defined(PYRAMID)

#if	defined(VAX4_2) || defined(VAX4_3)

/*
 * DEC VAX-11 class systems running 4.2 or 4.3 BSD Unix and Berkeley PC
 *
 * In the VAX assembler code that follows, remember, REMEMBER that
 * a quad result addressed in Rn has most significant longword
 * in Rn+1, NOT Rn!
 */

fraction 
zmakefraction (p, q)
     integer p, q;
{
  register r11;

  asm ("	movl	8(ap),r0		");	/* q		      */
  asm ("	xorl3	4(ap),r0,r11		");	/* sign of prod	      */
  asm ("	jgeq	Lmf1			");
  asm ("	mnegl	r0,r0			");	/* abs(q)*sign(p)	      */
  asm ("Lmf1:				");
  asm ("	divl2	$2,r0			");	/* 	 div 2 (signed)       */
  asm ("	emul	4(ap),$0x10000000,r0,r0	");	/* p*2^28+(sign(p)*(q/2)) */
  asm ("	ediv	8(ap),r0,r0,r1		");	/*((p*2^28)+	      */
  asm ("	jvs	Lmf2			");	/*	sign(p)*(q/2))/q      */
  asm ("	mnegl	r0,r1			");	/* check for 2^32	      */
  asm ("	jvc	Lmf3			");
  asm ("Lmf2:				");
  asm ("	movl	$1,_aritherror		");
  asm ("	movl	$0x7fffffff,r0		");	/* EL_GORDO		      */
  asm ("	tstl	r11			");
  asm ("	jgeq	Lmf3			");
  asm ("	mnegl	r0,r0			");
  asm ("Lmf3:				");
  asm ("	ret				");

}

integer 
ztakefraction (q, f)
     integer q;
     fraction f;
{
  register r11;

  asm ("	movl	$0x8000000,r0		");	/* 2^27		    */
  asm ("	xorl3	4(ap),8(ap),r11		");	/* sign of prod	    */
  asm ("	jgeq	Ltf1			");
  asm ("	mnegl	r0,r0			");	/* 2^27*sgn(prod)	    */
  asm ("Ltf1:				");
  asm ("	emul	4(ap),8(ap),r0,r0	");	/* q*f+sgn(prod)*2^27   */
  asm ("	ediv	$0x10000000,r0,r0,r1	");	/*	div 2^28 (signed!)  */
  asm ("	jvs	Ltf2			");
  asm ("	mnegl	r0,r1			");	/* check for -2^32	    */
  asm ("	jvc	Ltf3			");
  asm ("Ltf2:				");
  asm ("	movl	$1,_aritherror		");
  asm ("	movl	$0x7fffffff,r0		");	/* EL_GORDO		    */
  asm ("	tstl	r11			");
  asm ("	jgeq	Ltf3			");
  asm ("	mnegl	r0,r0			");
  asm ("Ltf3:				");
  asm ("	ret				");
}

#endif /* VAX */

#if	defined(PYRAMID)

/*
 * Pyramid 90x class of machines, running Pyramid's PASCAL
 */

fraction 
zmakefraction (p, q)
     integer p, q;
{

  asm ("	movw	$0,lr0		");	/* high word of 64 bit accum    */
  asm ("	mabsw	pr0,lr1		");	/* lr1 = abs(p)		    */
  asm ("	mabsw	pr1,lr2		");	/* lr2 = abs(q)		    */
  asm ("	ashll	$29,lr0		");	/* lr0'lr1 = abs(p)*2^29	    */
  asm ("	addw	lr2,lr1		");	/*  abs(p)*2^29 + abs(q)	    */
  asm ("	addwc	$0,lr0		");
  asm ("	ashrl	$1,lr0		");	/*  (abs(p)*2^28 + abs(q/2))    */
  asm ("	ediv	lr2,lr0		");	/*  (abs(p)*2^28 / abs(q)) + .5 */
  asm ("	bvc	L1		");
  asm ("	movw	$1,_aritherror	");
  asm ("	movw	$0x7fffffff,lr0	");	/* EL_GORDO			    */
  asm ("L1:			");
  asm ("	xorw	pr0,pr1		");	/* determine sign of result	    */
  asm ("	blt	L2		");
  asm ("	movw	lr0,pr0		");
  asm ("	ret			");
  asm ("L2:			");
  asm ("	mnegw	lr0,pr0		");	/* negative			    */
  asm ("	ret			");
}


integer 
ztakefraction (q, f)
     integer q;
     fraction f;
{
  asm ("	movw	$0,lr0		");	/* high word of 64 bit accum    */
  asm ("	mabsw	pr0,lr1		");	/* lr1 = abs(q)		    */
  asm ("	mabsw	pr1,lr2		");	/* lr2 = abs(f)		    */
  asm ("	emul	lr2,lr0		");	/* lr0'lr1 = abs(q) * abs(f)    */
  asm ("	addw	$0x08000000,lr1	");	/*  (abs(q)*abs(f))+2^27	    */
  asm ("	addwc	$0,lr0		");
  asm ("	ashll	$4,lr0		");	/* lr0=(abs(q)*abs(f))/2^28 + .5 */
  asm ("	bvc	L3		");
  asm ("	movw	$1,_aritherror	");
  asm ("	movw	$0x7fffffff,lr0	");	/* EL_GORDO			    */
  asm ("L3:			");
  asm ("	xorw	pr0,pr1		");	/* construct sign of product    */
  asm ("	blt	L4		");
  asm ("	movw	lr0,pr0		");
  asm ("	ret			");
  asm ("L4:			");
  asm ("	mnegw	lr0,pr0		");	/* negative			    */
  asm ("	ret			");
}

#endif /* PYRAMID */

#else /* the machine isn't anything special */

/* Here is essentially a verbatim copy of the WEB routines, with the
   hope that the C compiler may do better than the Pascal on code
   quality.  */

fraction 
zmakefraction (p, q)
     register integer p, q;
{
  int negative;
  register int be_careful;
  register fraction f;
  int n;

  if (p < 0)
    {
      negative = 1;
      p = -p;
    }
  else
    negative = 0;

  if (q < 0)
    {
      negative = !negative;
      q = -q;
    }

  n = p / q;
  p = p % q;

  if (n >= 8)
    {
      aritherror = true;
      return (negative ? -EL_GORDO : EL_GORDO);
    }

  n = (n - 1) * FRACTION_ONE;
  f = 1;
  do
    {
      be_careful = p - q;
      p = be_careful + p;
      if (p >= 0)
	{
	  f = f + f + 1;
	}
      else
	{
	  f <<= 1;
	  p += q;
	}
  } while (f < FRACTION_ONE);

  be_careful = p - q;
  if ((be_careful + p) >= 0)
    f += 1;

  return (negative ? -(f + n) : (f + n));
}

integer 
ztakefraction (q, f)
     register integer q;
     register fraction f;
{
  int n, negative;
  register int p, be_careful;

  if (f < 0)
    {
      negative = 1;
      f = -f;
    }
  else
    negative = 0;
  if (q < 0)
    {
      negative = !negative;
      q = -q;
    }

  if (f < FRACTION_ONE)
    n = 0;
  else
    {
      n = f / FRACTION_ONE;
      f = f % FRACTION_ONE;
      if (q < (EL_GORDO / n))
	n = n * q;
      else
	{
	  aritherror = true;
	  n = EL_GORDO;
	}
    }

  f += FRACTION_ONE;
  p = FRACTION_HALF;
  if (q < FRACTION_FOUR)
    do
      {
	if (f & 0x1)
	  p = (p + q) >> 1;
	else
	  p >>= 1;
	f >>= 1;
    } while (f > 1);
  else
  do
    {
      if (f & 0x1)
	p = p + ((q - p) >> 1);
      else
	p >>= 1;
      f >>= 1;
  } while (f > 1);

  be_careful = n - EL_GORDO;
  if ((be_careful + p) > 0)
    {
      aritherror = true;
      n = EL_GORDO - p;
    }

  return (negative ? -(n + p) : (n + p));
}
#endif /* ! (VAX4_2 || PYRAMID)  */




/* File routines.

	procedure setpaths;
		{ initialize path database from environment variables	     }

	function testaccess(
			var	filename: packed array of char;
			var	realfilename: packed array of char;
				accessmode: integer;
				filepath: integer	): boolean;
		{ check file whose name is in array filename to see	     }
		{ if it can be accessed using mode specified in accessmode.  }
		{ use the path selected by filepath to try to find file      }
		{ NB: >>> side effect <<< is to set array realfilename	     }
		{ to complete path name of located file.		     }
		{ Returns "TRUE" if found and can be accessed, else "FALSE"  }

*/

#define	R_OK	4
#undef	close			/* We're going to use the Unix close call */

/* These routines reference the following global variables and constants
   from Metafont and its associated utilities. If the name or type of
   these variables is changed in the Pascal source, be sure to make them
   match here!  */

#define READACCESS 4		/* args to test_access()	*/
#define WRITEACCESS 2

#define NOFILEPATH 0
#define TEXINPUTFILEPATH 1
#define TEXREADFILEPATH 2
#define TEXFONTFILEPATH 3
#define TEXFORMATFILEPATH 4
#define TEXPOOLFILEPATH 5
#define MFINPUTFILEPATH 6
#define MFBASEFILEPATH 7
#define MFPOOLFILEPATH 8

/* Fixed arrays are used to hold the paths, to avoid any possible
   problems involving interaction of malloc and undump.  */

char TeXinputpath[MAXPATHLENGTH] = TEXINPUTS;
char TeXfontpath[MAXPATHLENGTH] = TEXFONTS;
char TeXformatpath[MAXPATHLENGTH] = TEXFORMATS;
char TeXpoolpath[MAXPATHLENGTH] = TEXPOOL;
char MFinputpath[MAXPATHLENGTH] = MFINPUTS;
char MFbasepath[MAXPATHLENGTH] = MFBASES;
char MFpoolpath[MAXPATHLENGTH] = MFPOOL;

/* This copies at most n characters (including the null) from string s2
   to string s1, giving an error message for paths that are too long,
   identifying the PATH name.  */

static void 
path_copypath (s1, s2, n, pathname)
     register char *s1, *s2, *pathname;
     register int n;
{
  while ((*s1++ = *s2++) != '\0')
    if (--n == 0)
      {
	fprintf (stderr, "! Environment search path `%s' is too big.\n",
		 pathname);
	*--s1 = '\0';
	return;			/* let user continue with truncated path */
      }
}

/* This returns a char* pointer to the string selected by the path
   specifier, or NULL if no path or an illegal path selected. */

static char *
path_select (pathsel)
     integer pathsel;
{
  char *pathstring = NULL;

  switch (pathsel)
    {
    case TEXINPUTFILEPATH:
    case TEXREADFILEPATH:
      pathstring = TeXinputpath;
      break;
    case TEXFONTFILEPATH:
      pathstring = TeXfontpath;
      break;
    case TEXFORMATFILEPATH:
      pathstring = TeXformatpath;
      break;
    case TEXPOOLFILEPATH:
      pathstring = TeXpoolpath;
      break;
    case MFINPUTFILEPATH:
      pathstring = MFinputpath;
      break;
    case MFBASEFILEPATH:
      pathstring = MFbasepath;
      break;
    case MFPOOLFILEPATH:
      pathstring = MFpoolpath;
      break;
    default:
      pathstring = NULL;
    }
  return (pathstring);
}

/*
   path_buildname(filename, buffername, cpp)
	makes buffername contain the directory at *cpp,
	followed by '/', followed by the characters in filename
	up until the first blank there, and finally a '\0'.
	The cpp pointer is left pointing at the next directory
	in the path.

	But: if *cpp == NULL, then we are supposed to use filename as is.
*/

void 
path_buildname (filename, buffername, cpp)
     char *filename, *buffername;
     char **cpp;
{
  register char *p, *realname;

  realname = buffername;
  if ((p = *cpp) != NULL)
    {
      while ((*p != ':') && (*p != '\0'))
	{
	  *realname++ = *p++;
	  if (realname >= &(buffername[FILENAMESIZE - 1]))
	    break;
	}
      if (*p == '\0')
	*cpp = NULL;		/* at end of path now */
      else
	*cpp = p + 1;		/* else get past ':' */
      *realname++ = '/';	/* separate the area from the name to follow */
    }
  /* now append filename to buffername... */
  p = filename;
  if (*p == 0)
    p++;
  while (*p != ' ' && *p != 0)
    {
      if (realname >= &(buffername[FILENAMESIZE - 1]))
	{
	  fprintf (stderr, "! Full file name is too long\n");
	  break;
	}
      *realname++ = *p++;
    }
  *realname = '\0';
}

/*
 setpaths is called to set up the default path arrays as follows:
 if the user's environment has a value for the appropriate value,
 then use it;  otherwise, leave the current value of the array
 (which may be the default path, or it may be the result of
 a call to setpaths on a previous run that was made the subject of
 an undump: this will give the maker of a preloaded Metafont the
 option of providing a new set of "default" paths.

 Note that we have to copy the string from the environment area, since
 that will change on the next run (which matters if this is for a
 preloaded Metafont).
*/

void
setpaths ()
{
  register char *envpath;

  if ((envpath = getenv ("TEXINPUTS")) != NULL)
    path_copypath (TeXinputpath, envpath, MAXPATHLENGTH, "TEXINPUTS");
  if ((envpath = getenv ("TEXFONTS")) != NULL)
    path_copypath (TeXfontpath, envpath, MAXPATHLENGTH, "TEXFONTS");
  if ((envpath = getenv ("TEXFORMATS")) != NULL)
    path_copypath (TeXformatpath, envpath, MAXPATHLENGTH, "TEXFORMATS");
  if ((envpath = getenv ("TEXPOOL")) != NULL)
    path_copypath (TeXpoolpath, envpath, MAXPATHLENGTH, "TEXPOOL");
  if ((envpath = getenv ("MFINPUTS")) != NULL)
    path_copypath (MFinputpath, envpath, MAXPATHLENGTH, "MFINPUTS");
  if ((envpath = getenv ("MFBASES")) != NULL)
    path_copypath (MFbasepath, envpath, MAXPATHLENGTH, "MFBASES");
  if ((envpath = getenv ("MFPOOL")) != NULL)
    path_copypath (MFpoolpath, envpath, MAXPATHLENGTH, "MFPOOL");
}

/*
   testaccess(filename, realfilename, amode, filepath)

	Test whether or not the file whose name is in filename
	can be opened for reading (if mode=READACCESS)
	or writing (if mode=WRITEACCESS).

	The filepath argument is one of the ...FILEPATH constants
	defined above.  If the filename given in filename does not
	begin with '/', we try prepending all the ':'-separated directory
	names in the appropriate path to the filename until access
	can be made, if it ever can.

	The realfilename array will contain the name that
	yielded an access success.
*/

int 
ztestaccess (filename, realfilename, amode, filepath)
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

  do
    {
      path_buildname (filename, realfilename, &curpathplace);
      if (amode == READACCESS)
	{
	  /* use system call "access" to see if we could read it */
	  if (access (realfilename, 04) == 0)
	    ok = true;
	  else
	    ok = false;
	}
      else
	{
	  /* WRITEACCESS: use creat to see if we could create it, but close
          the file again if we're OK, to let pc open it for real */
	  if ((f = creat (realfilename, 0666)) >= 0)
	    {
	      ok = true;
	      (void) close (f);
	    }
	  else
	    ok = false;
	}
  } while (!ok && curpathplace != NULL);

  if (ok)
    {				/* pad realnameoffile with blanks, as Pascal wants */
      for (p = realfilename; *p != '\0'; p++)
	;			/* nothing: find end of string */
      while (p <= &(realfilename[FILENAMESIZE - 1]))
	*p++ = ' ';
    }

  return (ok);
}



/* On-line display routines.  Here we use a dispatch table indexed by
   the TERM environment variable to select the graphics routines
   appropriate to the user's terminal.  stdout must be connected to a
   terminal for us to do any graphics.  */

/* We don't want any other window routines screwing us up if we're
   trying to do the trap test.  We could have written another device for
   the trap test, but the terminal type conditionals in initscreen argue
   against that.  */

#ifdef TRAP
#undef SUNWIN
#undef HP2627WIN
#undef X10WIN
#undef X11WIN
#undef TEKTRONIXWIN
#endif


#ifdef	HP2627WIN
extern mf_hp2627_initscreen (), mf_hp2627_updatescreen ();
extern mf_hp2627_blankrectangle (), mf_hp2627_paintrow ();
#endif

#ifdef	SUNWIN
extern mf_sun_initscreen (), mf_sun_updatescreen ();
extern mf_sun_blankrectangle (), mf_sun_paintrow ();
#endif

#ifdef	TEKTRONIXWIN
extern mf_tektronix_initscreen (), mf_tektronix_updatescreen ();
extern mf_tektronix_blankrectangle (), mf_tektronix_paintrow ();
#endif

#ifdef	X10WIN
extern mf_x10_initscreen (), mf_x10_updatescreen ();
extern mf_x10_blankrectangle (), mf_x10_paintrow ();
#endif

#ifdef	X11WIN
extern mf_x11_initscreen (), mf_x11_updatescreen ();
extern mf_x11_blankrectangle (), mf_x11_paintrow ();
#endif

/* `mfwsw' contains the dispatch tables for each terminal.  We map the
   Pascal calls to the routines `init_screen', `update_screen',
   `blank_rectangle', and `paint_row' into the appropriate entry point
   for the specific terminal that MF is being run on.  */

struct mfwin_sw
{
  char *mfwsw_type;		/* Name of terminal a la TERMCAP.  */
  int (*mfwsw_initscreen) ();
  int (*mfwsw_updatescrn) ();
  int (*mfwsw_blankrect) ();
  int (*mfwsw_paintrow) ();
} mfwsw[] =

/* Now we have a long structure which initializes this array of
   ``Metafont window switches''.  */

{

#ifdef	HP2627WIN
  { "hp2627", mf_hp2627_initscreen, mf_hp2627_updatescreen,
    mf_hp2627_blankrectangle, mf_hp2627_paintrow },
#endif

#ifdef	SUNWIN
  { "sun", mf_sun_initscreen, mf_sun_updatescreen,
    mf_sun_blankrectangle, mf_sun_paintrow },
#endif

#ifdef	TEKTRONIXWIN
  { "tek", mf_tektronix_initscreen, mf_tektronix_updatescreen,
    mf_tektronix_blankrectangle, mf_tektronix_paintrow },
#endif

#ifdef	X10WIN
  { "xterm", mf_x10_initscreen, mf_x10_updatescreen,
    mf_x10_blankrectangle, mf_x10_paintrow },
#endif

#ifdef	X11WIN
  { "xterm",
    mf_x11_initscreen, mf_x11_updatescreen, 
    mf_x11_blankrectangle, mf_x11_paintrow },
#endif

  { "Irrelevant", NULL, NULL, NULL, NULL },

/* Finally, we must have an entry with a terminal type of NULL.  */
  { NULL, NULL, NULL, NULL, NULL }

}; /* End of the array initialization.  */


/* This is a pointer to current mfwsw[] entry.  */
static struct mfwin_sw *mfwp;

/* The following are routines that just jump to the correct
   terminal-specific graphics code. If none of the routines in the
   dispatch table exist, or they fail, we produce trap-compatible
   output, i.e., the same words and punctuation that the unchanged
   mf.web would produce.  */

/* This returns true if window operations legal, else false.  */

initscreen ()
{
#ifndef TRAP
  register char *ttytype;

  if ((ttytype = getenv ("TERM")) == NULL || !(isatty (fileno (stdout))))
    return (0);
  for (mfwp = mfwsw; mfwp->mfwsw_type != NULL; mfwp++)
    if (!strncmp (mfwp->mfwsw_type, ttytype, strlen (mfwp->mfwsw_type))
	|| !strcmp (ttytype, "emacs"))
      if (mfwp->mfwsw_initscreen)
	return ((*mfwp->mfwsw_initscreen) ());
      else
	{			/* Test terminal type */
	  printf ("Could not init_screen for `%s'.\n", ttytype);
	  return 1;
	}
  return 0;

#else /* TRAP */
  return 1;
#endif
}


/* Initialize things.  */

updatescreen ()
{
#ifndef TRAP
  if (mfwp->mfwsw_updatescrn)
    ((*mfwp->mfwsw_updatescrn) ());
  else
    {
      printf ("Updatescreen called\n");
    }
#else /* TRAP */
  fprintf (logfile, "Calling UPDATESCREEN\n");
#endif
}

/* This sets the rectangle bounded by ([left,right], [top,bottom]) to
   the background color.  */

blankrectangle (left, right, top, bottom)
     screencol left, right;
     screenrow top, bottom;
{
#ifndef TRAP
  if (mfwp->mfwsw_blankrect)
    ((*mfwp->mfwsw_blankrect) (left, right, top, bottom));
  else
    {
      printf ("Blankrectangle l=%d  r=%d  t=%d  b=%d\n",
	      left, right, top, bottom);
    }
#else /* TRAP */
  fprintf (logfile, "\nCalling BLANKRECTANGLE(%d,%d,%d,%d)\n", left,
	   right, top, bottom);
#endif
}

/* This paints ROW, starting with the color INIT_COLOR. 
   TRANSITION_VECTOR then specifies the length of the run; then we
   switch colors.  This goes on for VECTOR_SIZE transitions.  */

zpaintrow (row, init_color, transition_vector, vector_size)
     screenrow row;
     pixelcolor init_color;
     transspec transition_vector;
     screencol vector_size;
{
#ifndef TRAP
  if (mfwp->mfwsw_paintrow)
    ((*mfwp->mfwsw_paintrow) (row, init_color,
			      transition_vector, vector_size));
  else
    {
      printf ("Paintrow r=%d  c=%d  v=");
      while (vector_size-- > 0)
	printf ("%d  ", transition_vector++);
      printf ("\n");
    }
#else /* TRAP */
  unsigned k;

  fprintf (logfile, "Calling PAINTROW(%d,%d;", row, init_color);
  for (k = 0; k <= vector_size; k++)
    {
      fprintf (logfile, "%d", transition_vector[k]);
      if (k != vector_size)
	fprintf (logfile, ",");
    }
  fprintf (logfile, ")\n");
#endif
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
