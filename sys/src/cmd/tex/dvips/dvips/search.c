/*
 *   The search routine takes a directory list, separated by PATHSEP, and
 *   tries to open a file.  Null directory components indicate current
 *   directory. if the file SUBDIR exists and the file is a font file,
 *   it checks for the file in a subdirectory named the same as the font name.
 *   Returns the open file descriptor if ok, else NULL.
 */
#include "structures.h" /* The copyright notice in that file is included too! */
#include <ctype.h>
#ifdef SYSV
#define MAXPATHLEN (256)
#else
#ifdef VMS
#define MAXPATHLEN (256)
#else
#ifdef __THINK__
#define MAXPATHLEN (256)
#else   /* ~SYSV */
#include <sys/param.h>          /* for MAXPATHLEN */
#endif  /* ~SYSV */
#endif
#endif
#ifndef MSDOS
#ifndef VMS
#ifndef VMCMS /* IBM: VM/CMS */
#ifndef __THINK__
#include <pwd.h>
#endif
#endif  /* IBM: VM/CMS */
#endif
#endif
/*
 *
 *   We hope MAXPATHLEN is enough -- only rudimentary checking is done!
 */

#ifdef DEBUG
extern integer debug_flag;
#endif  /* DEBUG */
extern char *mfmode ;
extern int actualdpi ;
char realnameoffile[MAXPATHLEN] ;
FILE *
search(path, file, mode)
        char *path, *file, *mode ;
{
#ifdef SHORTFNAMES
   char *nn;
   int ni;
#endif
   extern char *getenv(), *newstring() ;
   register char *nam ;                 /* index into fname */
   register FILE *fd ;                  /* file desc of file */
   char fname[MAXPATHLEN] ;             /* to store file name */
   static char *home = 0 ;              /* home is where the heart is */
#ifdef VMCMS /* IBM: VM/CMS - we don't have paths or dirsep's but we strip off
                             filename if there is a Unix path dirsep    */
   register char *lastdirsep ;
   lastdirsep = strrchr(file, '/') ;
   if ( NULL != lastdirsep ) file = lastdirsep + 1 ;
   if ((fd=fopen(file,mode)) != NULL) {
      return(fd) ;
   } else {
      return(NULL) ;
   }
#else
   if (*file == DIRSEP) {               /* if full path name */
      if ((fd=fopen(file,mode)) != NULL) {
         strcpy(realnameoffile, file) ;
         return(fd) ;
      } else
         return(NULL) ;
   }
#endif   /* IBM: VM/CMS */

#ifdef MSDOS
   if ( isalpha(file[0]) && file[1]==':' ) {   /* if full path name */
      if ((fd=fopen(file,mode)) != NULL)
         return(fd) ;
      else
         return(NULL) ;
   }
#endif

   do {
      /* copy the current directory into fname */
      nam = fname;
      /* copy till PATHSEP */
      if (*path == '~') {
         char *p = nam ;
         path++ ;
         while (*path && *path != PATHSEP && *path != DIRSEP)
            *p++ = *path++ ;
         *p = 0 ;
         if (*nam == 0) {
            if (home == 0) {
#ifdef PLAN9
               if (home = getenv("home"))
#else
               if (home = getenv("HOME"))
#endif
                  home = newstring(home) ;
               else
                  home = "." ;
            }
            strcpy(fname, home) ;
         } else {
#ifdef MSDOS
            error("! ~username in path???") ;
#else
#ifdef VMS
            error("! ~username in path???") ;
#else
#ifdef VMCMS  /* IBM: VM/CMS */
            error("! ~username in path???") ;
#else
#ifdef __THINK__
            error("! ~username in path???") ;
#else
            struct passwd *pw = getpwnam(fname) ;
            if (pw)
               strcpy(fname, pw->pw_dir) ;
            else
               error("no such user") ;
#endif
#endif  /* IBM: VM/CMS */
#endif
#endif
         }
         nam = fname + strlen(fname) ;
      }
      while (*path != PATHSEP && *path) *nam++ = *path++;
      *nam = 0 ;
#ifndef VMS
#ifndef __THINK__
      if (nam == fname) *nam++ = '.';   /* null component is current dir */

      if (*file != '\0') {
         *nam++ = DIRSEP;                  /* add separator */
#ifdef SHORTFNAMES
	nn = strrchr(file, DIRSEP);
	ni = (nn) ? 15+(nn-file) : 14;
	strncpy(nam, file, ni);
	nam[ni] = '\0';
#else
         (void)strcpy(nam,file);                   /* tack the file on */
#endif
      }
      else
         *nam = '\0' ;
#else
      (void)strcpy(nam,file);                   /* tack the file on */
#endif
#else
#ifdef SHORTFNAMES
	nn = strrchr(file, DIRSEP);
	ni = (nn) ? 15+(nn-file) : 14;
	strncpy(nam, file, ni);
	nam[ni] = '\0';
#else
      (void)strcpy(nam,file);                   /* tack the file on */
#endif
#endif
      /* belated check -- bah! */
      if ((nam - fname) + strlen(file) + 1 > MAXPATHLEN)
         error("! overran allocated storage in search()");

#ifdef DEBUG
      if (dd(D_PATHS))
         (void)fprintf(stderr,"Trying to open %s\n", fname) ;
#endif
      if ((fd=fopen(fname,mode)) != NULL) {
         strcpy(realnameoffile, fname) ;
         return(fd);
      }

   /* skip over PATHSEP and try again */
   } while (*(path++));

   return(NULL);

}               /* end search */

FILE *
pksearch(path, file, mode, n, dpi, vdpi)
        char *path, *file, *mode ;
	char *n ;
	halfword dpi, vdpi ;
{
#ifdef SHORTFNAMES
   char *nn;
   int ni;
#endif
   extern char *getenv(), *newstring() ;
   register char *nam ;                 /* index into fname */
   register FILE *fd ;                  /* file desc of file */
   char fname[MAXPATHLEN] ;             /* to store file name */
   static char *home = 0 ;              /* home is where the heart is */
   int sub ;

   if (*file == DIRSEP) {               /* if full path name */
      if ((fd=fopen(file,mode)) != NULL)
         return(fd) ;
      else
         return(NULL) ;
   }
#ifdef MSDOS
   if ( isalpha(file[0]) && file[1]==':' ) {  /* if full path name */
      if ((fd=fopen(file,mode)) != NULL)
         return(fd) ;
      else
         return(NULL) ;
   }
#endif
   do {
      /* copy the current directory into fname */
      nam = fname;
      sub = 0 ;
      /* copy till PATHSEP */
      if (*path == '~') {
         char *p = nam ;
         path++ ;
         while (*path && *path != PATHSEP && *path != DIRSEP)
            *p++ = *path++ ;
         *p = 0 ;
         if (*nam == 0) {
            if (home == 0) {
               if (home = getenv("HOME"))
                  home = newstring(home) ;
               else
                  home = "." ;
            }
            strcpy(fname, home) ;
         } else {
#ifdef MSDOS
            error("! ~username in path???") ;
#else
#ifdef VMS
            error("! ~username in path???") ;
#else
#ifdef VMCMS  /* IBM: VM/CMS */
            error("! ~username in path???") ;
#else
#ifdef __THINK__
            error("! ~username in path???") ;
#else
            struct passwd *pw = getpwnam(fname) ;
            if (pw)
               strcpy(fname, pw->pw_dir) ;
            else
               error("no such user") ;
#endif
#endif /* IBM: VM/CMS */
#endif
#endif
         }
         nam = fname + strlen(fname) ;
      }
      /* copy till PATHSEP */
      while (*path != PATHSEP && *path) {
         if (*path == '%') {
            sub = 1 ;
            path++ ;
            switch(*path) {
               case 'b': sprintf(nam, "%d", actualdpi) ; break ;
               case 'd': sprintf(nam, "%d", dpi) ; break ;
               case 'f': strcpy(nam, n) ; break ;
               case 'm': if (mfmode == 0)
                            if (actualdpi == 300) mfmode = "imagen" ;
                            else if (actualdpi == 400) mfmode = "nexthi" ;
                            else if (actualdpi == 635) mfmode = "linolo" ;
                            else if (actualdpi == 1270) mfmode = "linohi" ;
                            else if (actualdpi == 2540) mfmode = "linosuper" ;
                         if (mfmode == 0)
                            error("! MF mode not set, but used in pk path") ;
                         strcpy(nam, mfmode) ;
                         break ;
               case 'p': strcpy(nam, "pk") ; break ;
               case '%': strcpy(nam, "%") ; break ;
               default: error("! bad format character in pk path") ;
            }
            nam = fname + strlen(fname) ;
            if (*path)
               path++ ;
         } else
            *nam++ = *path++;
      }
#ifndef VMS
#ifndef __THINK__
      if (nam == fname) *nam++ = '.';   /* null component is current dir */
#endif
#endif /* VMS */
      if (sub == 0 && *file) {
         *nam++ = DIRSEP ;
#ifdef SHORTFNAMES
	 nn = strrchr(file, DIRSEP);
	 ni = (nn) ? 15+(nn-file) : 14;
	 strncpy(nam, file, ni);
	 nam[ni] = '\0';
#else
         strcpy(nam, file) ;
#endif
      } else
         *nam = 0 ;

      /* belated check -- bah! */
      if (strlen(fname) + 1 > MAXPATHLEN)
         error("! overran allocated storage in search()");

#ifdef DEBUG
      if (dd(D_PATHS))
         (void)fprintf(stderr,"Trying to open %s\n", fname) ;
#endif
      if ((fd=fopen(fname,mode)) != NULL)
         return(fd);

   /* skip over PATHSEP and try again */
   } while (*(path++));

   return(NULL);

}               /* end search */

/* do we report file openings? */

#ifdef DEBUG
#  ifdef fopen
#    undef fopen
#  endif
#  ifdef VMCMS  /* IBM: VM/CMS */
#    define fopen cmsfopen
#  endif /* IBM: VM/CMS */
FILE *my_real_fopen(n, t)
register char *n, *t ;
{
   FILE *tf ;
   if (dd(D_FILES)) {
      fprintf(stderr, "<%s(%s)> ", n, t) ;
      tf = fopen(n, t) ;
      if (tf == 0)
         fprintf(stderr, "failed\n") ;
      else
         fprintf(stderr, "succeeded\n") ;
   } else
      tf = fopen(n, t) ;
   return tf ;
}
#endif
