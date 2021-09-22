/* makempx.c: this program emulates the makempx shell script from Metapost.

Copyright (C) 1997 Fabrice POPINEAU.

Adapted to DJGPP by Eli Zaretskii.

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Library General Public
License as published by the Free Software Foundation; either
version 2 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Library General Public License for more details.

You should have received a copy of the GNU Library General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */

/* #!/bin/sh */
/* # '$Id: makempx.c,v 1.4 1998/03/10 21:23:30 olaf Exp $' */
/* # Make an MPX file from the labels in a MetaPost source file, */
/* # using mpto and either dvitomp (TeX) or dmp (troff). */
/* # From John Hobby's original (though there's not much of it left by now). */
/* # Public domain. */

/* rcs_revision='$Revision: 1.4 $' */
/* version=`set - $rcs_revision; echo $2` */

/* : ${DMP=dmp} */
/* : ${DVITOMP=dvitomp} */
/* : ${MAKEMPX_BINDIR="j:\\TeX\\bin"} */
/* : ${MPTEXPRE=mptexpre.tex} */
/* : ${MPTOTEX="mpto -tex"} */
/* : ${MPTOTR="mpto -troff"} */
/* : ${NEWER=newer} */
/* : ${TEX=tex} */
/* : ${TROFF='eqn -d\$\$ | troff -Tpost'} */

/* PATH=$MAKEMPX_BINDIR:/bin:/usr/bin:$PATH */

/* # These names are documented in the MetaPost manual, so it's */
/* # unwise to change them. */
/* ERRLOG=mpxerr.log               # file for an error log if necessary */
/* TEXERR=mpxerr.tex               # file for erroneous TeX if any */
/* DVIERR=mpxerr.dvi               # troublesome dvi file if any */
/* TROFF_INERR=mpxerr              # file for erroneous troff input, if any */
/* TROFF_OUTERR=mpxerr.t           # file for troublesome troff output, if any */


/* usage="Usage: $0 [-tex|-troff] MPFILE MPXFILE. */
/*   If MPXFILE is older than MPFILE, translate the labels from the MetaPost */
/*   input file MPFIle to low-level commands in MPXFILE, by running */
/*     $MPTOTEX, $TEX, and $DVITOMP */
/*   by default; or, if -troff is specified, */
/*     $MPTOTR, $TROFF, and $DMP. */

/*   The current directory is used for writing temporary files.  Errors are */
/*   left in mpxerr.{tex,log,dvi}. */

/*   If the file named in \$MPTEXPRE (mptexpre.tex by default) exists, it is */
/*   prepended to the output in tex mode. */

/* Email bug reports to tex-k@mail.tug.org." */


/* mode=tex */

/* while test $# -gt 0; do */
/*   case "$1" in */
/*     -help|--help)  */
/*       echo "$usage"; exit 0;; */
/*     -version|--version) */
/*       echo "`basename $0` (Web2c 6.98) $version" */
/*       echo "There is NO warranty.  This script is public domain. */
/* Primary author: John Hobby; Web2c maintainer: K. Berry." */
/*       exit 0;; */
/*     -troff|--troff) mode=troff;; */
/*     -tex|--tex) mode=tex;; */
/*     -*)  */
/*       echo "$0: Invalid option: $1." >&2 */
/*       echo "Try \``basename $0` --help' for more information." >&2  */
/*       exit 1;; */
/*     *)  */
/*       if test -z "$MPFILE"; then */
/*         MPFILE=$1                   # input file */
/*       elif test -z "$MPXFILE"; then */
/*         MPXFILE=$1                  # output file */
/*       else */
/*         echo "$0: Extra argument $1." >&2  */
/*         echo "Try \``basename $0` --help' for more information." >&2 */
/*         exit 1 */
/*       fi;; */
/*   esac */
/*   shift */
/* done */

/* if test -z "$MPFILE" || test -z "$MPXFILE"; then */
/*   echo "$0: Need exactly two file arguments." >&2 */
/*   echo "Try \``basename $0` --help' for more information." >&2 */
/*   exit 1 */
/* fi */

/* trap "rm -f mpx$$.* $ERRLOG; exit 4" 1 2 3 15 */

/* # If MPX file is up-to-date, do nothing. */
/* if $NEWER $MPFILE $MPXFILE; then */

/*   # Have to remake. */
/*   # Step 0: Check typesetter mode for consistency. */
/*   case "$mode" in */
/*       tex) MPTO="$MPTOTEX";; */
/*     troff) MPTO="$MPTOTR";; */
/*         *) echo "$0: Unknown typesetter mode: $mode" >&2 */
/*            exit 1;; */
/*   esac */
  
/*   # Step 1: Extract typesetter source from MetaPost source. */
/*   if $MPTO $MPFILE >mpx$$.tex 2>$ERRLOG; then :; */
/*     # success */
/*   else */
/*     # failure */
/*     echo "$0: Command failed: $MPTO $MPFILE" >&2 */
/*     cat $ERRLOG >&2 */
/*     rm -f mpx$$.tex */
/*     exit 1 */
/*   fi */
/*   if test "$mode" = troff; then */
/*     mv -f mpx$$.tex mpx$$.i */
/*   fi */
  
/*   # Step 2: Run typesetter. */
/*   if test "$mode" = tex; then */
/*     if test -r $MPTEXPRE; then */
/*       # Prepend user file. */
/*       cat $MPTEXPRE mpx$$.tex >mpx$$.tmp */
/*       mv mpx$$.tmp mpx$$.tex */
/*     fi */

/*     if $TEX '\batchmode\input 'mpx$$.tex </dev/null >/dev/null; then */
/*       WHATEVER_TO_MPX="$DVITOMP" */
/*       INFILE=mpx$$.dvi */
/*       INERROR=$DVIERR */
/*     else */
/*       # failure */
/*       mv -f mpx$$.tex $TEXERR */
/*       mv -f mpx$$.log $ERRLOG */
/*       echo "$0: Command failed: $TEX $TEXERR; see $ERRLOG" >&2 */
/*       exit 2 */
/*     fi */
/*   elif test "$mode" = troff; then */
/*     if cat mpx$$.i | eval $TROFF >mpx$$.t; then */
/*       # success, prepare for step 3. */
/*       WHATEVER_TO_MPX="$DMP" */
/*       INFILE=mpx$$.t */
/*       INERROR=$TROFF_OUTERR */
/*     else */
/*       # failure */
/*       mv -f mpx$$.i $TROFF_INERR */
/*       echo "$0: Command failed: cat $TROFFERR | $TROFF" >&2 */
/*       rm -f mpx$$.t */
/*       exit 2 */
/*     fi */
/*   else */
/*     echo "$0: Unknown typesetter mode: $mode; how did this happen?" >&2 */
/*     exit 2 */
/*   fi */

/*   # Step 3: Translate typesetter output to a MetaPost MPX. */
/*   if $WHATEVER_TO_MPX $INFILE $MPXFILE >$ERRLOG; then */
/*     : # success */
/*   else  */
/*     # failure */
/*     mv -f $INFILE $INERROR */
/*     test $mode = troff && mv -f mpx$$.i $TROFF_INERR */
/*     echo "$0: Command failed: $WHATEVER_TO_MPX $INERROR $MPXFILE" >&2 */
/*     # Better to remove $MPXFILE if something went wrong rather than */
/*     # leaving behind an unfinished or unusable version since $NEWER */
/*     # might think that all is fine if $MPXFILE exists. */
/*     rm -f $MPXFILE */
/*     cat $ERRLOG >&2 */
/*     exit 3 */
/*   fi */

/*   rm -f $ERRLOG mpx$$.* */
/* fi */

/* exit 0 */

/*
  Global Variables
  */
/* #include <assert.h> */
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <string.h>
#include <signal.h>

#include <kpathsea/config.h>
#include <kpathsea/variable.h>
#include <kpathsea/c-dir.h>
#include <kpathsea/c-pathch.h>
#include <kpathsea/c-pathmx.h>
#include <kpathsea/proginit.h>
#include <kpathsea/line.h>
#include <kpathsea/concatn.h>

#include "stackenv.h"

static char *DMP;
static char *DVITOMP;
static char *MAKEMPX_BINDIR;
static char *MPTEXPRE;
static char *MPTOTEX;
static char *MPTOTR;
static char *NEWER;
static char *TEX;
static char *TROFF;

static struct alist {
  char *key;
  char **var;
  char *val;
} vardef[] = {
  {"DMP",            &DMP, "dmp"},
  {"DVITOMP",        &DVITOMP, "dvitomp"},
#ifdef _WIN32
  {"MAKEMPX_BINDIR", &MAKEMPX_BINDIR, "c:\\TeX\\bin"},
#else
#ifdef __DJGPP__
  {"MAKEMPX_BINDIR", &MAKEMPX_BINDIR, "c:/djgpp/bin"},
#else
  {"MAKEMPX_BINDIR", &MAKEMPX_BINDIR, "/usr/local/bin"},
#endif
#endif
  {"MPTEXPRE",       &MPTEXPRE, "mptexpre.tex"},
  {"MPTOTEX",        &MPTOTEX, "mpto -tex"},
  {"MPTOTR",         &MPTOTR, "mpto -troff"},
  {"NEWER",          &NEWER, "newer"},
  {"TEX",            &TEX, "tex"},
#ifdef _WIN32
  {"TROFF",          &TROFF, "eqn -d$$ | troff -Tpost"},
#else
#ifdef __DJGPP__
  /* DJGPP users are likely to have Groff whose PostScript device is `ps'. */
  {"TROFF",          &TROFF, "eqn -d'$$' | troff -Tps"},
#else
  {"TROFF",          &TROFF, "eqn -d'$$' | troff -Tpost"},
#endif
#endif
  {NULL, NULL, NULL}
};

static char *MPTO, *INFILE, *INERROR, *WHATEVER_TO_MPX;
static char mptemplate[FILENAME_MAX];

/* These names are documented in the MetaPost manual, so it's */
/* unwise to change them. */
char *ERRLOG="mpxerr.log";	/* file for an error log if necessary */
char *TEXERR="mpxerr.tex";      /* file for erroneous TeX if any */
char *DVIERR="mpxerr.dvi";      /* troublesome dvi file if any */
char *TROFF_INERR="mpxerr";	/* file for erroneous troff input, if any */
char *TROFF_OUTERR="mpxerr.t";; /* file for troublesome troff output, if any */
char *MPFILE = NULL;
char *MPXFILE = NULL;

char *progname;
char *version = "Revision: 1.7";
extern string mktex_version_string;

typedef enum { TEXMODE = 0, TROFFMODE} typeset_mode;

#ifdef __GNUC__
void __attribute__((noreturn)) usage(int exit_code)
#else
void usage(int exit_code)
#endif
{
  fprintf(stderr, "Usage: %s [-tex|-troff] MPFILE MPXFILE.\n\
  If MPXFILE is older than MPFILE, translate the labels from the MetaPost\n\
  input file MPFILE to low-level commands in MPXFILE, by running\n\
    %s, %s, and %s\n\
  by default; or, if -troff is specified,\n\
    %s, %s, and %s.\n\
\n\
  The current directory is used for writing temporary files.  Errors are\n\
  left in mpxerr.{tex,log,dvi}.\n\
\n\
  If the file named in $MPTEXPRE (mptexpre.tex by default) exists, it is\n\
  prepended to the output in tex mode.\n\
\n\
  Email bug reports to tex-k@mail.tug.org.\n",
	  progname, MPTOTEX, TEX, DVITOMP, MPTOTR, TROFF, DMP);
  mt_exit(exit_code);
}

void printversion(void)
{
  fprintf(stderr, "%s: (Web2c 7.0) %s\n", progname, version);
  fprintf(stderr, "%s: C replacement version %s\n", progname, mktex_version_string);
  fprintf(stderr, "There is NO warranty.  This program is public domain.\n\
Primary author: John Hobby; Web2c maintainer: K. Berry;\n\
C version: F. Popineau and Eli Zaretskii.");
  mt_exit(0);
}

void output_file(FILE *f, const_string name)
{
  FILE *fp;
  string line;

  if ((fp = fopen(name, "r")) == NULL) {
      perror(name);
  }
  else {
    while((line = read_line(fp)) != NULL) {
      fputs(line, f);
      free(line);
    }
    fclose(fp);
  }
}

void output_and_cleanup(int code)
{
#ifdef _WIN32
  WIN32_FIND_DATA ffd;
  HANDLE hnd;
  boolean go_on;
  string temp = concat(mptemplate, ".*");
  /* rm -f mpxXXXXX.* */
  hnd = FindFirstFile(temp, &ffd);
  go_on = (hnd != INVALID_HANDLE_VALUE); 
  while (go_on) {
    DeleteFile(ffd.cFileName);
    go_on = (FindNextFile(hnd, &ffd) == TRUE);
  }
  FindClose(hnd);
  free(temp);
#else
  DIR *dp;
  struct dirent *de;
  int temp_len = strlen(mptemplate) + 1; /* +1 for the dot */

  if ((dp = opendir(".")) != 0) {
    while ((de = readdir(dp)) != 0)
      if (FILESTRNCASEEQ(de->d_name, mptemplate, temp_len))
	remove(de->d_name);
  }
#endif

  /* ERRLOG is removed only on normal exit. */
  if (code == 0)
    remove(ERRLOG);
  
}

char *change_suffix(char *old, char *suf)
{
  char *p, *new;
  new = xstrdup(old);
  p = strrchr(new, '.');	/* assumes there are no slashes in OLD */
  if (p)
    strcpy(p, suf);
  else
    strcat(new, suf);
  return new;
}

/* This does what `newer' does, but avoids forking a child.  */
int newer(const char *first, const char *second)
{
  struct stat x, y;

  /* does the first file exist? */
  if (stat(first, &x) < 0)
    return 0;

  /* does the second file exist? */
  if (stat(second, &y) < 0)
    return 1;

  /* is the first file older than the second? */
  if (x.st_mtime < y.st_mtime)
    return 0;

  return 1;
}

int __cdecl main(int argc, char *argv[])
{
  struct alist *p;
  char buf[1024];
  char cmd[512];
  typeset_mode mode = TEXMODE;
  int i, newfd[3];
  FILE *ferr, *fmpx, *ftroff;
  char mpxtex[32];
  char *mpxi = NULL, *mpxt, *mpxtmp, *mpxlog, *mpxdvi;
  string path;
  string mpto_cmd;
  char errbuf[PATH_MAX*2];
  int mpto_status;

  if (!progname)
    progname = argv[0];
  kpse_set_progname (progname);

#ifdef WIN32
  SetConsoleCtrlHandler((PHANDLER_ROUTINE)mt_exit, TRUE);
#else
# ifdef SIGINT
  signal (SIGINT, sigint_handler);
# endif
# ifdef SIGQUIT
  signal (SIGQUIT, sigint_handler);
# endif
# ifdef SIGHUP
  signal (SIGHUP, sigint_handler);
# endif
# ifdef SIGTERM
  signal (SIGTERM, sigint_handler);
# endif
#endif

  /* First assigning default values to all variables. */
  for(p = vardef; p->key != NULL; p++) {
    /* testing if there is an env var */
    strcpy(buf, "$"); strcat(buf, p->key);
    *(p->var) = kpse_var_expand(buf);
    if (*(p->var) == NULL || **(p->var) == '\0')
      *(p->var) = p->val;
  }

  /* Modifying $PATH */

  /* The SELFAUTOLOC stuff is NOT in the original makempx script.
     However, it is a handy way to get the installation directory
     without modifying the source at compile time.

     Also note that if $PATH is undefined, the null pointer returned
     by `getenv' will serve as the trailing NULL to `concatn'.  */
  path = concatn(MAKEMPX_BINDIR, ENV_SEP_STRING,
		 kpse_var_value("SELFAUTOLOC"), ENV_SEP_STRING,
		 getenv("PATH"), NULL);
  xputenv("PATH", path);
  free(path);

  for (i = 1; i <argc; i++) {
    if (!strcmp(argv[i], "-help") || !strcmp(argv[i], "--help")
	|| !strcmp(argv[i], "-h")) {
      usage(0);
    }
    if (!strcmp(argv[i], "-version") || !strcmp(argv[i], "--version")
	|| !strcmp(argv[i], "-v")) {
      printversion();
    }
    if (!strcmp(argv[i], "-troff") || !strcmp(argv[i], "--troff")) {
      mode = TROFFMODE;
    }
    else if (!strcmp(argv[i], "-tex") || !strcmp(argv[i], "--tex")) {
      mode = TEXMODE;
    }
    /* FIXME: this disallows arguments with a leading dash, but
       that's how the original shell script works.  */
    else if (*argv[i] == '-') {
      fprintf(stderr, "%s: Invalid option: %s.\nTry `%s --help' for more information.\n",
	      progname, argv[i], program_invocation_short_name);
      mt_exit(1);
    }
    else if (!MPFILE || !*MPFILE)
      MPFILE = argv[i];
    else if (!MPXFILE || !*MPXFILE)
      MPXFILE = argv[i];
    else {
      fprintf(stderr, "%s: Extra argument %s.\nTry `%s --help' for more information.\n",
	      progname, argv[i], program_invocation_short_name);
      mt_exit(1);
    }
  }
  if (!MPXFILE || !*MPXFILE || !MPFILE || !*MPFILE) {
    fprintf(stderr, "%s:  Need exactly two file arguments.\nTry `%s --help' for more information.",
	    progname, program_invocation_short_name);
    mt_exit(1);
  }

  /* If MPX file is up-to-date, do nothing.  */
  if (!newer (MPFILE, MPXFILE))
    mt_exit(0);

  /* Have to remake.  */

  /* Step 0: Check typesetter mode for consistency.  */
  *cmd = '\0';
  switch(mode) {
  case TEXMODE:
    MPTO = MPTOTEX;
    break;
  case TROFFMODE:
    MPTO = MPTOTR;
    break;
  default:
    fprintf(stderr, "%s: Unknown typesetter mode\n", progname);
    mt_exit(1);
  }

  /* Step 1: Extract typesetter source from MetaPost source.  */

  sprintf(mptemplate, "mpx%d", getpid());
  strcpy(mpxtex, mptemplate); strcat(mpxtex, ".tex");
  /* redirecting 1 -> mpx$$.tex 2 -> ERRLOG */
  if (!(fmpx = fopen(mpxtex, "w"))) {
    perror(mpxtex);
    mt_exit(1);
  }
  if (!(ferr = fopen(ERRLOG, "w"))) {
    perror(ERRLOG);
    mt_exit(1);
  }

  newfd[0] = 0; newfd[1] = fileno(fmpx); newfd[2] = fileno(ferr);
  push_fd(newfd);

  mpto_cmd = concat3(MPTO, " ", MPFILE);
  mpto_status = system(mpto_cmd);
  free(mpto_cmd);
  pop_fd();
  fclose(fmpx);
  fclose(ferr);
  if (mpto_status != 0) {
    fprintf(stderr, "%s: Command failed: %s %s\n", progname, MPTO, MPFILE);

    output_file(stderr, ERRLOG);
    /* rm -f mpx$$.tex -- this is done by the wrap-up code */
    mt_exit(1);
  }

  /* Step 2: Run typesetter.  */

  switch(mode) {
  case TEXMODE:
    mpxdvi = change_suffix(mpxtex, ".dvi");
    mpxtmp = change_suffix(mpxtex, ".tmp");
    mpxlog = change_suffix(mpxtex, ".log");
    
    if (test_file('r', MPTEXPRE)) {
      if (catfile(MPTEXPRE, mpxtmp, 0) == FALSE
	  || catfile(mpxtex, mpxtmp, 1) == FALSE) {
	sprintf(errbuf, "%s: cat %s %s > %s",
		program_invocation_short_name, MPTEXPRE, mpxtex, mpxtmp);
	perror(errbuf);
	mt_exit(1);
      }
      mvfile(mpxtmp, mpxtex);
    }
#ifdef _WIN32
    sprintf(cmd, "%s --interaction=batchmode %s < nul > nul", TEX, mpxtex);
#else
    sprintf(cmd, "%s --interaction=batchmode %s </dev/null >/dev/null",
	    TEX, mpxtex);
#endif
    if (system(cmd)) {
      fprintf(stderr, "%s: Command failed: %s %s; see %s\n",
	      progname, TEX, TEXERR, ERRLOG);
      mvfile(mpxtex, TEXERR);
      mvfile(mpxlog, ERRLOG);
      mt_exit(2);
    }
    WHATEVER_TO_MPX = DVITOMP;
    INFILE = mpxdvi;
    INERROR = DVIERR;
    break;
  case TROFFMODE:
    mpxi = change_suffix(mpxtex, ".i");
    mpxt = change_suffix(mpxtex, ".t");
    mvfile(mpxtex, mpxi);
    sprintf(cmd, "%s > %s", TROFF, mpxt);
    if ((ftroff = popen(cmd, "w")) == NULL) {
      perror(cmd);
      mt_exit(1);
    }
    if (!(fmpx = fopen(mpxi, "r"))) {
      perror(mpxi);
      mt_exit(1);
    }
    while (fgets(buf, 1024, fmpx))
      fputs(buf, ftroff);
    if (pclose(ftroff) != 0) {
      fprintf(stderr, "%s: Command failed: cat %s | %s\n", progname,
	      TROFF_INERR, TROFF);
      mvfile(mpxi, TROFF_INERR);
      /* rm -f mpx$$.t : this is done in wrap-up code */
      mt_exit(2);
    }
    fclose(fmpx);
    WHATEVER_TO_MPX = DMP;
    INFILE = mpxt;
    INERROR = TROFF_OUTERR;
    break;
  default:
    fprintf(stderr, "%s: Unknown typesetter mode; how did this happen?\n",
	    progname);
    mt_exit(2);
  }

  /* Step 3: Translate typesetter output to a MetaPost MPX.  */

  sprintf(cmd, "%s %s %s > %s", WHATEVER_TO_MPX, INFILE, MPXFILE, ERRLOG);
  if (system(cmd)) {
    fprintf(stderr, "%s: Command failed: %s %s %s\n", progname,
	    WHATEVER_TO_MPX, INERROR, MPXFILE);
    mvfile(INFILE, INERROR);
    if (mode == TROFFMODE)
      mvfile(mpxi, TROFF_INERR);
    /* rm -f $MPXFILE : done in our exit code */
    output_file(stderr, ERRLOG);
    mt_exit(3);
  }
  mt_exit(0);
}
