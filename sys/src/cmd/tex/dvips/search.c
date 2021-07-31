/*
 *   The search routine takes a directory list, separated by PATHSEP, and
 *   tries to open a file.  Null directory components indicate current
 *   directory.  In an environment variable, null directory components
 *   indicate substitution of the default path list at that point.
 */
#include "dvips.h" /* The copyright notice in that file is included too! */
#include <ctype.h>
#ifdef OS2
#include <stdlib.h>
FILE *fat_fopen();
#endif

#if defined(SYSV) || defined(VMS) || defined(__THINK__) || defined(MSDOS) || defined(OS2) || defined(ATARIST)
#define MAXPATHLEN (256)
#else
#include <sys/param.h>          /* for MAXPATHLEN */
#endif
#if !defined(MSDOS) && !defined(OS2)
#ifndef VMS
#ifndef MVSXA
#ifndef VMCMS /* IBM: VM/CMS */
#ifndef __THINK__
#ifndef ATARIST
#include <pwd.h>
#endif
#endif
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
   extern char *getenv(), *newstring() ;
   register char *nam ;                 /* index into fname */
   register FILE *fd ;                  /* file desc of file */
   char fname[MAXPATHLEN] ;             /* to store file name */
   static char *home = 0 ;              /* home is where the heart is */
#ifdef MVSXA
char fname_safe[256];
register int i, firstext, lastext, lastchar;
#endif
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

#if defined MSDOS || defined OS2 || defined(ATARIST)
   if ( isalpha(file[0]) && file[1]==':' ) {   /* if full path name */
      if ((fd=fopen(file,mode)) != NULL) {
         strcpy(realnameoffile, file) ;
         return(fd) ;
      } else
         return(NULL) ;
   }
   if (*file == '/') {/* if full path name with unix DIRSEP less drive code */
      if ((fd=fopen(file,mode)) != NULL) {
         strcpy(realnameoffile, file) ;
         return(fd) ;
      } else
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
               if (0 != (home = getenv("home")))
#else
               if (0 != (home = getenv("HOME")))
#endif
                  home = newstring(home) ;
               else
                  home = "." ;
            }
            strcpy(fname, home) ;
         } else {
#if defined MSDOS || defined OS2
            error("! ~username in path???") ;
#else
#ifdef VMS
            error("! ~username in path???") ;
#else
#ifdef ATARIST
            error("! ~username in path???") ;
#else
#ifdef VMCMS  /* IBM: VM/CMS */
            error("! ~username in path???") ;
#else
#ifdef MVSXA  /* IBM: MVS/XA */
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
         if ((nam != fname) && *(nam-1) != DIRSEP) /* GNW 1992.07.09 */
            *nam++ = DIRSEP;                  /* add separator */
         (void)strcpy(nam,file);                   /* tack the file on */
      }
      else
         *nam = '\0' ;
#else
      (void)strcpy(nam,file);                   /* tack the file on */
#endif
#else
      (void)strcpy(nam,file);                   /* tack the file on */
#endif
#ifdef MVSXA
nam = fname;
if (strchr(nam,'=') != NULL) {
   (void) strcpy(fname_safe,fname);  /* save fname */
   firstext = strchr(nam, '=') - nam + 2;
   lastext = strrchr(nam, '.') - nam + 1;
   lastchar  = strlen(nam) - 1;

   (void) strcpy(fname,"dd:");  /* initialize fname */
   nam=&fname[3];
   for (i=lastext; i<=lastchar; i++) *nam++ = fname_safe[i] ;
           *nam++  = '(' ;
   for (i=firstext; i<lastext-1; i++) *nam++ = fname_safe[i] ;
           *nam++  = ')' ;
           *nam++  = 0   ;
   }
   else {
      if (fname[0] == '/') {
         fname[0] = '\'';
         strcat(&fname[strlen(fname)],"\'");
      }
      if (fname[0] == '.') fname[0] = ' ';
      if (fname[1] == '.') fname[1] = ' ';
   }
#endif

      /* belated check -- bah! */
      if ((nam - fname) + strlen(file) + 1 > MAXPATHLEN)
         error("! overran allocated storage in search()");

#ifdef DEBUG
      if (dd(D_PATHS))
         (void)fprintf(stderr,"search: Trying to open %s\n", fname) ;
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
#if defined MSDOS || defined OS2
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
               if (0 != (home = getenv("HOME")))
                  home = newstring(home) ;
               else
                  home = "." ;
            }
            strcpy(fname, home) ;
         } else {
#if defined MSDOS || defined OS2
            error("! ~username in path???") ;
#else
#ifdef VMS
            error("! ~username in path???") ;
#else
#ifdef ATARIST
            error("! ~username in path???") ;
#else
#ifdef VMCMS  /* IBM: VM/CMS */
            error("! ~username in path???") ;
#else
#ifdef MVSXA  /* IBM: MVS/XA */
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
#ifndef VMS
         /* change suggested by MG */
         if ((nam != fname) && *(nam-1) != DIRSEP) /* GNW 1992.07.09 */
            *nam++ = DIRSEP ;
#endif
         strcpy(nam, file) ;
      } else
         *nam = 0 ;

#ifdef MVSXA   /* IBM: MVS/XA */
      if (fname[0] == '/') {
         fname[0] = '\'';
         strcat(&fname[strlen(fname)],"\'");
      }
      if (fname[0] == '.') fname[0] = ' ';
      if (fname[1] == '.') fname[1] = ' ';
#endif         /* IBM: MVS/XA */
      /* belated check -- bah! */
      if (strlen(fname) + 1 > MAXPATHLEN)
         error("! overran allocated storage in search()");

#ifdef DEBUG
      if (dd(D_PATHS))
         (void)fprintf(stderr,"pksearch: Trying to open %s\n", fname) ;
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
#ifdef OS2
   if (tf == (FILE *)NULL)
     tf = fat_fopen(n, t); /* try again with filename truncated to 8.3 */
#endif
   return tf ;
}
#endif

#ifdef OS2
/* truncate filename at end of fname to FAT filesystem 8.3 limit */
/* if truncated, return fopen() with new name */
FILE *fat_fopen(fname, t)
char *fname, *t;
{
   char *np;	/* pointer to name within path */
   char nbuf[13], *ns, *nd;
   char n[MAXPATHLEN];
   int ni, ne;
   FILE *tf;
   strcpy(n, fname);
   for (ns=n; *ns; ns++) {
      if (*ns=='/')
         *ns=DIRSEP;
   }
   np = strrchr(n,DIRSEP);
   if (np==(char *)NULL)
      np = n;
   else
      np++;
   /* fail if it contains more than one '.' */
   ni = 0;
   for (ns=np; *ns; ns++) {
      if (*ns=='.')
         ni++;
   }
   if (ni>1)
      return (FILE *)NULL;
   /* copy it to nbuf, truncating to 8.3 */
   ns = np;
   nd = nbuf;
   ni = 0;
   while ((*ns!='.') && (*ns) && (ni<8)) {
      *nd++ = *ns++;
      ni++;
   }
   while ((*ns!='.') && (*ns)) {
      ns++;
      ni++;
   }
   ne = 0;
   if (*ns=='.') {
      *nd++ = *ns++;
      while ((*ns!='.') && (*ns) && (ne<3)) {
         *nd++ = *ns++;
         ne++;
      }
      while (*ns) {
         ns++;
         ne++;
      }
   }
   *nd++='\0';
   if ((ni>8) || (ne>3)) {
      strcpy(np,nbuf);
      /* now code copied from my_real_fopen() */
      if (dd(D_FILES)) {
         fprintf(stderr, "<%s(%s)> ", n, t) ;
         tf = fopen(n, t) ;
         if (tf == 0)
            fprintf(stderr, "failed\n") ;
         else
            fprintf(stderr, "succeeded\n") ;
      }
      else
         tf = fopen(n, t) ;
      return tf;
   }
   return (FILE *)NULL;
}
#endif
