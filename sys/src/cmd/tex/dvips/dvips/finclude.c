/*
 *  Code for allowing fonts to be used in PostScript files given via
 *  `psfile=...'.
 */
#include "structures.h" /* The copyright notice in that file is included too! */
#include <ctype.h>
#ifndef SYSV
extern char *strtok() ; /* some systems don't have this in strings.h */
#endif
#ifdef VMS
#define getname vms_getname
#endif

double atof();
/*
 *   These are the external routines we call.
 */
extern fontdesctype *newfontdesc() ;
extern fontdesctype *matchfont() ;
extern Boolean prescanchar() ;
extern Boolean preselectfont() ;
extern void scout() ;
extern void stringend() ;
extern void cmdout() ;
extern void numout() ;
extern void lfontout() ;
extern char *newstring() ;
extern FILE *search() ;
/*
 *   These are the external variables we access.
 */
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
extern Boolean includesfonts ;
extern char *figpath ;
/*
 * Create a font descriptor for a font included in a psfile.  There will be
 * no fontmaptype node for the resulting font descriptor until this font is
 * encountered by fontdef() (if that ever happens).
 */
fontdesctype *
ifontdef(name, area, scsize, dssize, scname)
char *name, *scname, *area ;
integer scsize, dssize ;
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
setfamily(f)
fontdesctype *f ;
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
getname(s)
char *s ;
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
 * Mark character usage in *f based on the hexadicimal bitmap found in
 * string s.  A two-digit offset separated by a colon gives the initial
 * character code.  We have no way of knowing how many times each character
 * is used or how many strings get created when showing the characters so
 * we just estimate two usages per character and one string per pair of
 * usages.
 */
void
includechars(f, s)
fontdesctype *f ;
char *s ;
{
   int b, c, d ;
   int l = strlen(s) ;

   if (l>0 && s[l-1]=='\n')
      s[--l] = 0 ;
   if (!isxdigit(s[0]) || !isxdigit(s[1]) || s[2]!=':'
         || strspn(s+3,"0123456789ABCDEFabcdef") < l-3) {
      fprintf(stderr, "%s\n", s) ;
      error("Bad syntax in included font usage table") ;
      return ;
   }
   c = (xdig[s[0]] << 4) + xdig[s[1]] ;
   s += 2 ;
   while (*++s) {
      d = xdig[*s] ;
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
 */
void
scan1fontcomment(p)
char *p ;
{
   char *q, *name, *area;
   char *scname;      /* location in buffer where we got scsize */
   integer scsize, dssize;
   fontdesctype *fptr;
   real DVIperBP;

   DVIperBP = actualdpi/(72.0*conv);
   p = strtok(p, " ");
   if (p==NULL) return;
   area = nextstring ;   /* tentatively in the string pool */
   name = getname(p);
   q = strtok((char *)0, " ");
   if (p==NULL || (scsize=(int)(atof(q)*DVIperBP))==0) {
      fprintf(stderr, "%s\n",p);
      error("No scaled size for included font");
      nextstring = area ;   /* remove from string pool */
      return;
   }
   scname = q;
   q = strtok((char *)0, " ");
   if (p==NULL || (dssize=(int)(atof(q)*DVIperBP))==0) {
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
scanvm(p)
char *p ;
{
   char* q;
   integer vm, vmmax;
   extern long atol() ;

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
 * Scan an initial sequence of comment lines looking for font and memory
 * usage specifications.  This does not handle the "atend" construction.
 */
void
scanfontcomments(filename)
char* filename ;
{
   char p[500];
   FILE *f;
   integer truecost = pagecost ;
   Boolean trueknown = 0 ;
   fontdesctype *oldcf = curfnt;

   f = search(figpath, filename, READ) ;
   if (f) {
      while (fgets(p,500,f) && p[0]=='%' &&
            (p[1]=='!' || p[1]=='%' || p[1]=='*'))
         if (strncmp(p, "%*Font:", 7) == 0)
            scan1fontcomment(p+7);
         else if (strncmp(p, "%%VMusage:", 9) == 0) {
            truecost += scanvm(p+10) ;
            trueknown = 1 ;
         }
      if (trueknown)
         pagecost = truecost ;
      fclose(f) ;
   }
   curfnt = oldcf;
}
/*
 * Is string s less than 30 characters long with no special characters
 * that are not allowed in PostScript commands.
 */
Boolean
okascmd(ss)
char *ss ;
{
   register c = 0 ;
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
nameout(area, name)
char *area, *name ;
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
fonttableout()
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
