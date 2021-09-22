/*
 *  Code for allowing fonts to be used in PostScript files given via
 *  `psfile=...'.
 *
 * Modified to check for font usage (DocumentFonts and DocumentNeededResources)
 * in all included postscript files. This information is added to the
 * global list of postscript fonts used (ps_fonts_used). Each font is
 * also looked up (from psfonts.map) and if a file needs to be downloaded
 * for that font, it is added (with add_header).
 * 11/1996 Aaron Sawdey
 */
#include "dvips.h" /* The copyright notice in that file is included too! */
#ifdef KPATHSEA
#include <kpathsea/c-ctype.h>
#else
#include <ctype.h>
#if !defined(SYSV) && !defined(WIN32)
extern char *strtok() ; /* some systems don't have this in strings.h */
#endif
#define ISXGIGIT isxdigit
#endif
#ifdef VMS
#define getname vms_getname
#endif

#ifndef STDC_HEADERS
double atof();
#endif

/*
 *   These are the external routines we call.
 */
#include "protos.h"

/*
 *   These are the external variables we access.
 */
extern struct header_list *ps_fonts_used;
extern char *infont ;
extern fontdesctype *curfnt ;
extern fontdesctype *fonthead ;
extern integer fontmem ;
extern fontdesctype *fonthd[MAXFONTHD] ;
extern int nextfonthd ;
extern char *nextstring ;
extern char xdig[256] ;
extern real conv ;
extern integer pagecost ;
extern int actualdpi ;
extern double mag ;
extern Boolean includesfonts ;
#ifndef KPATHSEA
extern char *figpath ;
#endif
extern int to_close ;
#ifdef DEBUG
extern integer debug_flag;
#endif  /* DEBUG */

/*
 * Create a font descriptor for a font included in a psfile.  There will be
 * no fontmaptype node for the resulting font descriptor until this font is
 * encountered by fontdef() (if that ever happens).
 */
fontdesctype *
ifontdef P5C(char *, name, char *, area, 
	     int, scsize, int, dssize, char *, scname)
{
   fontdesctype *fp;

   fp = newfontdesc((integer)0, scsize, dssize, name, area);
   fp->scalename = scname;
   fp->next = fonthead ;
   fonthead = fp ;
   return fp;
}
/*
 * When a font appears in an included psfile for the first time, this routine
 * links it into the fonthd[] array.
 */
void
setfamily P1C(fontdesctype *, f)
{
   int i ;

   fontmem -= DICTITEMCOST;
   for (i=0; i<nextfonthd; i++)
      if (strcmp(f->name, fonthd[i]->name)==0
            && strcmp(f->area, fonthd[i]->area)==0) {
         f->nextsize = fonthd[i];
         fonthd[i] = f;
         return;
      }
   if (nextfonthd==MAXFONTHD)
      error("! Too many fonts in included psfiles") ;
   fontmem -= NAMECOST + strlen(f->name) + strlen(f->area) ;
   fonthd[nextfonthd++] = f ;
   f->nextsize = NULL ;
}
/*
 * Convert file name s to a pair of new strings in the string pool.
 * The first string is the original value of nextstring; the second
 * string is the return value.
 */
char*
getname P1C(char *, s)
{
   char *a, *p, sav;

   a = NULL;
   for (p=s; *p!=0; p++)
      if (*p=='/')
         a = p+1 ;
   if (a==NULL) *nextstring++ = 0 ;
   else {   sav = *a ;
      *a = 0 ;
      (void) newstring(s) ;
      *a = sav ;
      s = a ;
   }
   return newstring(s);
}
/*
 * Mark character usage in *f based on the hexadecimal bitmap found in
 * string s.  A two-digit offset separated by a colon gives the initial
 * character code.  We have no way of knowing how many times each character
 * is used or how many strings get created when showing the characters so
 * we just estimate two usages per character and one string per pair of
 * usages.
 */
void
includechars P2C(fontdesctype *, f, char *, s)
{
   int b, c, d ;
   int l = strlen(s) ;

   if (l>0 && s[l-1]=='\n')
      s[--l] = 0 ;
   if (!ISXDIGIT(s[0]) || !ISXDIGIT(s[1]) || s[2]!=':'
         || strspn(s+3,"0123456789ABCDEFabcdef") < l-3) {
      fprintf(stderr, "%s\n", s) ;
      error("Bad syntax in included font usage table") ;
      return ;
   }
   c = (xdig[(int)(s[0])] << 4) + xdig[(int)(s[1])] ;
   s += 2 ;
   while (*++s) {
      d = xdig[(int)*s] ;
      for (b=8; b!=0; b>>=1) {
         if ((d&b)!=0) {
            pagecost ++ ;
            (void) prescanchar(&f->chardesc[c]) ;
         }
         if (++c==256) return ;
      }
   }
}
/*
 * String p should be start after the ":" in a font declaration of the form
%*FONT: <tfm-name> <scaled-size> <design-size> <2-hex-digits>:<hex-string>
 * where the sizes are floating-point numbers in units of PostScript points
 * (TeX's "bp").  We update the data structures for the included font,
 * charge fontmem for the VM used, and add to delchar if necessary.
 * Note that the scaled size and the design size are multiplied by mag/1000.
 * This is needed for the design size to undo the similar factor in conv since
 * design sizes are not supposed to be affected by magnification.  Applying
 * the magnification factor to the scaled size selects magnified fonts as is
 * appropriate in the normal case where the included PostScript is scaled by
 * mag/1000.  The definition of `fshow' in finclude.lpro unscales by `DVImag'
 * to account for this.  We cannot change the font scaled size to account for
 * options like `hscale=' because then the definition of `fshow' would have
 * to change.
 */
void
scan1fontcomment P1C(char *, p)
{
   char *q, *name, *area;
   char *scname;      /* location in buffer where we got scsize */
   integer scsize, dssize;
   fontdesctype *fptr;
   real DVIperBP;

   DVIperBP = actualdpi/(72.0*conv) * (mag/1000.0);
   p = strtok(p, " ");
   if (p==NULL) return;
   area = nextstring ;   /* tentatively in the string pool */
   name = getname(p);
   q = strtok((char *)0, " ");
   if (p==NULL || (scsize=(integer)(atof(q)*DVIperBP))==0) {
      fprintf(stderr, "%s\n",p);
      error("No scaled size for included font");
      nextstring = area ;   /* remove from string pool */
      return;
   }
   scname = q;
   q = strtok((char *)0, " ");
   if (p==NULL || (dssize=(integer)(atof(q)*DVIperBP))==0) {
      fprintf(stderr, "%s\n",p);
      error("No design size for included font");
      nextstring = area ;
      return;
   }
   q = strtok((char *)0, " ");
   fptr = matchfont(name, area, scsize, scname);
   if (!fptr) {
      fptr = ifontdef(name, area, scsize, dssize, newstring(scname));
      (void) preselectfont(fptr);
      setfamily(fptr);
   } else {
      nextstring = area;   /* remove from string pool */
      (void) preselectfont(fptr);
      if (fptr->scalename==NULL) {
         fptr->scalename=newstring(scname);
         setfamily(fptr);
      }
   }
   includesfonts = 1;
   fptr->psflag |= THISPAGE;
   includechars(fptr, q);
}
/*
 * Parse the arguments to a "%%VMusage" comment.  The Adobe Type 1 Font Format
 * book specifies two arguments. This routine will accept one or two arguments;
 * if there are two arguments we take the maximum.
 */
integer
scanvm P1C(char *, p)
{
   char* q;
   integer vm, vmmax;

   q = strtok(p, " ");
   if (q==NULL) {
      error("Missing data in VMusage comment");
      return 0;
   }
   vmmax = atol(q);
   q = strtok((char *)0, " ");
   if (q!=NULL && (vm=atol(q))>vmmax)
      vmmax = vm;
   return vmmax;
}
/*
 * Scan a list of font names.
 * Each name is added to the list ps_fonts_used, and if it has
 * an associated header file (from psfonts.map), the header file
 * is added with add_header.
 */
void
scan_fontnames P2C(char *, str, char *, psfile)
{
  char *p,*pe;
  struct resfont *re;
  int i;

  /* Turn all newlines, CRs, and tabs into spaces. */
  p = str;
  while(*p) {
    if(*p == '\r' || *p == '\n' || *p == '\t') *p = ' ';
    p++;
  }
  /* Remove trailing spaces. */
  p = str+strlen(str)-1;
  while(p > str && *p == ' ') {
    *p = '\0';
    p--;
  }

  p = str;
  while(*p == ' ') p++; /* skip leading whitespace */

  while(p && *p) {
     pe = strchr(p,' ');
     if(pe != NULL) *pe = '\0';

     i = add_name(p,&ps_fonts_used);

     if(i) {
#ifdef DEBUG
       if (dd(D_FONTS))
         (void)fprintf(stderr,
		       "Adding font '%s' from included postscript file '%s'.\n",
		       p,psfile);
#endif  /* DEBUG */

       re = findPSname(p);
       if(re != NULL) {
         if (re->Fontfile) {
            add_header(re->Fontfile) ;
         } else if (re->downloadheader) {
	/* this code borrowed from residentfont() in resident.c */
	   char *cp = re->downloadheader ;
	   char *q ;
	 
	   infont = re->PSname ;
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
         }
	 infont = 0 ;
       } else {
         char eb[1000];
         sprintf(eb,"Font %s used in file %s is not in the mapping file.",
                 p,psfile);
         error(eb);
       }
     }

     p = pe;
     if(p != NULL) {
       p++;
       while(*p == ' ') p++; /* skip leading whitespace */
     }
  }

  return;
}

/*
 * fc_state == 0: normal state, looking for interesting comments
 * fc_state == 1: looking for %%+ following %%DocumentFonts
 * fc_state == 2: looking for "%%+ font" following %%DocumentNeededResources
 */
static int fc_state = 0;
/*
 * Do we need to check for information at the end of the postscript file? 
 */
static int check_atend = 0;

void
scanfontusage P2C(char *, p, char *, psfile)
{
  if (strncmp(p, "%%DocumentFonts: ",17) == 0) {
    p += 17 ;
    while (*p && *p <= ' ')
       p++ ;
    if(!strncmp(p,"(atend)",7)) {
      check_atend = 1;
    } else {
      scan_fontnames(p,psfile);
      fc_state = 1;
    }
  } else if (strncmp(p, "%%DocumentNeededFonts: ",23)==0) {
    p += 23 ;
    while (*p && *p <= ' ')
       p++ ;
    if(!strncmp(p,"(atend)",7)) {
      check_atend = 1;
    } else {
      scan_fontnames(p,psfile);
      fc_state = 1;
    }
  } else if (fc_state == 1 && strncmp(p,"%%+",3) == 0) {
    scan_fontnames(p+3,psfile);
    fc_state = 1;
  } else if (strncmp(p, "%%DocumentNeededResources: ",27) == 0) {
    p += 27 ;
    while (*p && *p <= ' ')
       p++ ;
    if(!strncmp(p,"(atend)",7)) {
      check_atend = 1;
    } else {
      if(!strncmp(p,"font ",5)) scan_fontnames(p+5,psfile);
      fc_state = 2;
    }
  } else if (fc_state == 2 && strncmp(p,"%%+",3) == 0) {
    p += 3 ;
    while (*p && *p <= ' ')
       p++ ;
    if(!strncmp(p,"font ",5)) scan_fontnames(p+5,psfile);
    fc_state = 2;
  } else {
    fc_state = 0;
  }
  return;
}

/*
 * Scan an initial sequence of comment lines looking for font and memory
 * usage specifications.  This does not handle the "atend" construction.
 */
void
scanfontcomments P1C(char *, filename)
{
   char p[500];
   char *r;
   FILE *f;
   integer truecost = pagecost ;
   Boolean trueknown = 0 ;
   fontdesctype *oldcf = curfnt;

#ifdef DEBUG
      if (dd(D_FONTS))
         (void)fprintf(stderr,
		       "Checking for fonts in '%s'\n",filename);
#endif  /* DEBUG */

   if (*filename == '`') {
/*
 *   Allow scanning of ` commands.  Better return same results both times.
 */
      f = popen(filename+1, FOPEN_RBIN_MODE) ;
      to_close = USE_PCLOSE ;
   } else {
      f = search(figpath, filename, READ) ;
   }
   if (f) {
     fc_state = 0;
     check_atend = 0;
     while (fgets(p,500,f) && p[0]=='%' &&
            (p[1]=='!' || p[1]=='%' || p[1]=='*')) {
       if (strncmp(p, "%*Font:", 7) == 0) {
	 scan1fontcomment(p+7);
       } else if (strncmp(p, "%%VMusage:", 9) == 0) {
	 truecost += scanvm(p+10) ;
	 trueknown = 1 ;
       }
       scanfontusage(p,filename);
     }
     if (trueknown)
       pagecost = truecost ;

     if(check_atend) {
#ifdef DEBUG
       if (dd(D_FONTS))
         (void)fprintf(stderr,
		       "Checking for (atend) fonts in '%s'\n",filename);
#endif  /* DEBUG */

       fc_state = 0;
       
       fseek(f,-4096,2); /* seek to 4096 bytes before EOF. */
       fgets(p,500,f); /* throw away a partial line. */

       /* find %%Trailer */
       while((r=fgets(p,500,f)) && strncmp(p,"%%Trailer",9)) ;

       /* look for specs that were deferred to the trailer. */
       if(r != NULL) {
	 while(fgets(p,500,f)) {
	   if(p[0]=='%' && p[1]=='%') scanfontusage(p,filename);
	 }
       }
#ifdef DEBUG
       else { /* r == NULL */
	 if (dd(D_FONTS))
         (void)fprintf(stderr,
		       "Did not find %%%%Trailer in included file '%s'.\n",
		       filename);
       }
#endif  /* DEBUG */
     }
     close_file(f) ;
   }
   curfnt = oldcf;
}
/*
 * Is string s less than 30 characters long with no special characters
 * that are not allowed in PostScript commands.
 */
Boolean
okascmd P1C(char *, ss)
{
   register int c = 0 ;
   register char *s = ss ;

   while (*s)
      if (*s<' ' || *s>126 || ++c==30)
         return(0) ;
   return(strcspn(ss,"()<>[]{}%/") == c) ;
}
/*
 * Output font area and font name strings as a literal string
 */
void
nameout P2C(char *, area, char *, name)
{
   char buf[30] ;
   char *s ;

   if (*area==0 && okascmd(name)) {
      (void)sprintf(buf, "/%s", name) ;
      cmdout(name);
   } else {
      for (s=area; *s; s++)
         scout(*s) ;
      for (s=name; *s; s++)
         scout(*s) ;
      stringend();
      cmdout("cvn") ;
   }
}
/*
 * Output commands for defining a table of PostScript font identifiers for
 * fonts used in included psfiles in the current section.
 */
void
fonttableout P1H(void)
{
   int i, k;
   fontdesctype *f;

   for (i=0; i<nextfonthd; i++) {
      for (f=fonthd[i]; f!=NULL; f=f->nextsize)
         if (f->psflag==EXISTS) break;
      if (f!=NULL) {
         nameout(f->area, f->name);
         k = 0;
         do {   if (f->psflag==EXISTS) {
               cmdout(f->scalename);
               lfontout((int)f->psname);
            }
            f = f->nextsize;
            k++;
         } while (f!=NULL);
         numout((integer)k);
         cmdout("fstore");
      }
   }
}
