/*   For use with emTeX set FONTPATH to "TEXTFM"
 */
#ifndef FONTPATH
#define FONTPATH "TEXFONTS"
#endif

/*
 *   This code reads in and handles the defaults for the program from the
 *   file config.sw.  This entire file is a bit kludgy, sorry.
 */
#include "dvips.h" /* The copyright notice in that file is included too! */
#include "paths.h"
/*
 *   This is the structure definition for resident fonts.  We use
 *   a small and simple hash table to handle these.  We don't need
 *   a big hash table.
 */
struct resfont *reshash[RESHASHPRIME] ;
/*
 *   These are the external routines we use.
 */
extern void error() ;
extern integer scalewidth() ;
extern int tfmload() ;
extern FILE *search() ;
extern shalfword pkbyte() ;
extern integer pkquad() ;
extern integer pktrio() ;
extern Boolean pkopen() ;
extern char *getenv() ;
extern char *newstring() ;
extern int add_header() ;
extern int add_name() ;
extern char *get_name() ;
extern int system() ;
extern void handlepapersize() ;
extern void checkstrings() ;
void getpsinfo() ;
extern void *revlist() ;
/*
 *   These are the external variables we use.
 */
#ifdef DEBUG
extern integer debug_flag;
#endif  /* DEBUG */
extern integer pagecopies ;
extern int overridemag ;
extern long bytesleft ;
extern quarterword *raster ;
extern FILE *pkfile ;
extern char *oname ;
extern integer swmem, fontmem ;
extern char *tfmpath, *pictpath ;
extern char *pkpath ;
extern char *vfpath ;
extern char *figpath ;
extern char *configpath ;
extern Boolean noenv ;
extern Boolean downloadpspk ;
#ifdef SEARCH_SUBDIRECTORIES
extern char *fontsubdirpath ;
#endif
#ifdef FONTLIB
extern char *flipath, *fliname ;
#endif
extern char *headerpath ;
extern char *paperfmt ; 
extern char *nextstring ;
extern char *maxstring ;
extern char *warningmsg ;
extern Boolean disablecomments ;
extern Boolean compressed ;
extern int quiet ;
extern int filter ;
extern Boolean reverse ;
extern Boolean usesPSfonts ;
extern Boolean nosmallchars ;
extern Boolean removecomments ;
extern Boolean safetyenclose ;
extern Boolean dopprescan ;
extern integer maxsecsize ;
extern integer mag ;
extern Boolean sepfiles ;
extern int actualdpi ;
extern int vactualdpi ;
extern int maxdrift ;
extern int vmaxdrift ;
extern char *printer ;
extern char *mfmode ;
extern Boolean sendcontrolD ;
extern int lastresortsizes[] ;
extern integer hoff, voff ;
extern struct papsiz *papsizes ;
extern Boolean secure ;
extern integer hpapersize, vpapersize ;
extern int landscape ;
/*
 *   To maintain a list of document fonts, we use the following
 *   pointer.
 */
struct header_list *ps_fonts_used ;
/*
 *   Our hash routine.
 */
int
hash(s)
   char *s ;
{
   int h = 12 ;

   while (*s != 0)
      h = (h + h + *s++) % RESHASHPRIME ;
   return(h) ;
}
/*
 *   Reverse the hash chains.
 */
void
revpslists() {
   register int i ;
   for (i=0; i<RESHASHPRIME; i++)
      reshash[i] = (struct resfont *)revlist(reshash[i]) ;
}
/*
 *   cleanres() marks all resident fonts as not being yet sent.
 */
void
cleanres() {
   register int i ;
   register struct resfont *p ;
   for (i=0; i<RESHASHPRIME; i++)
      for (p=reshash[i]; p; p=p->next)
         p->sent = 0 ;
}
/*
 *   The routine that looks up a font name.
 */
struct resfont *
lookup(name)
   char *name ;
{
   struct resfont *p ;

   for (p=reshash[hash(name)]; p!=NULL; p=p->next)
      if (strcmp(p->Keyname, name)==0)
         return(p) ;
   return(NULL) ;
}
/*
 *   This routine adds an entry.
 */
void
add_entry(TeXname, PSname, specinfo, downloadinfo)
   char *TeXname, *PSname, *specinfo, *downloadinfo ;
{
   struct resfont *p ;
   int h ;

   if (PSname == NULL)
      PSname = TeXname ;
   p = (struct resfont *)mymalloc((integer)sizeof(struct resfont)) ;
   p->Keyname = TeXname ;
   p->PSname = PSname ;
   p->TeXname = TeXname ;
   p->specialinstructions = specinfo ;
   if (downloadinfo && *downloadinfo)
      p->downloadheader = downloadinfo ;
   else
      p->downloadheader = 0 ;
   h = hash(TeXname) ;
   p->next = reshash[h] ;
   p->sent = 0 ;
   reshash[h] = p ;
}
/*
 *   Now our residentfont routine.  Returns the number of characters in
 *   this font, based on the TFM file.
 */
extern char *infont ;
int
residentfont(curfnt)
        register fontdesctype *curfnt ;
{
   register shalfword i ;
   struct resfont *p ;

/*
 *   First we determine if we can find this font in the resident list.
 */
   if (*curfnt->area)
      return 0 ; /* resident fonts never have a nonstandard font area */
   if ((p=lookup(curfnt->name))==NULL)
      return 0 ;
/*
 *   This is not yet the correct way to do things, but it is useful as it
 *   is so we leave it in.  The problem:  if resident Times-Roman is
 *   re-encoded, then it will be downloaded as bitmaps; this is not
 *   right.  The solution will be to introduce two types of `<'
 *   directives, one that downloads fonts and one that downloads
 *   short headers that are innocuous.
 */
   if (p->downloadheader && downloadpspk) {
#ifdef DEBUG
      if (dd(D_FONTS))
         (void)fprintf(stderr,"Using PK font %s for <%s>.\n",
                                     curfnt->name, p->PSname) ;
#endif  /* DEBUG */
      return 0 ;
   }
/*
 *   We clear out some pointers:
 */
#ifdef DEBUG
   if (dd(D_FONTS))
        (void)fprintf(stderr,"Font %s <%s> is resident.\n",
                                     curfnt->name, p->PSname) ;
#endif  /* DEBUG */
   curfnt->resfont = p ;
   curfnt->name = p->TeXname ;
   for (i=0; i<256; i++) {
      curfnt->chardesc[i].TFMwidth = 0 ;
      curfnt->chardesc[i].packptr = NULL ;
      curfnt->chardesc[i].pixelwidth = 0 ;
      curfnt->chardesc[i].flags = 0 ;
   }
   add_name(p->PSname, &ps_fonts_used) ;
/*
 *   We include the font here.  But we only should need to include the
 *   font if we have a stupid spooler; smart spoolers should be able
 *   to supply it automatically.
 */
   if (p->downloadheader) {
      char *cp = p->downloadheader ;
      char *q ;

      infont = p->PSname ;
      while (1) {
         q = cp ;
         while (*cp && *cp != ' ')
            cp++ ;
         if (*cp) {
            *cp = 0 ;
            add_header(q) ;
            *cp++ = ' ' ;
         } else {
            add_header(q) ;
            break ;
         }
         infont = 0 ;
      }
      infont = 0 ;
   }
   i = tfmload(curfnt) ;
   if (i < 0)
      i = 1 ;
   usesPSfonts = 1 ;
   return(i) ;
}
#define INLINE_SIZE (500)
static char was_inline[INLINE_SIZE] ;
void
bad_config() {
   extern void exit() ;

   error("Error in config file:") ;
   (void)fprintf(stderr, "%s\n", was_inline) ;
   exit(1) ;
}
/*
 *   Get environment variables! These override entries in ./config.h.
 *   We substitute everything of the form ::, ^: or :$ with default,
 *   so a user can easily build on to the existing paths.
 */
static char *getpath(who, what)
char *who, *what ;
{
   if (who) {
      register char *pp, *qq ;
      int lastsep = 1 ;

      for (pp=nextstring, qq=who; *qq;) {
         if (*qq == PATHSEP) {
            if (lastsep) {
               strcpy(pp, what) ;
               pp = pp + strlen(pp) ;
            }
            lastsep = 1 ;
         } else
            lastsep = 0 ;
         *pp++ = *qq++ ;
      }
      if (lastsep) {
         strcpy(pp, what) ;
         pp = pp + strlen(pp) ;
      }
      *pp = 0 ;
      qq = nextstring ;
      nextstring = pp + 1 ;
      return qq ;
   } else
      return what ;
}
/*
 *   We use this function so we can support strings delimited by
 *   double quotes with spaces in them.  We also accept strings
 *   with spaces in them, but kill off any spaces at the end.
 */
char *configstring(s, nullok)
char *s ;
int nullok ;
{
   char tstr[300] ;
   char *p = tstr ;

   while (*s && *s <= ' ')
      s++ ;
   if (*s == '"') {
      s++ ;
      while (*s != 10 && *s != 0 && *s != '"' && p < tstr+290)
         *p++ = *s++ ;
   } else {
      while (*s && p < tstr+290)
         *p++ = *s++ ;
      while (*(p-1) <= ' ' && p > tstr)
         p-- ;
   }
   *p = 0 ;
   if (p == tstr && ! nullok)
      bad_config() ;
   return newstring(tstr) ;
}
/*
 *   Now we have the getdefaults routine.
 */
static char *psmapfile = PSMAPFILE ;
void
getdefaults(s)
char *s ;
{
   FILE *deffile ;
   char PSname[300] ;
   register char *p ;
   int i, j ;
   integer hsiz, vsiz ;
   char *d = configpath ;
   int canaddtopaper = 0 ;

   if (printer == NULL) {
      if (s) {
         strcpy(PSname, s) ;
      } else {
#ifndef VMCMS  /* IBM: VM/CMS - don't have home directory on VMCMS */
#ifndef MVSXA
         d = "~" ;
#endif
#endif  /* IBM: VM/CMS */
         strcpy(PSname, DVIPSRC) ;
      }
   } else {
#if defined(MSDOS) || defined(OS2)
      strcpy(PSname, printer) ;
      strcat(PSname, ".cfg") ;
#else
      strcpy(PSname, "config.") ;
      strcat(PSname, printer) ;
#endif
   }
   if ((deffile=search(d,PSname,READ))!=NULL) {
      while (fgets(was_inline, INLINE_SIZE, deffile)!=NULL) {
/*
 *   We need to get rid of the newline.
 */
       for (p=was_inline; *p; p++) ;
       if (p > was_inline) *(p-1) = 0 ;
       if (was_inline[0] != '@')
          canaddtopaper = 0 ;
       switch (was_inline[0]) {
/*
 *   Handling paper size information:
 *
 *      If line is empty, then we clear out the paper size information
 *      we have so far.
 *
 *      If it is `@+', then we add to the current paper size info.
 *
 *      If it is `name hsize vsize', then we start a new definition.
 */
case '@' :
         p = was_inline + 1 ;
         while (*p && *p <= ' ') p++ ;
         if (*p == 0) {
            papsizes = 0 ; /* throw away memory */
         } else if (*p == '+') {
            if (canaddtopaper == 0)
               error(
      " @+ in config files must immediately following a @ lines") ;
            else {
               *(nextstring-1) = '\n' ;/* IBM: VM/CMS - changed 10 to "\n" */
               p++ ;
               while (*p && *p == ' ') p++ ;
               strcpy(nextstring, p) ;
               nextstring += strlen(p) + 1 ;
            }
         } else {
            struct papsiz *ps ;
            
            ps = (struct papsiz *)mymalloc((integer)sizeof(struct papsiz)) ;
            ps->next = papsizes ;
            papsizes = ps ;
            ps->name = p ;
            while (*p && *p > ' ')
               p++ ;
            *p++ = 0 ;
            ps->name = newstring(ps->name) ;
            while (*p && *p <= ' ') p++ ;
            handlepapersize(p, &hsiz, &vsiz) ;
            ps->xsize = hsiz ;
            ps->ysize = vsiz ;
            ps->specdat = nextstring++ ;
            canaddtopaper = 1 ;
         }
         break ;
case 'a' :
         dopprescan = (was_inline[1] != '0') ;
         break ;
case 'b':
#ifdef SHORTINT
         if (sscanf(was_inline+1, "%ld", &pagecopies) != 1) bad_config() ;
#else
         if (sscanf(was_inline+1, "%d", &pagecopies) != 1) bad_config() ;
#endif
         if (pagecopies < 1 || pagecopies > 1000)
            bad_config() ;
         break ;
case 'm' :
#ifdef SHORTINT
         if (sscanf(was_inline+1, "%ld", &swmem) != 1) bad_config() ;
#else   /* ~SHORTINT */
         if (sscanf(was_inline+1, "%d", &swmem) != 1) bad_config() ;
#endif  /* ~SHORTINT */
         swmem += fontmem ; /* grab headers we've seen already */
         break ;
case 'M' :
         mfmode = configstring(was_inline+1, 0) ;
         break ;
case 'o' :
         oname = configstring(was_inline+1, 1) ;
         if (*oname && oname[strlen(oname)-1] == ':')
            sendcontrolD = 1 ; /* if we send to a device, *we* are spooler */
         break ;
case 'O' :
         p = was_inline + 1 ;
         handlepapersize(p, &hoff, &voff) ;
         break ;
#ifdef FONTLIB
case 'L' : 
         {
            char tempname[300] ;
            extern char *fliparse() ;
            if (sscanf(was_inline+1, "%s", PSname) != 1) bad_config() ;
            else {
               flipath = getpath(fliparse(PSname,tempname), flipath);
               fliname = newstring(tempname) ;
            }
	 }
         break ;
#endif
case 'T' : 
         if (sscanf(was_inline+1, "%s", PSname) != 1) bad_config() ;
         else tfmpath = getpath(PSname, tfmpath) ;
         break ;
case 'P' :
         if (sscanf(was_inline+1, "%s", PSname) != 1) bad_config() ;
         else pkpath = getpath(PSname, pkpath) ;
         break ;
case 'p' :
         p = was_inline + 1 ;
         while (*p && *p <= ' ')
            p++ ;
         if (*p == '+') {
            if (sscanf(p+1, "%s", PSname) != 1) bad_config() ;
            getpsinfo(PSname) ;
         } else {
            psmapfile = configstring(was_inline+1, 0) ;
         }
         break ;
case 'v' : case 'V' :
         if (sscanf(was_inline+1, "%s", PSname) != 1) bad_config() ;
         else vfpath = getpath(PSname, vfpath) ;
         break ;
case 'S' :
         if (sscanf(was_inline+1, "%s", PSname) != 1) bad_config() ;
         else figpath = getpath(PSname, figpath) ;
         break ;
case 's':
         safetyenclose = 1 ;
         break ;
case 'H' : 
         if (sscanf(was_inline+1, "%s", PSname) != 1) bad_config() ;
         else headerpath = getpath(PSname, headerpath) ;
         break ;
case '%': case ' ' : case '*' : case '#' : case ';' :
case '=' : case 0 : case '\n' :
         break ;
case 'r' :
         reverse = (was_inline[1] != '0') ;
         break ;
/*
 *   This case is for last resort font scaling; I hate this, but enough
 *   people have in no uncertain terms demanded it that I'll go ahead and
 *   add it.
 *
 *   This line must have numbers on it, resolutions, to search for the
 *   font as a last resort, and then the font will be scaled.  These
 *   resolutions should be in increasing order.
 *
 *   For most machines, just `300' is sufficient here; on the NeXT,
 *   `300 400' may be more appropriate.
 */
case 'R':
         i = 0 ;
         p = was_inline + 1 ;
         while (*p) {
            while (*p && *p <= ' ')
               p++ ;
            if ('0' <= *p && *p <= '9') {
               j = 0 ;
               while ('0' <= *p && *p <= '9')
                  j = 10 * j + (*p++ - '0') ;
               if (i > 0)
                  if (lastresortsizes[i-1] > j) {
                     error("last resort sizes (R) must be sorted") ;
                     bad_config() ;
                  }
               lastresortsizes[i++] = j ;
            } else {
               if (*p == 0)
                  break ;
               error("! only numbers expected on `R' line in config!") ;
            }
         }
         lastresortsizes[i] = 32000 ;
         break ;
case 'D' :
         if (sscanf(was_inline+1, "%d", &actualdpi) != 1) bad_config() ;
         if (actualdpi < 10 || actualdpi > 10000) bad_config() ;
	 vactualdpi = actualdpi;
         break ;
/*
 *   Execute a command.  This can be dangerous, but can also be very useful.
 */
case 'E' :
#ifdef SECURE
         error("dvips was compiled with SECURE, which disables E in config") ;
#else
         if (secure) {
            error("dvips -R option used, which disables E in config") ;
            break ;
         }
         (void)system(was_inline+1) ;
#endif
         break ;
case 'K':
         removecomments = (was_inline[1] != '0') ;
         break ;
case 'U':
         nosmallchars = (was_inline[1] != '0') ;
         break ;
case 'W':
         for (p=was_inline+1; *p && *p <= ' '; p++) ;
         if (*p)
            warningmsg = newstring(p) ;
         else
            warningmsg = 0 ;
         break ;
case 'X' :
         if (sscanf(was_inline+1, "%d", &actualdpi) != 1) bad_config() ;
         if (actualdpi < 10 || actualdpi > 10000) bad_config() ;
         break ;
case 'Y' :
         if (sscanf(was_inline+1, "%d", &vactualdpi) != 1) bad_config() ;
         if (vactualdpi < 10 || vactualdpi > 10000) bad_config() ;
         break ;
case 'x': case 'y':
         if (sscanf(was_inline+1, "%d", &mag) != 1) bad_config() ;
         overridemag = (was_inline[0] == 'x') ? 1 : -1 ;
         break ;
case 'e' :
         if (sscanf(was_inline+1, "%d", &maxdrift) != 1) bad_config() ;
         if (maxdrift < 0) bad_config() ;
	 vmaxdrift = maxdrift;
         break ;
case 'q' : case 'Q' :
         quiet = (was_inline[1] != '0') ;
         break ;
case 'f' : case 'F' :
         filter = (was_inline[1] != '0') ;
         break ;
case 'h' : 
         if (sscanf(was_inline+1, "%s", PSname) != 1) bad_config() ;
         else (void)add_header(PSname) ;
         break ;
case 'i' :
         if (sscanf(was_inline+1, "%d", &maxsecsize) != 1)
            maxsecsize = 0 ;
         sepfiles = 1 ;
         break ;
case 'I':
         noenv = (was_inline[1] != '0') ;
         break ;
case 'N' :
         disablecomments = (was_inline[1] != '0') ;
         break ;
case 'Z' :
         compressed = (was_inline[1] != '0') ;
         break ;
case 't' :
         if (sscanf(was_inline+1, "%s", PSname) != 1) bad_config() ;
         else {
           if (strcmp(PSname, "landscape") == 0) {
               if (hpapersize || vpapersize)
                  error(
            "both landscape and papersize specified; ignoring landscape") ;
               else
                  landscape = 1 ;
            } else
               paperfmt = newstring(PSname) ;
         }
         break ;
default:
         bad_config() ;
      }
     }
     (void)fclose(deffile) ;
   } else {
      if (printer)
         error("! no such printer (can't find corresponding config file)") ;
   }
}

/*
 *   If a character pointer is passed in, use that name; else, use the
 *   default (possibly set) name.
 */
void getpsinfo(name)
char *name ;
{
   FILE *deffile ;
   register char *p ;
   char *specinfo, *downloadinfo ;
   char downbuf[200] ;

   if (name == 0)
      name = psmapfile ;
   if ((deffile=search(configpath, name, READ))!=NULL) {
      while (fgets(was_inline, INLINE_SIZE, deffile)!=NULL) {
         p = was_inline ;
         if (*p > ' ' && *p != '*' && *p != '#' && *p != ';' && *p != '%') {
            char *TeXname = NULL ;
            char *PSname = NULL ;
            specinfo = NULL ;
            downloadinfo = NULL ;
            downbuf[0] = 0 ;
            while (*p) {
               while (*p && *p <= ' ')
                  p++ ;
               if (*p) {
                  if (*p == '"')
                     specinfo = p + 1 ;
                  else if (*p == '<') {
                     if (downloadinfo) {
                        strcat(downbuf, downloadinfo) ;
                        strcat(downbuf, " ") ;
                     }
                     while (p[1] == ' ' || p[1] == '\t')
                        p++ ;
                     downloadinfo = p + 1 ;
                  } else if (TeXname)
                     PSname = p ;
                  else
                     TeXname = p ;
                  if (*p == '"') {
                     p++ ;
                     while (*p != '"' && *p)
                        p++ ;
                  } else
                     while (*p > ' ')
                        p++ ;
                  if (*p)
                     *p++ = 0 ;
               }
            }
            if (downloadinfo)
               strcat(downbuf, downloadinfo) ;
            if (TeXname) {
               TeXname = newstring(TeXname) ;
               specinfo = newstring(specinfo) ;
               PSname = newstring(PSname) ;
               downloadinfo = newstring(downbuf) ;
               add_entry(TeXname, PSname, specinfo, downloadinfo) ;
            }
   	 }
      }
      (void)fclose(deffile) ;
   }
   checkstrings() ;
}
/*
 *   Get environment variables! These override entries in ./config.h.
 *   We substitute everything of the form ::, ^: or :$ with default,
 *   so a user can easily build on to the existing paths.
 */
static char *getenvup(who, what)
char *who, *what ;
{
   return getpath(getenv(who), what) ;
}
#ifdef SEARCH_SUBDIRECTORIES
static char *concat3();
#endif
void checkenv(which)
int which ;
{
   if (which) {
      tfmpath = getenvup(FONTPATH, tfmpath) ;
      vfpath = getenvup("VFFONTS", vfpath) ;
      pictpath = getenvup("TEXPICTS", pictpath) ;
      figpath = getenvup("TEXINPUTS", figpath) ;
      headerpath = getenvup("DVIPSHEADERS", headerpath) ;
      if (getenv("TEXPKS"))
         pkpath = getenvup("TEXPKS", pkpath) ;
      else if (getenv("TEXPACKED"))
         pkpath = getenvup("TEXPACKED", pkpath) ;
      else if (getenv("PKFONTS"))
         pkpath = getenvup("PKFONTS", pkpath) ;
#ifdef SEARCH_SUBDIRECTORIES
      else if (getenv(FONTPATH))
         pkpath = getenvup(FONTPATH, pkpath) ;
      if (getenv ("TEXFONTS_SUBDIR"))
         fontsubdirpath = getenvup ("TEXFONTS_SUBDIR", fontsubdirpath);
      {
         char pathsep[2] ;
         char *do_subdir_path();
         char *dirs = do_subdir_path (fontsubdirpath);
         /* If the paths were in dynamic storage before, that memory is
            wasted now.  */
         pathsep[0] = PATHSEP ;
         pathsep[1] = '\0' ;
         tfmpath = concat3 (tfmpath, pathsep, dirs);
         pkpath = concat3 (pkpath, pathsep, dirs);
      }
#endif
   } else
      configpath = getenvup("TEXCONFIG", configpath) ;
}

#ifdef SEARCH_SUBDIRECTORIES

#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>

#ifdef SYSV
#include <dirent.h>
typedef struct dirent *directory_entry_type;
#else
#include <sys/dir.h>
typedef struct direct *directory_entry_type;
#endif

/* Declare the routine to get the current working directory.  */

#ifndef HAVE_GETCWD
extern char *getwd ();
#define getcwd(b, len)  ((b) ? getwd (b) : getwd (xmalloc (len)))
#else
#ifdef ANSI
extern char *getcwd (char *, int);
#else
extern char *getcwd ();
#endif /* not ANSI */
#endif /* not HAVE_GETWD */

#if defined(SYSV) || defined(VMS) || defined(MSDOS) || defined(OS2) || defined(ATARIST)
#define MAXPATHLEN (256)
#else   /* ~SYSV */
#include <sys/param.h>          /* for MAXPATHLEN */
#endif

extern void exit() ;
extern int chdir() ;

/* Memory operations: variants of malloc(3) and realloc(3) that just
   give up the ghost when they fail.  */

extern char *realloc ();

char *
xmalloc (size)
  unsigned size;
{
  char *mem = malloc (size);
  
  if (mem == NULL)
    {
      fprintf (stderr, "! Cannot allocate %u bytes.\n", size);
      exit (10);
    }
  
  return mem;
}


char *
xrealloc (ptr, size)
  char *ptr;
  unsigned size;
{
  char *mem = realloc (ptr, size);
  
  if (mem == NULL)
    {
      fprintf (stderr, "! Cannot reallocate %u bytes at %x.\n", size, (int)ptr);
      exit (10);
    }
    
  return mem;
}


/* Return, in NAME, the next component of PATH, i.e., the characters up
   to the next PATHSEP.  */
   
static void
next_component (name, path)
  char name[];
  char **path;
{
  unsigned count = 0;
  
  while (**path != 0 && **path != PATHSEP)
    {
      name[count++] = **path;
      (*path)++; /* Move further along, even between calls.  */
    }
  
  name[count] = 0;
  if (**path == PATHSEP)
    (*path)++; /* Move past the delimiter.  */
}


#ifndef _POSIX_SOURCE
#ifndef S_ISDIR
#define S_ISDIR(m) ((m & S_IFMT) == S_IFDIR)
#endif
#endif

/* Return true if FN is a directory or a symlink to a directory,
   false if not. */

int
is_dir (fn)
  char *fn;
{
  struct stat stats;

  return stat (fn, &stats) == 0 && S_ISDIR (stats.st_mode);
}


static char *
concat3 (s1, s2, s3)
  char *s1, *s2, *s3;
{
  char *r = xmalloc (strlen (s1) + strlen (s2) + strlen (s3) + 1);
  strcpy (r, s1);
  strcat (r, s2);
  strcat (r, s3);
  return r;
}


/* DIR_LIST is the default list of directories (colon-separated) to
   search.  We want to add all the subdirectories directly below each of
   the directories in the path.
     
   We return the list of directories found.  */

char *
do_subdir_path (dir_list)
  char *dir_list;
{
  char *cwd;
  unsigned len;
  char *result = xmalloc ((unsigned)1);
  char *temp = dir_list;
  char dirsep[2] ;

  dirsep[0] = DIRSEP ;
  dirsep[1] = '\0' ;

  /* Make a copy in writable memory.  */
  dir_list = xmalloc (strlen (temp) + 1);
  strcpy (dir_list, temp);
  
  *result = 0;

  /* Unfortunately, we can't look in the environment for the current
     directory, because if we are running under a program (let's say
     Emacs), the PWD variable might have been set by Emacs' parent
     to the current directory at the time Emacs was invoked.  This
     is not necessarily the same directory the user expects to be
     in.  So, we must always call getcwd(3) or getwd(3), even though
     they are slow and prone to hang in networked installations.  */
  cwd = getcwd (NULL, MAXPATHLEN + 2);
  if (cwd == NULL)
    {
      perror ("getcwd");
      exit (errno);
    }

  do
    {
      DIR *dir;
      directory_entry_type e;
      char dirname[MAXPATHLEN];

      next_component (dirname, &dir_list);

      /* All the `::'s should be gone by now, but we may as well make
         sure `chdir' doesn't crash.  */
      if (*dirname == 0) continue;

      /* By changing directories, we save a bunch of string
         concatenations (and make the pathnames the kernel looks up
         shorter).  */
      if (chdir (dirname) != 0) continue;

      dir = opendir (".");
      if (dir == NULL) continue;

      while ((e = readdir (dir)) != NULL)
        {
          if (is_dir (e->d_name) && strcmp (e->d_name, ".") != 0
              && strcmp (e->d_name, "..") != 0)
            {
              char *found = concat3 (dirname, dirsep, e->d_name);

              result = xrealloc (result, strlen (result) + strlen (found) + 2);

              len = strlen (result);
              if (len > 0)
                {
                  result[len] = PATHSEP;
                  result[len + 1] = 0;
                }
              strcat (result, found);
              free (found);
            }
        }
      closedir (dir);

      /* Change back to the current directory, in case the path
         contains relative directory names.  */
      if (chdir (cwd) != 0)
        {
          perror (cwd);
          exit (errno);
        }
    }
  while (*dir_list != 0);
  
  return result;
}
#endif /* SEARCH_SUBDIRECTORIES */
