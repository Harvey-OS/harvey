/*
 *   OUTPATH is where to send the output.  If you want a .ps file to
 *   be created by default, set this to "".  If you want to automatically
 *   invoke a pipe (as in lpr), make the first character an exclamation
 *   point or a vertical bar, and the remainder the command line to
 *   execute.
 */
#define OUTPATH ""
/*   (Actually OUTPATH will be overridden by an `o' line in config.ps.) */
/*
 *   Names of config and prologue files:
 */
#if defined MSDOS || defined OS2 || defined(ATARIST)
#define DVIPSRC "dvips.ini"
#else
#ifdef VMCMS  /* IBM: VM/CMS */
#define DVIPSRC "dvips.profile"
#else
#ifdef MVSXA  /* IBM: MVS/XA */
#define DVIPSRC "dvips.profile"
#else
#define DVIPSRC ".dvipsrc"
#endif  /* IBM: VM/CMS */
#endif
#endif

#define HEADERFILE "tex.pro"
#define CHEADERFILE "texc.pro"
#define PSFONTHEADER "texps.pro"
#define IFONTHEADER "finclude.pro"
#define SPECIALHEADER "special.pro"
#define COLORHEADER "color.pro"  /* IBM: color */
#define CROPHEADER "crop.pro"
#define PSMAPFILE "psfonts.map"
#ifndef CONFIGFILE
#define CONFIGFILE "config.ps"
#endif
#ifdef RESEARCH
#define FORMSFILE "forms.pro"
#endif

/* arguments to fopen */
#define READ            "r"

/* directories are separated in the path by PATHSEP */
/* DIRSEP is the char that separates directories from files */
#ifdef __THINK__
#define READBIN		"rb"	/* Macintosh OS will use binary mode */
#define PATHSEP         ',' /* use same syntax as VMS */
#define DIRSEP		':'
#else
#if defined MSDOS || defined OS2 || defined(ATARIST)
#define READBIN		"rb"	/* MSDOS and OS/2 must use binary mode */
#define PATHSEP         ';'
#define DIRSEP		'\\'
#else
#ifdef VMS
#define READBIN		"rb"	/* VMS must use binary mode */
#define PATHSEP         ','
#define DIRSEP		':'
#else
#ifdef VMCMS /* IBM: VM/CMS */
#define READBIN         "rb" /* VMCMS must use binary mode */
#define PATHSEP         ' '
#define DIRSEP          ' '
#else
#ifdef MVSXA /* IBM: MVS/XA */
#define READBIN         "rb" /* MVSXA must use binary mode */
#define PATHSEP         ':'
#define DIRSEP          '.'
#else
#define READBIN		"r"	/* UNIX doesn't care */
#define PATHSEP         ':'
#define DIRSEP          '/'
#endif  /* IBM: VM/CMS */
#endif
#endif
#endif
#endif

extern void error() ;

/* paths are all in the Makefile; by not supplying defaults, we force
   the installer to set them up. */
