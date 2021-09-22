/*
 *   The search routine takes a directory list, separated by PATHSEP, and
 *   tries to open a file.  Null directory components indicate current
 *   directory.  In an environment variable, null directory components
 *   indicate substitution of the default path list at that point.
 */
#include "dvips.h" /* The copyright notice in that file is included too! */
#ifdef KPATHSEA
#include <kpathsea/c-ctype.h>
#include <kpathsea/tex-file.h>
#include <kpathsea/tex-glyph.h>
extern char name[];
#else
#include <ctype.h>
#if !defined(WIN32)
extern int fclose();         /* these added to keep SunOS happy */
extern int pclose();
extern char *getenv();
#endif
#ifdef OS2
#include <stdlib.h>
FILE *fat_fopen();
#endif

extern char *newstring P1H(char *) ;

#if defined(SYSV) || defined(VMS) || defined(__THINK__) || defined(MSDOS) || defined(OS2) || defined(ATARIST) || defined(WIN32)
#define MAXPATHLEN (2000)
#else
#include <sys/param.h>          /* for MAXPATHLEN */
#endif
#if !defined(MSDOS) && !defined(OS2) && !defined(WIN32)
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
#endif /* KPATHSEA */
/*
 *
 *   We hope MAXPATHLEN is enough -- only rudimentary checking is done!
 */

#ifdef SECURE
extern Boolean secure;
#endif
#ifdef DEBUG
extern integer debug_flag;
#endif  /* DEBUG */
extern char *mfmode ;
extern int actualdpi ;
int to_close ;

#if defined(KPATHSEA) && !defined(UNCOMPRESS)
#define UNCOMPRESS "gzip -d"
#endif

#ifdef KPATHSEA
char *realnameoffile ;

FILE *
search P3C(kpse_file_format_type, format, char *, file, char *, mode)
{
  FILE *ret;
  string found_name;

#ifdef SECURE
  /* This change suggested by maj@cl.cam.ac.uk to disallow reading of
     arbitrary files.  */
  if (secure && kpse_absolute_p (file)) return NULL;
#endif

  /* Most file looked for through here must exist -- the exception is
     VF's. Bitmap fonts go through pksearch. */
  found_name = kpse_find_file (file, format, format != vfpath);

  if (found_name) {
    unsigned len = strlen (found_name);
#ifndef AMIGA
    if ((format == figpath || format == headerpath)
        && ((len > 2 && FILESTRCASEEQ (found_name + len - 2, ".Z"))
            || (len > 3 && FILESTRCASEEQ (found_name + len - 3, ".gz")))) {
      char *cmd = concat3 (UNCOMPRESS, "<", found_name);
      ret = popen (cmd, mode);
      to_close = USE_PCLOSE ;
    } else {
#endif /* not AMIGA */
      ret = fopen (found_name, mode);
      to_close = USE_FCLOSE ;
#ifndef AMIGA
    }
#endif /* not AMIGA */
    if (!ret)
      FATAL_PERROR (found_name);
    /* Free result of previous search.  */
    if (realnameoffile)
      free (realnameoffile);
    /* Save in `name' and `realnameoffile' because other routines
       access those globals.  Sigh.  */
    realnameoffile = found_name;
    strcpy(name, realnameoffile);
  } else
    ret = NULL;

  return ret;
}               /* end search */

FILE *
pksearch P6C(char *, path, char *, file, char *, mode,
	     halfword, dpi, char **, name_ret, int *, dpi_ret)
{
  FILE *ret;
  kpse_glyph_file_type font_file;
  string found_name = kpse_find_pk (file, dpi, &font_file);
  
  if (found_name)
    {
      ret = fopen (found_name, mode);
      if (!ret)
        FATAL_PERROR (name);

      /* Free result of previous search.  */
      if (realnameoffile)
	free (realnameoffile);
      /* Save in `name' and `realnameoffile' because other routines
	 access those globals.  Sigh.  */
      realnameoffile = found_name;
      strcpy(name, realnameoffile);
      *name_ret = font_file.name;
      *dpi_ret = font_file.dpi;
    }
  else
    ret = NULL;

  return ret;
}               /* end search */
#else /* ! KPATHSEA */
char realnameoffile[MAXPATHLEN] ;

extern char *figpath, *pictpath, *headerpath ;
FILE *
search P3C(char *, path, char *, file, char *, mode)
{
   register char *nam ;                 /* index into fname */
   register FILE *fd ;                  /* file desc of file */
   char fname[MAXPATHLEN] ;             /* to store file name */
   static char *home = 0 ;              /* home is where the heart is */
   int len = strlen(file) ;
   int tryz = 0 ;
   if (len>=3 &&
        ((file[len-2] == '.' && file[len-1] == 'Z') ||
         (file[len-3] == '.' && file[len-2] == 'g' && file[len-1] == 'z')) &&
        (path==figpath || path==pictpath || path==headerpath))
      tryz = file[len-1] ;
   to_close = USE_FCLOSE ;
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
   if (*file == DIRSEP 
       || NAME_BEGINS_WITH_DEVICE(file)) {               /* if full path name */
      if ((fd=fopen(file,mode)) != NULL) {
         strcpy(realnameoffile, file) ;
         if (tryz) {
            char *cmd = mymalloc(strlen(file) + 20) ;
            strcpy(cmd, (tryz=='z' ? "gzip -d <" : "compress -d <")) ;
            strcat(cmd, file) ;
            fclose(fd) ;
            fd = popen(cmd, "r") ;
            to_close = USE_PCLOSE ;
            free(cmd) ;
         }
         return(fd) ;
      } else
         return(NULL) ;
   }
#endif   /* IBM: VM/CMS */

#if defined MSDOS || defined OS2 || defined(ATARIST) || defined(WIN32)
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
               if (0 != (home = getenv("home")))
                  home = newstring(home) ;
               else
                  home = "." ;
            }
            strcpy(fname, home) ;
         } else {
#if defined MSDOS || defined OS2
            error("! ~username in path???") ;
#else
#ifdef WIN32
	    /* FIXME: at least under NT, it should be possible to 
	       retrieve the HOME DIR for a given user */
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
         if (tryz) {
            char *cmd = mymalloc(strlen(file) + 20) ;
            strcpy(cmd, (tryz=='z' ? "gzip -d <" : "compress -d <")) ;
            strcat(cmd, file) ;
            fclose(fd) ;
            fd = popen(cmd, "r") ;
            to_close = USE_PCLOSE ;
         }
         return(fd);
      }

   /* skip over PATHSEP and try again */
   } while (*(path++));

   return(NULL);

}               /* end search */

FILE *
pksearch P6C(char *, path, char *, file, char *, mode,
	     char *, n, halfword, dpi, halfword, vdpi)
{
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
#if defined MSDOS || defined OS2 || defined(WIN32)
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
               if (0 != (home = getenv("home")))
                  home = newstring(home) ;
               else
                  home = "." ;
            }
            strcpy(fname, home) ;
         } else {
#if defined MSDOS || defined OS2
            error("! ~username in path???") ;
#else
#ifdef WIN32
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
               default:  fprintf(stderr, "Format character: %c\n", *path) ;
                         error("! bad format character in pk path") ;
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
#endif /* KPATHSEA */

/* do we report file openings? */

#ifdef DEBUG
#  ifdef fopen
#    undef fopen
#  endif
#  ifdef VMCMS  /* IBM: VM/CMS */
#    define fopen cmsfopen
#  endif /* IBM: VM/CMS */
FILE *my_real_fopen P2C(register char *, n, register char *, t)
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
FILE *fat_fopen P2C(char *, fname, char *, t)
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

int close_file P1C(FILE *, f)
{
   switch(to_close) {
case USE_PCLOSE:  return pclose(f) ;
case USE_FCLOSE:  return fclose(f) ;
default:          return -1 ;
   }
}
