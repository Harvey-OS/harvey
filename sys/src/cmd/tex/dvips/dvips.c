/*
 *   This is the main routine.
 */
#ifndef DEFRES
#define DEFRES (300)
#endif

#include "dvips.h" /* The copyright notice there is included too! */
#ifndef SYSV
extern char *strtok() ; /* some systems don't have this in strings.h */
#endif
#ifdef VMS
#define GLOBAL globaldef
#ifdef __GNUC__
#include "climsgdef.h"	/* created by hand, extracted from STARLET.MLB */
			/* and put in GNU_CC:[INCLUDE.LOCAL]           */
#include "ctype.h"
#include "descrip.h"
#else
#include climsgdef
#include ctype
#include descrip
#endif
#endif
/*
 *   First we define some globals.
 */
#ifdef VMS
    static char ofnme[252],infnme[252],pap[40],thh[20];
#endif
fontdesctype *fonthead ;      /* list of all fonts mentioned so far */
fontdesctype *curfnt ;        /* the currently selected font */
sectiontype *sections ;       /* sections to process document in */
Boolean manualfeed ;          /* manual feed? */
Boolean compressed ;          /* compressed? */
Boolean downloadpspk ;        /* use PK for downloaded PS fonts? */
Boolean safetyenclose ;
                          /* enclose in save/restore for stupid spoolers? */
Boolean removecomments = 1 ;  /* remove comments from included PS? */
Boolean nosmallchars ;        /* disable small char optimization for X4045? */
Boolean cropmarks ;           /* add cropmarks? */
Boolean abspage = 0 ;         /* are page numbers absolute? */
Boolean tryepsf = 0 ;         /* should we try to make it espf? */
Boolean secure = 0 ;          /* make safe for suid */
int collatedcopies = 1 ;      /* how many collated copies? */
int sectioncopies = 1 ;       /* how many times to repeat each section? */
integer pagecopies = 1 ;          /* how many times to repeat each page? */
shalfword linepos = 0 ;       /* where are we on the line being output? */
integer maxpages ;            /* the maximum number of pages */
Boolean notfirst, notlast ;   /* true if a first page was specified */
Boolean evenpages, oddpages ; /* true if doing only even/only odd pages */
Boolean pagelist ;            /* true if using page ranges */
Boolean sendcontrolD ;        /* should we send a control D at end? */
integer firstpage ;           /* the number of the first page if specified */
integer lastpage ;
integer firstseq ;
integer lastseq ;
integer hpapersize, vpapersize ; /* horizontal and vertical paper size */
integer hoff, voff ;          /* horizontal and vertical offsets */
integer maxsecsize ;          /* the maximum size of a section */
integer firstboploc ;         /* where the first bop is */
Boolean sepfiles ;            /* each section in its own file? */
int numcopies ;               /* number of copies of each page to print */
char *oname ;                 /* output file name */
char *iname ;                 /* dvi file name */
char *fulliname ;             /* same, with current working directory */
char *strings ;               /* strings for program */
char *nextstring, *maxstring ; /* string pointers */
FILE *dvifile, *bitfile ;     /* dvi and output files */
quarterword *curpos ;        /* current position in virtual character packet */
quarterword *curlim ;         /* final byte in virtual character packet */
fontmaptype *ffont ;          /* first font in current frame */
real conv ;                   /* conversion ratio, pixels per DVI unit */
real vconv ;                  /* conversion ratio, pixels per DVI unit */
real alpha ;                  /* conversion ratio, DVI unit per TFM unit */
#ifndef ATARIST
integer
#else
long
#endif
mag ;                         /* the magnification of this document */
integer num, den ;            /* the numerator and denominator */
int overridemag ;             /* substitute for mag value in DVI file? */
int actualdpi = DEFRES ;      /* the actual resolution of the printer */
int vactualdpi = DEFRES ;     /* the actual resolution of the printer */
int maxdrift ;                /* max pixels away from true rounded position */
int vmaxdrift ;               /* max pixels away from true rounded position */
char *paperfmt ;              /* command-line paper format */
int landscape = 0 ;           /* landscape mode */
integer fontmem ;             /* memory remaining in printer */
integer pagecount ;           /* page counter for the sections */
integer pagenum ;             /* the page number we currently look at */
long bytesleft ;              /* number of bytes left in raster */
quarterword *raster ;         /* area for raster manipulations */
integer hh, vv ;              /* horizontal and vertical pixel positions */

/*-----------------------------------------------------------------------*
 * The PATH definitions cannot be defined on the command line because so many
 * DEFINE's overflow the DCL buffer when using the GNU CC compiler.
 *-----------------------------------------------------------------------*/
#if defined(VMS) && defined(__GNUC__)
#include "vms_gcc_paths.h"
#endif

#ifdef ATARIST
#   define TFMPATH "."
#   define PKPATH "."
#   define VFPATH "."
#   define FIGPATH "."
#   define HEADERPATH "."
#   define CONFIGPATH "."
#endif
char *tfmpath = TFMPATH ;     /* pointer to directories for tfm files */
char *pkpath = PKPATH ;       /* pointer to directories for pk files */
char *vfpath = VFPATH ;       /* pointer to directories for vf files */
char *figpath = FIGPATH ;     /* pointer to directories for figure files */
char *headerpath = HEADERPATH ; /* pointer to directories for header files */
char *configpath = CONFIGPATH ; /* where to find config files */
char *infont ;                /* is the file we are downloading a font? */
#ifndef PICTPATH
#ifndef __THINK__
#define PICTPATH "."
#else
#define PICTPATH ":"
#endif
#endif
char *pictpath = PICTPATH ;   /* where IFF/etc. pictures are found */
#ifdef SEARCH_SUBDIRECTORIES
char *fontsubdirpath = FONTSUBDIRPATH ;
#endif
#ifdef FONTLIB
char *flipath = FLIPATH ;     /* pointer to directories for fli files */
char *fliname = FLINAME ;     /* pointer to names of fli files */
#endif
integer swmem ;               /* font memory in the PostScript printer */
int quiet ;                   /* should we only print errors to stderr? */
int filter ;                  /* act as filter default output to stdout,
                                               default input to stdin? */
int prettycolumn ;            /* the column we are at when running pretty */
int gargc ;                   /* global argument count */
char **gargv ;                /* global argument vector */
int totalpages = 0 ;          /* total number of pages */
Boolean reverse ;             /* are we going reverse? */
Boolean usesPSfonts ;         /* do we use local PostScript fonts? */
Boolean usesspecial ;         /* do we use \special? */
Boolean headers_off ;         /* do we send headers or not? */
Boolean usescolor ;           /* IBM: color - do we use colors? */
char *headerfile ;            /* default header file */
char *warningmsg ;            /* a message to write, if set in config file */
Boolean multiplesects ;       /* more than one section? */
Boolean disablecomments ;     /* should we suppress any EPSF comments? */
char *printer ;               /* what printer to send this to? */
char *mfmode ;                /* default MF mode */
frametype frames[MAXFRAME] ;  /* stack for virtual fonts */
fontdesctype *baseFonts[256] ; /* base fonts for dvi file */
integer pagecost;               /* memory used on the page being prescanned */
int delchar;                    /* characters to delete from prescanned page */
integer fsizetol;               /* max dvi units error for psfile font sizes */
Boolean includesfonts;          /* are fonts used in included psfiles? */
fontdesctype *fonthd[MAXFONTHD];/* list headers for included fonts of 1 name */
int nextfonthd;                 /* next unused fonthd[] index */
char xdig[256];                 /* table for reading hexadecimal digits */
char banner[] = BANNER ;        /* our startup message */
Boolean noenv = 0 ;             /* ignore PRINTER envir variable? */
Boolean dopprescan = 0 ;        /* do we do a scan before the prescan? */
integer lastheadermem ;         /* how much mem did the last header require? */
extern int dontmakefont ;
struct papsiz *papsizes ;       /* all available paper size */
int headersready ;              /* ready to check headers? */
#if defined(MSDOS) || defined(OS2) || defined(ATARIST)
char *mfjobname = NULL;         /* name of mfjob file if given */
FILE *mfjobfile = NULL;         /* mfjob file for font making instructions */
#endif
#ifdef DEBUG
integer debug_flag = 0;
#endif /* DEBUG */
char queryline[256];                /* interactive query of options */
int qargc;
char *qargv[32];
char queryoptions;
#ifdef RESEARCH
integer formsperpage;	      /* number of dvi pages to print per real page */
#endif
/*
 *   This routine calls the following externals:
 */
extern void outbangspecials() ;
extern void prescanpages() ;
extern void pprescanpages() ;
extern void initprinter() ;
extern void cleanprinter() ;
extern void dosection() ;
extern void getdefaults() ;
extern void cmdout() ;
extern void numout() ;
extern void initcolor() ;
extern int add_header() ;
extern int ParsePages() ;
extern void checkenv() ;
extern void getpsinfo(), revpslists() ;
#ifdef FONTLIB
extern void fliload() ;
#endif
#ifdef __THINK__
int dcommand(char ***);
#endif

/* Declare the routine to get the current working directory.  */

#ifndef IGNORE_CWD
#ifndef HAVE_GETCWD
extern char *getwd (); /* said to be faster than getcwd (SunOS man page) */
#define getcwd(b, len)  getwd(b) /* used here only when b nonnull */
#else
#ifdef ANSI
extern char *getcwd (char *, int);
#else
extern char *getcwd ();
#endif /* not ANSI */
#endif /* not HAVE_GETWD */
#if defined(SYSV) || defined(VMS) || defined(MSDOS) || defined(OS2) || defined(ATARIST)
#define MAXPATHLEN (256)
#else
#include <sys/param.h>          /* for MAXPATHLEN */
#endif
#endif /* not IGNORE_CWD */

static char *helparr[] = {
#ifndef VMCMS
"    Usage: dvips [options] filename[.dvi]",
#else
"    VM/CMS Usage:",
"           dvips fname [ftype [fmode]] [options]",
"or",
"           dvips fname[.ftype[.fmode]] [options]",
#endif
#ifdef RESEARCH
"a*  Conserve memory, not time      A   Print only odd (TeX) pages",
"b # Page copies, for posters e.g.  B   Print only even (TeX) pages",
"c # Uncollated copies              C # Collated copies",
"d # Debugging                      D # Resolution",
"e # Maxdrift value                 E*  Try to create EPSF",
"f*  Run as filter                  F*  Send control-D at end",
"h f Add header file                H f Add header file",
"i*  Separate file per section      G c Specify desired page size",
"k*  Print crop marks               I c DVI pages per physical page",
"l # Last page                      J c Printer memory",
"m*  Manual feed                    K*  Pull comments from inclusions",
"n # Maximum number of pages        L   Landscape mode",
"o f Output file                    M*  Don't make fonts",
"p # First page                     N*  No structured comments",
"q*  Run quietly                    O c Set/change paper offset",
"r*  Reverse order of pages         P s Load config.$s",
"s*  Enclose output in save/restore R   Run securely",
"t s Paper format                   S # Max section size in pages",
"x # Override dvi magnification     U*  Disable string param trick",  
"                                   V*  Send downloadable PS fonts as PK",
"                                   X # Horizontal resolution",
"                                   Y # Vertical resolution",
"                                   Z*  Compress bitmap fonts",
#else
"a*  Conserve memory, not time      y # Multiply by dvi magnification",
"b # Page copies, for posters e.g.  A   Print only odd (TeX) pages",
"c # Uncollated copies              B   Print only even (TeX) pages",
"d # Debugging                      C # Collated copies",
"e # Maxdrift value                 D # Resolution",
"f*  Run as filter                  E*  Try to create EPSF",
"h f Add header file                F*  Send control-D at end",
"i*  Separate file per section      K*  Pull comments from inclusions",
"k*  Print crop marks               M*  Don't make fonts",
"l # Last page                      N*  No structured comments",
"m*  Manual feed                    O c Set/change paper offset",
#if defined(MSDOS) || defined(OS2)
"n # Maximum number of pages        P s Load $s.cfg",
#else
"n # Maximum number of pages        P s Load config.$s",
#endif
"o f Output file                    R   Run securely",
"p # First page                     S # Max section size in pages",
"q*  Run quietly                    T c Specify desired page size",
"r*  Reverse order of pages         U*  Disable string param trick",
"s*  Enclose output in save/restore V*  Send downloadable PS fonts as PK",
"t s Paper format                   X # Horizontal resolution",
"x # Override dvi magnification     Y # Vertical resolution",  
"                                   Z*  Compress bitmap fonts",
#endif
/* "-   Interactive query of options", */
"    # = number   f = file   s = string  * = suffix, `0' to turn off",
"    c = comma-separated dimension pair (e.g., 3.2in,-32.1cm)", 0} ;
void help() {
   char **p ;
   for (p=helparr; *p; p++)
      fprintf(stderr, " %s\n", *p) ;
}
/*
 *   This error routine prints an error message; if the first
 *   character is !, it aborts the job.
 */
static char *progname ;
void
error(s)
        char *s ;
{
   extern void exit() ;

   if (prettycolumn > 0)
        fprintf(stderr,"\n");
   prettycolumn = 0;
   (void)fprintf(stderr, "%s: %s\n", progname, s) ;
   if (*s=='!') {
      if (bitfile != NULL) {
         cleanprinter() ;
      }
      exit(1) ; /* fatal */
   }
}
/*
 *   This is our malloc that checks the results.  We debug the
 *   allocations but not the frees, since memory fragmentation
 *   might be such that we can never use the free'd memory and
 *   it's wise to be conservative.  The only real place we free
 *   is when repacking *huge* characters anyway.
 */
#ifdef DEBUG
static integer totalalloc = 0 ;
#endif
char *mymalloc(n)
integer n ;
{
   char *p ;

#ifdef SMALLMALLOC
   if (n > 65500L)
      error("! can't allocate more than 64K!") ;
#endif
   if (n <= 0) /* catch strange 0 mallocs in flib.c without breaking code */
      n = 1 ;
#ifdef DEBUG
   totalalloc += n ;
   if (dd(D_MEM)) {
#ifdef SHORTINT
      fprintf(stderr, "Alloc %ld\n", n) ;
#else
      fprintf(stderr, "Alloc %d\n", n) ;
#endif
   }
#endif
   p = malloc(n) ;
   if (p == NULL)
      error("! no memory") ;
   return p ;
}
void
morestrings() {
   strings = mymalloc((integer)STRINGSIZE) ;
   nextstring = strings ;
   maxstring = strings + STRINGSIZE - 200 ;
   *nextstring++ = 0 ;
}
void
checkstrings() {
   if (nextstring - strings > STRINGSIZE / 2)
      morestrings() ;
}
/*
 *   Initialize sets up all the globals and data structures.
 */
void
initialize()
{
   int i;
   char *s;

   nextfonthd = 0;
   for (i=0; i<256; i++)
      xdig[i] = 0;
   i = 0;
   for (s="0123456789ABCDEF"; *s!=0; s++)
      xdig[(int)*s] = i++;
   i = 10;
   for (s="abcdef"; *s!=0; s++)
      xdig[(int)*s] = i++;
   morestrings() ;
   maxpages = 100000 ;
   numcopies = 1 ;
   iname = fulliname = strings ;
   bitfile = NULL ;
   bytesleft = 0 ;
   swmem = SWMEM ;
   oname = OUTPATH ;
   sendcontrolD = 0 ;
   multiplesects = 0 ;
   disablecomments = 0 ;
   maxdrift = -1 ;
   vmaxdrift = -1 ;
#ifdef RESEARCH
   formsperpage = 1;
#endif
}
/*
 *   This routine copies a string into the string `pool', safely.
 */
char *
newstring(s)
   char *s ;
{
   int l ;

   if (s == NULL)
      return(NULL) ;
   l = strlen(s) ;
   if (nextstring + l >= maxstring)
      morestrings() ;
   if (nextstring + l >= maxstring)
      error("! out of string space") ;
   (void)strcpy(nextstring, s) ;
   s = nextstring ;
   nextstring += l + 1 ;
   return(s) ;
}
void newoutname() {
   static int seq = 0 ;
   static char *seqptr = 0 ;
   char *p ;

   if (oname == 0 || *oname == 0)
      error("! need an output file name to specify separate files") ;
   if (*oname != '!' && *oname != '|') {
      if (seqptr == 0) {
         oname = newstring(oname) ;
         seqptr = 0 ;
         for (p = oname; *p; p++)
            if (*p == '.')
               seqptr = p + 1 ;
         if (seqptr == 0)
            seqptr = p ;
         nextstring += 5 ; /* make room for the number, up to five digits */
      }
      sprintf(seqptr, "%03d", ++seq) ;
   }
}
/*
 *   This routine reverses a list, where a list is defined to be any
 *   structure whose first element is a pointer to another such structure.
 */
VOID *revlist(p)
VOID *p ;
{
   struct list {
      struct list *next ;
   } *pp = (struct list *)p, *qq = 0, *tt ;

   while (pp) {
      tt = pp->next ;
      pp->next = qq ;
      qq = pp ;
      pp = tt ;
   }
   return (VOID *)qq ;
}
/* this asks for a new set of arguments from the command line */
void
queryargs()
{
   fputs("Options: ",stdout);
   fgets(queryline,256,stdin);
   qargc=1;
   if ( (qargv[1] = strtok(queryline," \n")) != (char *)NULL ) {
      qargc=2;
      while ( ((qargv[qargc] = strtok((char *)NULL," \n")) != (char *)NULL)
            && (qargc < 31) )
         qargc++;
   }
   qargv[qargc] = (char *)NULL;
}
 
/*
 *   Finally, our main routine.
 */
extern void handlepapersize() ;
#ifdef VMS
main()
#else
void main(argc, argv)
        int argc ;
        char *argv[] ;
#endif
{
   extern void exit() ;
   int i, lastext = -1 ;
#ifdef MVSXA
   int firstext = -1 ;
#endif
   register sectiontype *sects ;

#ifdef __THINK__
   argc = dcommand(&argv) ; /* do I/O stream redirection */
#endif
#ifdef VMS		/* Grab the command-line buffer */
   short len_arg;
   $DESCRIPTOR( verb_dsc, "DVIPS ");	/* assume the verb is always DVIPS */
   struct dsc$descriptor_d temp_dsc = { 0, DSC$K_DTYPE_T, DSC$K_CLASS_D, 0};

   progname = &thh[0] ;
   strcpy(progname,"DVIPS%ERROR");

   lib$get_foreign( &temp_dsc, 0, &len_arg, 0);	/* Get the command line */
   str$prefix(&temp_dsc, &verb_dsc);		/* prepend the VERB     */
   len_arg += verb_dsc.dsc$w_length;		/* update the length    */
   temp_dsc.dsc$a_pointer[len_arg] = '\0';	/* terminate the string */
   gargv = &temp_dsc.dsc$a_pointer;		/* point to the buffer  */
   gargc = 1 ;					/* only one big argv    */
#else
   progname = argv[0] ;
   gargv = argv ;
   gargc = argc ;
/* we sneak a look at the first arg in case it's debugging */
#ifdef DEBUG
   if (argc > 1 && strncmp(argv[1], "-d", 2)==0) {
      if (argv[1][2]==0) {
         if (sscanf(argv[2], "%d", &debug_flag)==0)
            debug_flag = 0 ;
      } else {
         if (sscanf(argv[1]+2, "%d", &debug_flag)==0)
            debug_flag = 0 ;
      }
   }
#endif
#endif
   initialize() ;
   checkenv(0) ;
   getdefaults(CONFIGFILE) ;
   getdefaults((char *)0) ;
/*
 *   This next whole big section of code is straightforward; we just scan
 *   the options.  An argument can either immediately follow its option letter
 *   or be separated by spaces.  Any argument not preceded by '-' and an
 *   option letter is considered a file name; the program complains if more
 *   than one file name is given, and uses stdin if none is given.
 */
#ifdef VMS
vmscli();
#else
#ifdef RESEARCH
/*
 *   For the Bell Labs Research version, we make it look a little more
 *   like the lp software.
 *   Formsperpage is settable from the command line with -I
 *   Memory is settable from the command line with -J.
 *   Landscape mode is selectable from the command line with -L.
 *
 *   Finally, -T  is a synonym for -P, but we do a prescan for -T
 *   so that any other command line options will override the -T,
 *   independent of the option order.  Unfortunately, dvips later
 *   started using -T for papersize, so we let -G be used for papersize
 *   instead (too many people know about -T now)
 */
   for (i=1; i<argc; i++) {
      if (argv[i][0]=='-' && argv[i][1]=='T') {
	    char *p=argv[i]+2;
            if (*p == 0 && argv[i+1])
               p = argv[++i] ;
	    if (noenv)
		error("! More than one -T option.") ;
            printer = p ;
            noenv = 1 ;
            getdefaults("") ;
      }
   }
#endif /* RESEARCH */
   queryoptions = 0;
   do
   {
      for (i=1; i<argc; i++) {
         if (*argv[i]=='-') {
            char *p=argv[i]+2 ;
            char c=argv[i][1] ;
            switch (c) {
case '-':
               queryoptions = 1;
               break;
case 'a':
               dopprescan = (*p != '0') ;
               break ;
case 'b':
               if (*p == 0 && argv[i+1])
                  p = argv[++i] ;
               if (sscanf(p, "%d", &pagecopies)==0)
                  error("! Bad number of page copies option (-b).") ;
               if (pagecopies < 1 || pagecopies > 1000)
                  error("! can only print one to a thousand page copies") ;
               break ;
case 'c' :
               if (*p == 0 && argv[i+1])
                  p = argv[++i] ;
               if (sscanf(p, "%d", &numcopies)==0)
                  error("! Bad number of copies option (-c).") ;
               break ;
case 'd' :
#ifdef DEBUG
               if (*p == 0 && argv[i+1])
                  p = argv[++i];
               if (sscanf(p, "%d", &debug_flag)==0)
                  error("! Bad debug option (-d).");
               break;
#else
               error("not compiled in debug mode") ;
               break ;
#endif /* DEBUG */
case 'e' :
               if (*p == 0 && argv[i+1])
                  p = argv[++i] ;
               if (sscanf(p, "%d", &maxdrift)==0 || maxdrift<0)
                  error("! Bad maxdrift option (-e).") ;
               vmaxdrift = maxdrift;
               break ;
case 'f' :
               filter = (*p != '0') ;
               if (filter)
                  oname = "" ;
               noenv = 1 ;
               sendcontrolD = 0 ;
               break ;
case 'h' : case 'H' :
               if (*p == 0 && argv[i+1])
                  p = argv[++i] ;
               if (strcmp(p, "-") == 0)
                  headers_off = 1 ;
               else
                  (void)add_header(p) ;
               break ;
case 'i':
               sepfiles = (*p != '0') ;
               break ;
case 'k':
               cropmarks = (*p != '0') ;
               break ;
case 'R':
               secure = 1 ;
               break ;
case 'S':
               if (*p == 0 && argv[i+1])
                  p = argv[++i] ;
               if (sscanf(p, "%d", &maxsecsize)==0)
                  error("! Bad section size arg (-S).") ;
               break ;
case 'm' :
               manualfeed = (*p != '0') ;
               break ;
case 'n' :
               if (*p == 0 && argv[i+1])
                  p = argv[++i] ;
#ifdef SHORTINT
               if (sscanf(p, "%ld", &maxpages)==0)
#else        /* ~SHORTINT */
               if (sscanf(p, "%d", &maxpages)==0)
#endif        /* ~SHORTINT */
                  error("! Bad number of pages option (-n).") ;
               break ;
case 'o' :
               if (*p == 0 && argv[i+1] && *argv[i+1]!='-')
                  p = argv[++i] ;
               oname = p ;
               noenv = 1 ;
               sendcontrolD = 0 ;
               break ;
case 'O' :
               if (*p == 0 && argv[i+1])
                  p = argv[++i] ;
               handlepapersize(p, &hoff, &voff) ;
               break ;
#ifdef RESEARCH
case 'G' :
#else
case 'T' :
#endif
               if (*p == 0 && argv[i+1])
                  p = argv[++i] ;
               handlepapersize(p, &hpapersize, &vpapersize) ;
               if (landscape) {
                  error(
              "both landscape and papersize specified; ignoring landscape") ;
                  landscape = 0 ;
               }
               break ;
case 'p' :
#if defined(MSDOS) || defined(OS2) || defined(ATARIST)
               /* check for emTeX job file (-pj=filename) */
               if (*p == 'j') {
                 p++;
                 if (*p == '=' || *p == ':')
                   p++;
                 mfjobname = newstring(p);
                 break;
               }
               /* must be page number instead */
#endif
               if (*p == 'p') {  /* a -pp specifier for a page list? */
                  p++ ;
                  if (*p == 0 && argv[i+1])
                     p = argv[++i] ;
                  if (ParsePages(p))
                     error("! Bad page list specifier (-pp).") ;
                  pagelist = 1 ;
                  break ;
               }
               if (*p == 0 && argv[i+1])
                  p = argv[++i] ;
               if (*p == '=') {
                  abspage = 1 ;
                  p++ ;
               }
#ifdef SHORTINT
               switch(sscanf(p, "%ld.%ld", &firstpage, &firstseq)) {
#else        /* ~SHORTINT */
               switch(sscanf(p, "%d.%d", &firstpage, &firstseq)) {
#endif        /* ~SHORTINT */
case 1:           firstseq = 0 ;
case 2:           break ;
default:
                  error("! Bad first page option (-p).") ;
               }
               notfirst = 1 ;
               break ;
case 'l':
               if (*p == 0 && argv[i+1])
                  p = argv[++i] ;
               if (*p == '=') {
                  abspage = 1 ;
                  p++ ;
               }
#ifdef SHORTINT
               switch(sscanf(p, "%ld.%ld", &lastpage, &lastseq)) {
#else        /* ~SHORTINT */
               switch(sscanf(p, "%d.%d", &lastpage, &lastseq)) {
#endif        /* ~SHORTINT */
case 1:           lastseq = 0 ;
case 2:           break ;
default:
                  error("! Bad last page option (-l).") ;
               }
               notlast = 1 ;
               break ;
case 'A':
               oddpages = 1 ;
               break ;
case 'B':
               evenpages = 1 ;
               break ;
case 'q' : case 'Q' :
               quiet = (*p != '0') ;
               break ;
case 'r' :
               reverse = (*p != '0') ;
               break ;
case 't' :
               if (*p == 0 && argv[i+1])
                  p = argv[++i] ;
               if (strcmp(p, "landscape") == 0) {
                  if (hpapersize || vpapersize)
                     error(
             "both landscape and papersize specified; ignoring landscape") ;
                  else
                     landscape = 1 ;
               } else
                  paperfmt = p ;
               break ;
case 'x' : case 'y' :
               if (*p == 0 && argv[i+1])
                  p = argv[++i] ;
#ifndef ATARIST
               if (sscanf(p, "%d", &mag)==0 || mag < 10 ||
#else
               if (sscanf(p, "%ld", &mag)==0 || mag < 10 ||
#endif
                          mag > 100000)
                  error("! Bad magnification parameter (-x).") ;
               overridemag = (c == 'x' ? 1 : -1) ;
               break ;
case 'C' :
               if (*p == 0 && argv[i+1])
                  p = argv[++i] ;
               if (sscanf(p, "%d", &collatedcopies)==0)
                  error("! Bad number of collated copies option (-C).") ;
               break ;
case 'D' :
               if (*p == 0 && argv[i+1])
                  p = argv[++i] ;
               if (sscanf(p, "%d", &actualdpi)==0 || actualdpi < 10 ||
                          actualdpi > 10000)
                  error("! Bad dpi parameter (-D).") ;
               vactualdpi = actualdpi;
               break ;
case 'E' :
               tryepsf = (*p != '0') ;
               break ;
case 'K' :
               removecomments = (*p != '0') ;
               break ;
#ifdef RESEARCH
case 'L' :
	    landscape = 1;
	    break;
case 'I' :
            if (*p == 0 && argv[i+1])
               p = argv[++i] ;
#ifdef SHORTINT
            if (sscanf(p, "%ld", &formsperpage)==0)
#else	/* ~SHORTINT */
            if (sscanf(p, "%d", &formsperpage)==0)
#endif	/* ~SHORTINT */
               error("! bad forms-per-page option (-I).") ;
            break ;
case 'J' :
            if (*p == 0 && argv[i+1])
               p = argv[++i] ;
#ifdef SHORTINT
            if (sscanf(p, "%ld", &swmem)==0)
#else	/* ~SHORTINT */
            if (sscanf(p, "%d", &swmem)==0)
#endif	/* ~SHORTINT */
               error("! Bad memory option (-J).") ;
            break;
case 'T' :
	    /* handled in prepass */
            if (*p == 0 && argv[i+1])
               ++i ;
            break ;
#endif
case 'U' :
               nosmallchars = (*p != '0') ;
               break ;
case 'X' :
               if (*p == 0 && argv[i+1])
                  p = argv[++i] ;
               if (sscanf(p, "%d", &actualdpi)==0 || actualdpi < 10 ||
                          actualdpi > 10000)
                  error("! Bad dpi parameter (-D).") ;
               break ;
case 'Y' :
               if (*p == 0 && argv[i+1])
                  p = argv[++i] ;
               if (sscanf(p, "%d", &vactualdpi)==0 || vactualdpi < 10 ||
                          vactualdpi > 10000)
                  error("! Bad dpi parameter (-D).") ;
               vactualdpi = vactualdpi;
               break ;
case 'F' :
               sendcontrolD = (*p != '0') ;
               break ;
case 'M':
               dontmakefont = (*p != '0') ;
               break ;
case 'N' :
               disablecomments = (*p != '0') ;
               break ;
case 'P' :
               {
                  struct papsiz *opapsiz = papsizes ;
                  struct papsiz *npapsiz ;
                  papsizes = 0 ;
                  if (*p == 0 && argv[i+1])
                     p = argv[++i] ;
                  printer = p ;
                  noenv = 1 ;
                  getdefaults("") ;
		  npapsiz = opapsiz ;
                  while (npapsiz && npapsiz->next)
                     npapsiz = npapsiz->next ;
                  if (npapsiz) {
                     npapsiz->next = papsizes ;
                     papsizes = opapsiz ;
                  }
	       }
               break ;
case 's' :
               safetyenclose = (*p != '0') ;
               break ;
case 'V':
               downloadpspk = (*p != '0') ;
               break ;
case 'Z' :
               compressed = (*p != '0') ;
               break ;
case '?' :
               (void)fprintf(stderr, banner) ;
               help() ;
               break ;
default:
#ifdef RESEARCH
              /* flags G, J, K are extra; rest were forgotten in original */
              error(
              "! Bad option, not one of abcdefhiklmnopqrstxyABCDEFGHIJKLMNOPRSTUVXYZ?") ;
#else
              error(
              "! Bad option, not one of acdefhiklmnopqrstxCDEFKMNOPSTUXYZ?") ;
#endif
            }
         } else {
            if (*iname == 0) {
               register char *p ;
   
               lastext = 0 ;
               iname = nextstring ;
               p = argv[i] ;
               while (*p) {
                  *nextstring = *p++ ;
                  if (*nextstring == '.')
                     lastext = nextstring - iname ;
                  else if (*nextstring == '/' || *nextstring == ':')
                     lastext = 0 ;
                  nextstring++ ;
               }
               *nextstring++ = '.' ;
               *nextstring++ = 'd' ;
               *nextstring++ = 'v' ;
               *nextstring++ = 'i' ;
               *nextstring++ = 0 ;
            } else
               error("! Two input file names specified.") ;
         }
      }
      if (noenv == 0) {
         register char *p ;
         struct papsiz *opapsiz = papsizes ;
         extern char *getenv() ;
         papsizes = 0 ;
	 if (0 != (p = getenv("PRINTER"))) {
#if defined(MSDOS) || defined(OS2)
            strcpy(nextstring, p) ;
            strcat(nextstring, ".cfg") ;
#else
            strcpy(nextstring, "config.") ;
            strcat(nextstring, p) ;
#endif
            getdefaults(nextstring) ;
         }
         {
            struct papsiz *npapsiz = opapsiz ;
            while (npapsiz && npapsiz->next)
               npapsiz = npapsiz->next ;
            if (npapsiz) {
               npapsiz->next = papsizes ;
               papsizes = opapsiz ;
            }
         }
      }
      papsizes = (struct papsiz *)revlist((void *)papsizes) ;
      if (queryoptions != 0) {            /* get new options */
         (void)fprintf(stderr, banner) ;
         help() ;
         queryargs();
         if (qargc == 1)
           queryoptions = 0;
         else {
           qargv[0] = argv[0];
           argc=qargc;
           argv=qargv;
         }
      }
   } while (queryoptions != 0) ;
#endif
   checkenv(1) ;
/*
 *   The logic here is a bit convoluted.  Since all `additional'
 *   PostScript font information files are loaded *before* the master
 *   one, and yet they should be able to override the master one, we
 *   have to add the information in the master list to the *ends* of
 *   the hash chain.  We do this by reversing the lists, adding them
 *   to the front, and then reversing them again.
 */
   revpslists() ;
   getpsinfo((char *)NULL) ;
   revpslists() ;
   if (!quiet)
      (void)fprintf(stderr, banner) ;
   if (*iname) {
      dvifile = fopen(iname, READBIN) ;
/*
 *   Allow names like a.b.
 */
      if (dvifile == 0) {
         iname[strlen(iname)-4] = 0 ; /* remove the .dvi suffix */
         dvifile = fopen(iname, READBIN) ;
      }
   }
   if (oname[0] == '-' && oname[1] == 0)
      oname[0] = 0 ;
   if (*oname == 0 && ! filter) {
      oname = nextstring ;
#ifndef VMCMS  /* get stuff before LAST "." */
      lastext = strlen(iname) - 1 ;
      while (iname[lastext] != '.' && lastext > 0)
         lastext-- ;
      if (iname[lastext] != '.')
         lastext = strlen(iname) - 1 ;
#else   /* for VM/CMS we take the stuff before FIRST "." */
      lastext = strchr(iname,'.') - iname ;
      if ( lastext <= 0 )     /* if no '.' in "iname" */
         lastext = strlen(iname) -1 ;
#endif
#ifdef MVSXA /* IBM: MVS/XA */
      if (strchr(iname, '(') != NULL  &&
          strchr(iname, ')') != NULL) {
      firstext = strchr(iname, '(') - iname + 1;
      lastext = strrchr(iname, ')') - iname - 1;
         }
      else {
      if (strrchr(iname, '.') != NULL) {
      lastext = strrchr(iname, '.') - iname - 1;
           }
         else lastext = strlen(iname) - 1 ;
      if (strchr(iname, '\'') != NULL)
         firstext = strchr(iname, '.') - iname + 1;
         else firstext = 0;
      }
#endif  /* IBM: MVS/XA */
#ifdef MVSXA /* IBM: MVS/XA */
      for (i=firstext; i<=lastext; i++)
#else
      for (i=0; i<=lastext; i++)
#endif
         *nextstring++ = iname[i] ;
      if (iname[lastext] != '.')
         *nextstring++ = '.' ;
#ifndef VMCMS
      *nextstring++ = 'p' ;
      *nextstring++ = 's' ;
#else  /* might as well keep things uppercase */
      *nextstring++ = 'P' ;
      *nextstring++ = 'S' ;
#endif
      *nextstring++ = 0 ;
/*
 *   Now we check the name, and `throw away' any prefix information.
 *   This means throwing away anything before (and including) a colon
 *   or slash.
 */
      {
         char *p ;

         for (p=oname; *p && p[1]; p++)
            if (*p == ':' || *p == DIRSEP)
               oname = p + 1 ;
      }
   }
#ifdef DEBUG
   if (dd(D_PATHS)) {
#ifdef SHORTINT
        (void)fprintf(stderr,"input file %s output file %s swmem %ld\n",
#else /* ~SHORTINT */
           (void)fprintf(stderr,"input file %s output file %s swmem %d\n",
#endif /* ~SHORTINT */
           iname, oname, swmem) ;
   (void)fprintf(stderr,"tfm path %s\npk path %s\n", tfmpath, pkpath) ;
   (void)fprintf(stderr,"fig path %s\nvf path %s\n", figpath, vfpath) ;
   (void)fprintf(stderr,"config path %s\nheader path %s\n",
                  configpath, headerpath) ;
#ifdef FONTLIB
   (void)fprintf(stderr,"fli path %s\nfli names %s\n", flipath, fliname) ;
#endif
   } /* dd(D_PATHS) */
#endif /* DEBUG */
/*
 *   Now we try to open the dvi file.
 */
   if (warningmsg)
      error(warningmsg) ;
   headersready = 1 ;
   headerfile = (compressed? CHEADERFILE : HEADERFILE) ;
   (void)add_header(headerfile) ;
   if (*iname != 0) {
      fulliname = nextstring ;
#ifndef IGNORE_CWD
      if (*iname != '/') {
        getcwd(nextstring, MAXPATHLEN + 2);
        while (*nextstring++) ;
#ifdef VMS		/* VMS doesn't need the '/' character appended */
        nextstring--;	/* so just back up one byte. */
#else
        *(nextstring-1) = '/' ;
#endif
      }
#endif
      strcpy(nextstring,iname) ;
      while (*nextstring++) ; /* advance nextstring past fulliname */
   } else if (filter)
      dvifile = stdin;
   else {
      help() ;
      exit(0) ;
   }
   initcolor() ;
   if (dvifile==NULL) {
      extern char errbuf[];
      (void)sprintf(errbuf,"! DVI file <%s> can't be opened.", iname) ;
      error("! DVI file can't be opened.") ;
   }
   if (fseek(dvifile, 0L, 0) < 0)
      error("! DVI file must not be a pipe.") ;
#ifdef FONTLIB
   fliload();    /* read the font libaries */
#endif
/*
 *   Now we do our main work.
 */
   swmem += fontmem ;
   if (maxdrift < 0) {
      if (actualdpi <= 599)
         maxdrift = actualdpi / 100 ;
      else if (actualdpi < 1199)
         maxdrift = actualdpi / 200 + 3 ;
      else
         maxdrift = actualdpi / 400 + 6 ;
   }
   if (vmaxdrift < 0) {
      if (vactualdpi <= 599)
         vmaxdrift = vactualdpi / 100 ;
      else if (vactualdpi < 1199)
         vmaxdrift = vactualdpi / 200 + 3 ;
      else
         vmaxdrift = vactualdpi / 400 + 6 ;
   }
   if (dopprescan)
      pprescanpages() ;
   prescanpages() ;
#if defined MSDOS || defined OS2 || defined(ATARIST)
   if (mfjobfile != (FILE*)NULL) {
     char answer[5];
     fputs("}\n",mfjobfile);
     fclose(mfjobfile);
     fputs("Exit to make missing fonts now (y/n)? ",stdout);
     fgets(answer,4,stdin);
     if (*answer=='y' || *answer=='Y')
       exit(8); /* exit with errorlevel 8 for emTeX dvidrv */
   }
#endif
   if (includesfonts)
      (void)add_header(IFONTHEADER) ;
   if (usesPSfonts)
      (void)add_header(PSFONTHEADER) ;
   if (usesspecial)
      (void)add_header(SPECIALHEADER) ;
   if (usescolor)  /* IBM: color */
      (void)add_header(COLORHEADER) ;
#ifdef RESEARCH
   if (formsperpage > 1)
      (void)add_header(FORMSFILE) ;
#endif
   sects = sections ;
   totalpages *= collatedcopies ;
   if (sects == NULL || sects->next == NULL) {
      sectioncopies = collatedcopies ;
      collatedcopies = 1 ;
   } else {
      if (! sepfiles)
         multiplesects = 1 ;
   }
   totalpages *= pagecopies ;
   if (tryepsf) {
      if (totalpages != 1 || paperfmt || landscape || manualfeed ||
          collatedcopies > 1 || numcopies > 1 || cropmarks ||
          *iname == 0) {
         error("Can't make it EPSF, sorry") ;
         tryepsf = 0 ;
      }
   }
   if (! sepfiles) {
      initprinter(0) ;
      outbangspecials() ;
   }
   for (i=0; i<collatedcopies; i++) {
      sects = sections ;
      while (sects != NULL) {
         if (sepfiles) {
            newoutname() ;
            if (! quiet) {
               if (prettycolumn + strlen(oname) + 6 > STDOUTSIZE) {
                  fprintf(stderr, "\n") ;
                  prettycolumn = 0 ;
               }
               (void)fprintf(stderr, "(-> %s) ", oname) ;
               prettycolumn += strlen(oname) + 6 ;
            }
            initprinter(sects->numpages) ;
            outbangspecials() ;
         } else if (! quiet) {
            if (prettycolumn > STDOUTSIZE) {
               fprintf(stderr, "\n") ;
               prettycolumn = 0 ;
            }
            (void)fprintf(stderr, ". ") ;
            prettycolumn += 2 ;
         }
         (void)fflush(stderr) ;
         dosection(sects, sectioncopies) ;
         sects = sects->next ;
         if (sepfiles)
            cleanprinter() ;
      }
   }
   if (! sepfiles)
      cleanprinter() ;
   if (! quiet)
      (void)fprintf(stderr, "\n") ;
#ifdef DEBUG
   if (dd(D_MEM)) {
#ifdef SHORTINT
      fprintf(stderr, "Total memory allocated:  %ld\n", totalalloc) ;
#else
      fprintf(stderr, "Total memory allocated:  %d\n", totalalloc) ;
#endif
   }
#endif
   exit(0) ;
   /*NOTREACHED*/
}
#ifdef VMS
#include "[]vmscli.c"
#endif

#ifdef VMCMS  /* IBM: VM/CMS */
#include "dvipscms.h"
#endif

#ifdef MVSXA  /* IBM: MVS/XA */
#include "dvipsmvs.h"
#endif
