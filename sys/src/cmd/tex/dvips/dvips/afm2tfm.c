/*
 *   This program converts AFM files to TeX TFM files, and optionally
 *   to TeX VPL files that retain all kerning and ligature information.
 *   Both files make the characters not normally encoded by TeX available
 *   by character codes greater than 127.
 */

/*   (Modified by Don Knuth from Tom Rokicki's pre-VPL version.) */
/*   VM/CMS port by J. Hafner (hafner@almaden.ibm.com), based on
 *   the port by Alessio Guglielmi (guglielmi@ipisnsib.bitnet)
 *   and Marco Prevedelli (prevedelli@ipisnsva.bitnet).
 *   This port is still in test state.  No guarantees.
 *
*/

#include <stdio.h>
#ifdef SYSV
#include <string.h>
#else
#ifdef VMS
#include <string.h>
#else
#ifdef __THINK__
#include <string.h>
#else
#include <strings.h>
#endif
#endif
#endif
#include <math.h>

#ifdef VMCMS
#define interesting lookstr  /* for 8 character truncation conflicts */
#include "dvipscms.h"
extern FILE *cmsfopen() ;
extern char ebcdic2ascii[] ;
#ifdef fopen
#undef fopen
#endif
#define fopen cmsfopen
#endif

#ifdef MSDOS
#define WRITEBIN "wb"
#else
#ifdef VMCMS
#define WRITEBIN "wb, lrecl=1024, recfm=f"
#else
#define WRITEBIN "w"
#endif
#endif

#ifdef __TURBOC__
#define SMALLMALLOC
#endif

struct encoding {
   char *name ;
   char *vec[256] ;
} ;
struct encoding staticencoding = {
  "TeX text",
  {"Gamma", "Delta", "Theta", "Lambda", "Xi", "Pi", "Sigma",
   "Upsilon", "Phi", "Psi", "Omega", "arrowup", "arrowdown", "quotesingle",
   "exclamdown", "questiondown", "dotlessi", "dotlessj", "grave", "acute",
   "caron", "breve", "macron", "ring", "cedilla", "germandbls", "ae", "oe",
   "oslash", "AE", "OE", "Oslash", "space", "exclam", "quotedbl", "numbersign",
   "dollar", "percent", "ampersand", "quoteright", "parenleft", "parenright",
   "asterisk", "plus", "comma", "hyphen", "period", "slash", "zero", "one",
   "two", "three", "four", "five", "six", "seven", "eight", "nine", "colon",
   "semicolon", "less", "equal", "greater", "question", "at", "A", "B", "C",
   "D", "E", "F", "G", "H", "I", "J", "K", "L", "M", "N", "O", "P", "Q", "R",
   "S", "T", "U", "V", "W", "X", "Y", "Z", "bracketleft", "backslash",
   "bracketright", "circumflex", "underscore", "quoteleft", "a", "b", "c", "d",
   "e", "f", "g", "h", "i", "j", "k", "l", "m", "n", "o", "p", "q", "r", "s",
   "t", "u", "v", "w", "x", "y", "z", "braceleft", "bar", "braceright",
   "tilde", "dieresis", "", "", "", "", "", "", "", "", "", "", "", "", "",
    "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
    "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
    "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
    "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
    "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
    "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
    "", "", "", "", "", "", "" } } ;
/*
 *   It's easier to put this in static storage and parse it as we go
 *   than to build the structures ourselves.
 */
char *staticligkern[] = {
   "% LIGKERN space l =: lslash ; space L =: Lslash ;",
   "% LIGKERN question quoteleft =: questiondown ;",
   "% LIGKERN exclam quoteleft =: exclamdown ;",
   "% LIGKERN hyphen hyphen =: endash ; endash hyphen =: emdash ;",
   "% LIGKERN quoteleft quoteleft =: quotedblleft ;",
   "% LIGKERN quoteright quoteright =: quotedblright ;",
   "% LIGKERN space {} * ; * {} space ; 0 {} * ; * {} 0 ;",
   "% LIGKERN 1 {} * ; * {} 1 ; 2 {} * ; * {} 2 ; 3 {} * ; * {} 3 ;",
   "% LIGKERN 4 {} * ; * {} 4 ; 5 {} * ; * {} 5 ; 6 {} * ; * {} 6 ;",
   "% LIGKERN 7 {} * ; * {} 7 ; 8 {} * ; * {} 8 ; 9 {} * ; * {} 9 ;",
   0 } ;
/*
 *   The above layout corresponds to TeX Typewriter Type and is compatible
 *   with TeX Text because the position of ligatures is immaterial.
 */
struct encoding *outencoding = 0 ;
struct encoding *inencoding = 0 ;
char *outenname, *inenname ;/* the file names for input and output encodings */
int boundarychar = -1 ;     /* the boundary character */
int ignoreligkern ;         /* do we look at ligkern info in the encoding? */
/*
 *   This is what we store Adobe data in.
 */
struct adobeinfo {
   struct adobeinfo *next ;
   int adobenum, texnum, width ;
   char *adobename ;
   int llx, lly, urx, ury ;
   struct lig *ligs ;
   struct kern *kerns ;
   struct pcc *pccs ;
   int wptr, hptr, dptr, iptr ;
} *adobechars, *adobeptrs[256], *texptrs[256],
  *uppercase[256], *lowercase[256] ;
int nexttex[256] ; /* for characters encoded multiple times in output */
/*
 *   These are the eight ligature ops, in VPL terms and in METAFONT terms.
 */
char *vplligops[] = {
   "LIG", "/LIG", "/LIG>", "LIG/", "LIG/>", "/LIG/", "/LIG/>", "/LIG/>>", 0
} ;
char *encligops[] = {
   "=:", "|=:", "|=:>", "=:|", "=:|>", "|=:|", "|=:|>", "|=:|>>", 0
} ;
struct lig {
   struct lig *next ;
   char *succ, *sub ;
   short op, boundleft ;
} ;
struct kern {
   struct kern *next ;
   char *succ ;
   int delta ;
} ;
struct pcc {
   struct pcc *next ;
   char * partname ;
   int xoffset, yoffset ;
} ;

FILE *afmin, *vplout, *tfmout ;
char inname[200], outname[200] ; /* names of input and output files */
char buffer[255]; /* input buffer (modified while parsing) */
char obuffer[255] ; /* unmodified copy of input buffer */
char *param ; /* current position in input buffer */
char *fontname = "Unknown" ;
char *codingscheme = "Unspecified" ;
#ifdef VMCMS
char *ebcodingscheme = "Unspecified" ;
#endif
float italicangle = 0.0 ;
char fixedpitch ;
char makevpl ;
int xheight = 400 ;
int fontspace ;
int bc, ec ;
long cksum ;
float efactor = 1.0, slant = 0.0 ;
char *efactorparam, *slantparam ;
double newslant ;
char titlebuf[100] ;

void
error(s)
register char *s ;
{
   extern void exit() ;

   (void)fprintf(stderr, "%s\n", s) ;
   if (obuffer[0]) {
      (void)fprintf(stderr, "%s\n", obuffer) ;
      while (param > buffer) {
         (void)fprintf(stderr, " ") ;
         param-- ;
      }
      (void)fprintf(stderr, "^\n") ;
   }
   if (*s == '!')
      exit(1) ;
}

int
transform(x,y)
   register int x,y ;
{
   register double acc ;
   acc = efactor * x + slant *y ;
   return (int)(acc>=0? acc+0.5 : acc-0.5 ) ;
}

int
getline() {
   register char *p ;
   register int c ;

   param = buffer ;
   for (p=buffer; (c=getc(afmin)) != EOF && c != '\n';)/* changed 10 to '\n' */
      *p++ = c ;
   *p = 0 ;
   (void)strcpy(obuffer, buffer) ;
   if (p == buffer && c == EOF)
      return(0) ;
   else
      return(1) ;
}

char *interesting[] = { "FontName", "ItalicAngle", "IsFixedPitch",
   "XHeight", "C", "KPX", "CC", "EncodingScheme", NULL} ;
#define FontName (0)
#define ItalicAngle (1)
#define IsFixedPitch (2)
#define XHeight (3)
#define C (4)
#define KPX (5)
#define CC (6)
#define EncodingScheme (7)
#define NONE (-1)
int
interest(s)
char *s ;
{
   register char **p ;
   register int n ;

   for (p=interesting, n=0; *p; p++, n++)
      if (strcmp(s, *p)==0)
         return(n) ;
   return(NONE) ;
}

char *
mymalloc(len)
unsigned long len ;
{
   register char *p ;
   int i ;
#ifndef IBM6000
#ifdef MSDOS
   extern void *malloc() ;
#else
   extern char *malloc() ;
#endif
#endif

#ifdef SMALLMALLOC
   if (len > 65500L)
      error("! can't allocate more than 64K!") ;
#endif
   p = malloc((unsigned)len) ;
   if (p==NULL)
      error("! out of memory") ;
   for (i=0; i<len; i++)
      p[i] = 0 ;
   return(p) ;
}

char *
newstring(s)
char *s ;
{
   char *q = mymalloc((unsigned long)(strlen(s) + 1)) ;
   (void)strcpy(q, s) ;
   return q ;
}

char *
paramnewstring() {
   register char *p, *q ;

   p = param ;
   while (*p > ' ')
      p++ ;
   if (*p != 0)
      *p++ = 0 ;
   q = newstring(param) ;
   while (*p && *p <= ' ')
      p++ ;
   param = p ;
   return(q) ;
}

char *
paramstring() {
   register char *p, *q ;

   p = param ;
   while (*p > ' ')
      p++ ;
   q = param ;
   if (*p != 0)
      *p++ = 0 ;
   while (*p && *p <= ' ')
      p++ ;
   param = p ;
   return(q) ;
}

int
paramnum() {
   register char *p ;
   int i ;

   p = paramstring() ;
   if (sscanf(p, "%d", &i) != 1)
      error("! integer expected") ;
   return(i) ;
}

float
paramfloat() {
   register char *p ;
   float i ;

   p = paramstring() ;
   if (sscanf(p, "%f", &i) != 1)
      error("! number expected") ;
   return(i) ;
}

struct adobeinfo *
newchar() {
   register struct adobeinfo *ai ;

   ai = (struct adobeinfo *)mymalloc((unsigned long)sizeof(struct adobeinfo)) ;
   ai->adobenum = -1 ;
   ai->texnum = -1 ;
   ai->width = -1 ;
   ai->adobename = NULL ;
   ai->llx = -1 ;
   ai->lly = -1 ;
   ai->urx = -1 ;
   ai->ury = -1 ;
   ai->ligs = NULL ;
   ai->kerns = NULL ;
   ai->pccs = NULL ;
   ai->next = adobechars ;
   adobechars = ai ;
   return(ai) ;
}

struct kern *
newkern() {
   register struct kern *nk ;

   nk = (struct kern *)mymalloc((unsigned long)sizeof(struct kern)) ;
   nk->next = NULL ;
   nk->succ = NULL ;
   nk->delta = 0 ;
   return(nk) ;
}

struct pcc *
newpcc() {
   register struct pcc *np ;

   np = (struct pcc *)mymalloc((unsigned long)sizeof(struct pcc)) ;
   np->next = NULL ;
   np->partname = NULL ;
   np->xoffset = 0 ;
   np->yoffset = 0 ;
   return(np) ;
}

struct lig *
newlig() {
   register struct lig *nl ;

   nl = (struct lig *)mymalloc((unsigned long)sizeof(struct lig)) ;
   nl->next = NULL ;
   nl->succ = NULL ;
   nl->sub = NULL ;
   nl->op = 0 ; /* the default =: op */
   nl->boundleft = 0 ;
   return(nl) ;
}

void
expect(s)
char *s ;
{
   if (strcmp(paramstring(), s) != 0) {
      (void)fprintf(stderr, "%s expected: ", s) ;
      error("! syntax error") ;
   }
}

void
handlechar() { /* an input line beginning with C */
   register struct adobeinfo *ai ;
   register struct lig *nl ;

   ai = newchar() ;
   ai->adobenum = paramnum() ;
   expect(";") ;
   expect("WX") ;
   ai->width = transform(paramnum(),0) ;
   if (ai->adobenum >= 0 && ai->adobenum < 256) {
      adobeptrs[ai->adobenum] = ai ;
   }
   expect(";") ;
   expect("N") ;
   ai->adobename = paramnewstring() ;
   expect(";") ;
   expect("B") ;
   ai->llx = paramnum() ;
   ai->lly = paramnum() ;
   ai->llx = transform(ai->llx, ai->lly) ;
   ai->urx = paramnum() ;
   ai->ury = paramnum() ;
   ai->urx = transform(ai->urx, ai->ury) ;
/* We need to avoid negative heights or depths. They break accents in
   math mode, among other things.  */
   if (ai->lly > 0)
      ai->lly = 0 ;
   if (ai->ury < 0)
      ai->ury = 0 ;
   expect(";") ;
/* Now look for ligatures (which aren't present in fixedpitch fonts) */
   while (*param == 'L' && !fixedpitch) {
      expect("L") ;
      nl = newlig() ;
      nl->succ = paramnewstring() ;
      nl->sub = paramnewstring() ;
      nl->next = ai->ligs ;
      ai->ligs = nl ;
      expect(";") ;
   }
}

struct adobeinfo *
findadobe(p)
char *p ;
{
   register struct adobeinfo *ai ;

   for (ai=adobechars; ai; ai = ai->next)
      if (strcmp(p, ai->adobename)==0)
         return(ai) ;
   return(NULL) ;
}

/*
 *   The following comment no longer applies; we rely on the LIGKERN
 *   entries to kill space kerns.  Also, the same applies to numbers.
 *
 * We ignore kerns before and after space characters, because (1) TeX
 * is using the space only for Polish ligatures, and (2) TeX's
 * boundarychar mechanisms are not oriented to kerns (they apply
 * to both spaces and punctuation) so we don't want to use them.
 */
void
handlekern() { /* an input line beginning with KPX */
   register struct adobeinfo *ai ;
   register char *p ;
   register struct kern *nk ;

   p = paramstring() ;
   ai = findadobe(p) ;
   if (ai == NULL)
      error("kern char not found") ;
   else {
      nk = newkern() ;
      nk->succ = paramnewstring() ;
      nk->delta = transform(paramnum(),0) ;
      nk->next = ai->kerns ;
      ai->kerns = nk ;
    }
}

void
handleconstruct() { /* an input line beginning with CC */
   register struct adobeinfo *ai ;
   register char *p ;
   register struct pcc *np ;
   register int n ;
   struct pcc *npp = NULL;

   p = paramstring() ;
   ai = findadobe(p) ;
   if (ai == NULL)
      error("! composite character name not found") ;
   n = paramnum() ;
   expect(";") ;
   while (n--) {
      if (strcmp(paramstring(),"PCC") != 0) return ;
        /* maybe I should expect("PCC") instead, but I'm playing it safe */
      np = newpcc() ;
      np->partname = paramnewstring() ;
      if (findadobe(np->partname)==NULL) return ;
      np->xoffset = paramnum() ;
      np->yoffset = paramnum() ;
      np->xoffset = transform(np->xoffset, np->yoffset) ;
      if (npp) npp->next = np ;
      else ai->pccs = np ;
      npp = np ;
      expect(";") ;
   }
}

struct encoding *readencoding() ;

void
makeaccentligs() {
   register struct adobeinfo *ai, *aci ;
   register char *p ;
   register struct lig *nl ;
   for (ai=adobechars; ai; ai=ai->next) {
      p = ai->adobename ;
      if (strlen(p)>2)
         if ((aci=findadobe(p+1)) && (aci->adobenum > 127)) {
            nl = newlig() ;
            nl->succ = mymalloc((unsigned long)2) ;
            *(nl->succ + 1) = 0 ;
            *(nl->succ) = *p ;
            nl->sub = ai->adobename ;
            nl->next = aci->ligs ;
            aci->ligs = nl ;
         }
   }
}

void
readadobe() {
   struct adobeinfo *ai ;
#ifdef VMCMS
    int i;
#endif

/*
 *   We allocate a placeholder boundary char.
 */
   ai = newchar() ;
   ai->adobenum = -1 ;
   ai->adobename = "||" ; /* boundary character name */
   while (getline()) {
      switch(interest(paramstring())) {
case FontName:
         fontname = paramnewstring() ;
#ifdef VMCMS
   i=0;
   while(fontname[i] != '\0') {
      fontname[i]=ebcdic2ascii[toupper(fontname[i])];
      i++;
   };
#endif
         break ;
case EncodingScheme:
         codingscheme = paramnewstring() ;
#ifdef VMCMS
   strcpy(ebcodingscheme, codingscheme);
   i=0;
   while(codingscheme[i] != '\0') {
      codingscheme[i]=ebcdic2ascii[toupper(codingscheme[i])];
      i++;
   }
#endif
         break ;
case ItalicAngle:
         italicangle = paramfloat() ;
         break ;
case IsFixedPitch:
         if (*param == 't' || *param == 'T')
            fixedpitch = 1 ;
         else
            fixedpitch = 0 ;
         break ;
case XHeight:
         xheight = paramnum() ;
         break ;
case C:
         handlechar() ;
         break ;
case KPX:
         handlekern() ;
         break ;
case CC:
         handleconstruct() ;
         break ;
default:
         break ;
      }
   }
   fclose(afmin) ;
   afmin = 0 ;
}
/*
 *   Re-encode the adobe font.  Assumes that the header file will
 *   also contain the appropriate instructions!
 */
void
handlereencoding() {
   if (inenname) {
      int i ;
      struct adobeinfo *ai ;
      char *p ;

      ignoreligkern = 1 ;
      inencoding = readencoding(inenname) ;
      for (i=0; i<256; i++)
         if (ai=adobeptrs[i]) {
            ai->adobenum = -1 ;
            adobeptrs[i] = NULL ;
         }
      for (i=0; i<256; i++) {
         p = inencoding->vec[i] ;
         if (p && *p && (ai = findadobe(p))) {
            ai->adobenum = i ;
            adobeptrs[i] = ai ;
         }
      }
      codingscheme = inencoding->name ;
   }
   ignoreligkern = 0 ;
   if (outenname) {
      outencoding = readencoding(outenname) ;
   } else {
      outencoding = readencoding((char *)0) ;
   }
}

/*
 *   This routine reverses a list.  We use it because we accumulate the
 *   adobeinfo list in reverse order, but when we go to map the
 *   characters, we would prefer to use the original ordering.  It just
 *   makes more sense.
 */
struct adobeinfo *revlist(p)
struct adobeinfo *p ;
{
   struct adobeinfo *q = 0, *t ;

   while (p) {
      t = p->next ;
      p->next = q ;
      q = p ;
      p = t ;
   }
   return (void *)q ;
}

void
assignchars() {
   register char **p ;
   register int i, j ;
   register struct adobeinfo *ai ;
   int nextfree = 128 ;

/*
 *   First, we assign all those that match perfectly.
 */
   for (i=0, p=outencoding->vec; i<256; i++, p++)
      if ((ai=findadobe(*p)) && (ai->adobenum >= 0 || ai->pccs != NULL)) {
         if (ai->texnum >= 0)
            nexttex[i] = ai->texnum ; /* linked list */
         ai->texnum = i ;
         texptrs[i] = ai ;
      }
/*
 *   Next, we assign all the others, retaining the adobe positions, possibly
 *   multiply assigning characters.
 */
   for (ai=adobechars; ai; ai=ai->next)
      if (ai->adobenum >= 0 && ai->texnum < 0 && texptrs[ai->adobenum]==0) {
         ai->texnum = ai->adobenum ;
         texptrs[ai->adobenum] = ai ;
      }
/*
 *   Finally, we map all remaining characters into free locations beginning
 *   with 128, if we know how to construct those characters.
 */
   adobechars = revlist(adobechars) ;
   for (ai=adobechars; ai; ai=ai->next)
      if (ai->texnum<0 && (ai->adobenum>=0 || ai->pccs !=NULL)) {
         while (texptrs[nextfree]) {
            nextfree=(nextfree+1)&255 ;
            if (nextfree==128) return ; /* all slots full */
         }
         ai->texnum = nextfree ;
         texptrs[nextfree] = ai ;
      }
/*
 *   Now, if any of the characters are encoded multiple times, we want
 *   ai->texnum to be the first one assigned, since that is most likely
 *   to be the most important one.  So we reverse the above lists.
 */
   for (ai=adobechars; ai; ai=ai->next)
      if (ai->texnum >= 0) {
         j = -1 ;
         while (nexttex[ai->texnum] >= 0) {
            i = nexttex[ai->texnum] ;
            nexttex[ai->texnum] = j ;
            j = ai->texnum ;
            ai->texnum = i ;
         }
         nexttex[ai->texnum] = j ;
      }
}

void
upmap() { /* Compute uppercase mapping, when making a small caps font */
   register struct adobeinfo *ai, *Ai ;
   register char *p, *q ;
   register struct pcc *np, *nq ;
   int i ;
   char lwr[50] ;

   for (Ai=adobechars; Ai; Ai=Ai->next) {
      p = Ai->adobename ;
      if (*p>='A' && *p<='Z') {
         q = lwr ;
         for (; *p; p++)
            *q++ = ((*p>='A' && *p<='Z') ? *p+32 : *p) ;
         *q = 0;
         if (ai=findadobe(lwr)) {
            for (i = ai->texnum; i >= 0; i = nexttex[i])
               uppercase[i] = Ai ;
            for (i = Ai->texnum; i >= 0; i = nexttex[i])
               lowercase[i] = ai ;
         }
      }
   }
/* Note that, contrary to the normal true/false conventions,
 * uppercase[i] is NULL and lowercase[i] is non-NULL when i is the
 * ASCII code of an uppercase letter; and vice versa for lowercase letters */

   if (ai=findadobe("germandbls"))
      if (Ai=findadobe("S")) { /* we also construct SS */
         for (i=ai->texnum; i >= 0; i = nexttex[i])
            uppercase[i] = ai ;
         ai->adobenum = -1 ;
         ai->width = Ai->width << 1 ;
         ai->llx = Ai->llx ;
         ai->lly = Ai->lly ;
         ai->urx = Ai->width + Ai->urx ;
         ai->ury = Ai->ury ;
         ai->kerns = Ai->kerns ;
         np = newpcc() ;
         np->partname = "S" ;
         nq = newpcc() ;
         nq->partname = "S" ;
         nq->xoffset = Ai->width ;
         np->next = nq ;
         ai->pccs = np ;
      }
   if ((ai=findadobe("dotlessi")))
      for (i=ai->texnum; i >= 0; i = nexttex[i])
         uppercase[i] = findadobe("I") ;
   if ((ai=findadobe("dotlessj")))
      for (i=ai->texnum; i >= 0; i = nexttex[i])
         uppercase[i] = findadobe("J") ;
}
/* The logic above seems to work well enough, but it leaves useless characters
 * like `fi' and `fl' in the font if they were present initially,
 * and it omits characters like `dotlessj' if they are absent initially */

/* Now we turn to computing the TFM file */

int lf, lh, nw, nh, nd, ni, nl, nk, ne, np ;

void
write16(what)
register short what ;
{
   (void)fputc(what >> 8, tfmout) ;
   (void)fputc(what & 255, tfmout) ;
}

void
writearr(p, n)
register long *p ;
register int n ;
{
   while (n) {
      write16((short)(*p >> 16)) ;
      write16((short)(*p & 65535)) ;
      p++ ;
      n-- ;
   }
}

void
makebcpl(p, s, n)
register long *p ;
register char *s ;
register int n ;
{
   register long t ;
   register long sc ;

   if (strlen(s) < n)
      n = strlen(s) ;
   t = ((long)n) << 24 ;
   sc = 16 ;
   while (n > 0) {
      t |= ((long)(*(unsigned char *)s++)) << sc ;
      sc -= 8 ;
      if (sc < 0) {
         *p++ = t ;
         t = 0 ;
         sc = 24 ;
      }
      n-- ;
   }
   *p++ = t ;
}

int source[257] ;
int unsort[257] ;

/*
 *   Next we need a routine to reduce the number of distinct dimensions
 *   in a TFM file. Given an array what[0]...what[oldn-1], we want to
 *   group its elements into newn clusters, in such a way that the maximum
 *   difference between elements of a cluster is as small as possible.
 *   Furthermore, what[0]=0, and this value must remain in a cluster by
 *   itself. Data such as `0 4 6 7 9' with newn=3 shows that an iterative
 *   scheme in which 6 is first clustered with 7 will not work. So we
 *   borrow a neat algorithm from METAFONT to find the true optimum.
 *   Memory location what[oldn] is set to 0x7fffffffL for convenience.
 */
long nextd ; /* smallest value that will give a different mincover */
int
mincover(what,d) /* tells how many clusters result, given max difference d */
register long d ;
long *what ;
{
   register int m ;
   register long l ;
   register long *p ;

   nextd = 0x7fffffffL ;
   p = what+1 ;
   m = 1 ;
   while (*p<0x7fffffffL) {
      m++ ;
      l = *p ;
      while (*++p <= l+d) ;
      if (*p-l < nextd) nextd = *p-l ;
   }
   return (m) ;
}

void
remap(what, oldn, newn)
long *what ;
int oldn, newn ;
{
   register int i, j ;
   register long d, l ;

   what[oldn] = 0x7fffffffL ;
   for (i=oldn-1; i>0; i--) {
      d = what[i] ;
      for (j=i; what[j+1]<d; j++) {
         what[j] = what[j+1] ;
         source[j] = source[j+1] ;
      }
      what[j] = d ;
      source[j] = i ;
   } /* Tom, don't let me ever catch you using bubblesort again! -- Don */

   i = mincover(what, 0L) ;
   d = nextd ;
   while (mincover(what,d+d)>newn) d += d ;
   while (mincover(what,d)>newn) d = nextd ;

   i = 1 ;
   j = 0 ;
   while (i<oldn) {
      j++ ;
      l = what[i] ;
      unsort[source[i]] = j ;
      while (what[++i] <= l+d) {
         unsort[source[i]] = j ;
         if (i-j == oldn-newn) d = 0 ;
      }
      what[j] = (l+what[i-1])/2 ;
   }
}

long checksum() {
   int i ;
   long s1 = 0, s2 = 0 ;
   char *p ;
   struct adobeinfo *ai ;

   for (i=0; i<256; i++)
      if (ai=adobeptrs[i]) {
         s1 = (s1 << 1) ^ ai->width ;
         for (p=ai->adobename; *p; p++)
            s2 = (s2 * 3) + *p ;
      }
   s1 = (s1 << 1) ^ s2 ;
   return s1 ;
}

/*
 *   The next routine simply scales something.
 *   Input is in 1000ths of an em.  Output is in FIXFACTORths of 1000.
 */
#define FIXFACTOR (0x100000L) /* 2^{20}, the unit fixnum */
long
scale(what)
long what ;
{
   return(((what / 1000) << 20) +
          (((what % 1000) << 20) + 500) / 1000) ;
}

long *header, *charinfo, *width, *height, *depth, *ligkern, *kern, *tparam,
     *italic ;
long *tfmdata ;

void
buildtfm() {
   register int i, j ;
   register struct adobeinfo *ai ;

   if (fontspace == 0) {
      if (adobeptrs[32])
         fontspace = adobeptrs[32]->width ;
      else
         fontspace = transform(500, 0) ;
   }
   header = tfmdata ;
   cksum = checksum() ;
   header[0] = cksum ;
   header[1] = 0xa00000 ; /* 10pt design size */
   makebcpl(header+2, codingscheme, 39) ;
   makebcpl(header+12, fontname, 19) ;
   lh = 17 ;
   charinfo = header + lh ;

   for (i=0; i<256 && adobeptrs[i]==NULL; i++) ;
   bc = i ;
   for (i=255; i>=0 && adobeptrs[i]==NULL; i--) ;
   ec = i;
   if (ec < bc)
      error("! no Adobe characters") ;

   width = charinfo + (ec - bc + 1) ;
   width[0] = 0 ;
   nw++ ;
   for (i=bc; i<=ec; i++)
      if (ai=adobeptrs[i]) {
         width[nw]=ai->width ;
         for (j=1; width[j]!=ai->width; j++) ;
         ai->wptr = j ;
         if (j==nw)
            nw++ ;
      }
   if (nw>256)
      error("! 256 chars with different widths") ;
   depth = width + nw ;
   depth[0] = 0 ;
   nd = 1 ;
   for (i=bc; i<=ec; i++)
      if (ai=adobeptrs[i]) {
         depth[nd] = -ai->lly ;
         for (j=0; depth[j]!=-ai->lly; j++) ;
         ai->dptr = j ;
         if (j==nd)
            nd++ ;
      }
   if (nd > 16) {
      remap(depth, nd, 16) ;
      nd = 16 ;
      for (i=bc; i<=ec; i++)
         if (ai=adobeptrs[i])
            ai->dptr = unsort[ai->dptr] ;
   }
   height = depth + nd ;
   height[0] = 0 ;
   nh = 1 ;
   for (i=bc; i<=ec; i++)
      if (ai=adobeptrs[i]) {
         height[nh]=ai->ury ;
         for (j=0; height[j]!=ai->ury; j++) ;
         ai->hptr = j ;
         if (j==nh)
            nh++ ;
      }
   if (nh > 16) {
      remap(height, nh, 16) ;
      nh = 16 ;
      for (i=bc; i<=ec; i++)
         if (ai=adobeptrs[i])
            ai->hptr = unsort[ai->hptr] ;
   }
   italic  = height + nh ;
   italic[0] = 0 ;
   ni = 1 ;
   for (i=bc; i<=ec; i++)
      if (ai=adobeptrs[i]) {
         italic[ni] = ai->urx - ai->width ;
         if (italic[ni]<0)
            italic[ni] = 0 ;
         for (j=0; italic[j]!=italic[ni]; j++) ;
         ai->iptr = j ;
         if (j==ni)
            ni++ ;
      }
   if (ni > 64) {
      remap(italic, ni, 64) ;
      ni = 64 ;
      for (i=bc; i<=ec; i++)
         if (ai=adobeptrs[i])
            ai->iptr = unsort[ai->iptr] ;
   }

   for (i=bc; i<=ec; i++)
      if (ai=adobeptrs[i])
         charinfo[i-bc] = ((long)(ai->wptr)<<24) +
                           ((long)(ai->hptr)<<20) +
                            ((long)(ai->dptr)<<16) +
                             ((long)(ai->iptr)<<10) ;

   ligkern = italic + ni ;
   nl = 0 ; /* ligatures and kerns omitted from raw Adobe font */
   kern = ligkern + nl ;
   nk = 0 ;

   newslant = (double)slant - efactor * tan(italicangle*(3.1415926535/180.0)) ;
   tparam = kern + nk ;
   tparam[0] = (long)(FIXFACTOR * newslant + 0.5) ;
   tparam[1] = scale((long)fontspace) ;
   tparam[2] = (fixedpitch ? 0 : scale((long)(300*efactor+0.5))) ;
   tparam[3] = (fixedpitch ? 0 : scale((long)(100*efactor+0.5))) ;
   tparam[4] = scale((long)xheight) ;
   tparam[5] = scale((long)(1000*efactor+0.5)) ;
   np = 6 ;
}

void
writesarr(what, len)
long *what ;
int len ;
{
   register long *p ;
   int i ;

   p = what ;
   i = len ;
   while (i) {
      *p = scale(*p) ;
      (void)scale(*p) ; /* need this kludge for some compilers */
      p++ ;
      i-- ;
   }
   writearr(what, len) ;
}

void
writetfm() {
   lf = 6 + lh + (ec - bc + 1) + nw + nh + nd + ni + nl + nk + ne + np ;
   write16(lf) ;
   write16(lh) ;
   write16(bc) ;
   write16(ec) ;
   write16(nw) ;
   write16(nh) ;
   write16(nd) ;
   write16(ni) ;
   write16(nl) ;
   write16(nk) ;
   write16(ne) ;
   write16(np) ;
   writearr(header, lh) ;
   writearr(charinfo, ec-bc+1) ;
   writesarr(width, nw) ;
   writesarr(height, nh) ;
   writesarr(depth, nd) ;
   writesarr(italic, ni) ;
   writearr(ligkern, nl) ;
   writesarr(kern, nk) ;
   writearr(tparam, np) ;
}

/* OK, the TFM file is done! Now for our next trick, the VPL file. */

/* For TeX we want to compute a character height that works properly
 * with accents. The following list of accents doesn't need to be complete. */
char *accents[] = { "acute", "tilde", "caron", "dieresis", NULL} ;
int
texheight(ai)
register struct adobeinfo *ai ;
{
   register char **p;
   register struct adobeinfo *aci, *acci ;
   if (*(ai->adobename + 1)) return (ai->ury) ; /* that was the simple case */
   for (p=accents; *p; p++)  /* otherwise we look for accented letters */
      if (aci=findadobe(*p)) {
         strcpy(buffer,ai->adobename) ;
         strcat(buffer,*p) ;
         if (acci=findadobe(buffer)) return (acci->ury - aci->ury + xheight) ;
      }
   return (ai->ury) ;
}

/* modified tgr to eliminate varargs problems */

#define vout(s)  fprintf(vplout, s)
int level ; /* the depth of parenthesis nesting in VPL file being written */
void vlevout() {
   register int l = level ;
   while (l--) vout("   ") ;
}
void vlevnlout() {
   vout("\n") ;
   vlevout() ;
}
#define voutln(str) {fprintf(vplout,"%s\n",str);vlevout();}
#define voutln2(f,s) {fprintf(vplout,f,s);vlevnlout();}
#define voutln3(f,a,b) {fprintf(vplout,f,a,b);vlevnlout();}
#define voutln4(f,a,b,c) {fprintf(vplout,f,a,b,c);vlevnlout();}
void
vleft()
{
   level++ ;
   vout("(") ;
}

void
vright()
{
   level-- ;
   voutln(")") ;
}

int forceoctal = 0 ;

char vcharbuf[6] ;
char *vchar(c)
int c ;
{
   if (forceoctal == 0 &&
         ((c>='0' && c<='9')||(c>='A' && c<='Z')||(c>='a' && c<='z')))
      (void) sprintf(vcharbuf,"C %c", c) ;
   else (void) sprintf(vcharbuf,"O %o", (unsigned)c) ;
   return (vcharbuf) ;
}

void
writevpl()
{
   register int i, j, k ;
   register struct adobeinfo *ai ;
   register struct lig *nlig ;
   register struct kern *nkern ;
   register struct pcc *npcc ;
   struct adobeinfo *asucc, *asub, *api ;
   int xoff, yoff, ht ;
   char unlabeled ;

   voutln2("(VTITLE Created by %s)", titlebuf) ;
   voutln("(COMMENT Please edit that VTITLE if you edit this file)") ;
   (void)sprintf(obuffer, "TeX-%s%s%s%s", outname,
      (efactor==1.0? "" : "-E"), (slant==0.0? "" : "-S"),
                 (makevpl==1? "" : "-CSC")) ;
   if (strlen(obuffer)>19) { /* too long, will retain first 9 and last 10 */
      register char *p, *q ;
      for (p = &obuffer[9], q = &obuffer[strlen(obuffer)-10] ; p<&obuffer[19];
              p++, q++) *p = *q ;
      obuffer[19] = '\0' ;
   }
   voutln2("(FAMILY %s)" , obuffer) ;
#ifdef VMCMS
   voutln3("(CODINGSCHEME %s + %s)", outencoding->name, ebcodingscheme) ;
#else
   voutln3("(CODINGSCHEME %s + %s)", outencoding->name, codingscheme) ;
#endif
   voutln("(DESIGNSIZE R 10.0)") ;
   voutln("(DESIGNUNITS R 1000)") ;
   voutln("(COMMENT DESIGNSIZE (1 em) IS IN POINTS)") ;
   voutln("(COMMENT OTHER DIMENSIONS ARE MULTIPLES OF DESIGNSIZE/1000)") ;
   voutln2("(CHECKSUM O %lo)",cksum ^ 0xffffffff) ;
   if (boundarychar >= 0)
      voutln2("(BOUNDARYCHAR O %lo)", (unsigned long)boundarychar) ;
   vleft() ; voutln("FONTDIMEN") ;
   if (newslant)
      voutln2("(SLANT R %f)", newslant) ;
   voutln2("(SPACE D %d)", fontspace) ;
   if (! fixedpitch) {
      voutln2("(STRETCH D %d)", transform(200,0)) ;
      voutln2("(SHRINK D %d)", transform(100,0)) ;
   }
   voutln2("(XHEIGHT D %d)", xheight) ;
   voutln2("(QUAD D %d)", transform(1000,0)) ;
   voutln2("(EXTRASPACE D %d)", fixedpitch ? fontspace : transform(111, 0)) ;
   vright() ;
   vleft() ; voutln("MAPFONT D 0");
   voutln2("(FONTNAME %s)", outname) ;
   voutln2("(FONTCHECKSUM O %lo)", (unsigned long)cksum) ;
   vright() ;
   if (makevpl>1) {
      vleft() ; voutln("MAPFONT D 1");
      voutln2("(FONTNAME %s)", outname) ;
      voutln("(FONTAT D 800)") ;
      voutln2("(FONTCHECKSUM O %lo)", (unsigned long)cksum) ;
      vright() ;
   }

   for (i=0; i<256 && texptrs[i]==NULL; i++) ;
   bc = i ;
   for (i=255; i>=0 && texptrs[i]==NULL; i--) ;
   ec = i;

   vleft() ; voutln("LIGTABLE") ;
   ai = findadobe("||") ;
   unlabeled = 1 ;
   for (nlig=ai->ligs; nlig; nlig=nlig->next)
      if (asucc=findadobe(nlig->succ))
         if (asub=findadobe(nlig->sub))
            if (asucc->texnum>=0)
               if (asub->texnum>=0) {
                  if (unlabeled) {
                     voutln("(LABEL BOUNDARYCHAR)") ;
                     unlabeled = 0 ;
                  }
                  for (j = asucc->texnum; j >= 0; j = nexttex[j]) {
                     voutln4("(%s %s O %o)", vplligops[nlig->op],
                         vchar(j), (unsigned)asub->texnum) ;
                  }
               }
   for (i=bc; i<=ec; i++)
      if ((ai=texptrs[i]) && ai->texnum == i) {
         unlabeled = 1 ;
         if (uppercase[i]==NULL) /* omit ligatures from smallcap lowercase */
            for (nlig=ai->ligs; nlig; nlig=nlig->next)
               if (asucc=findadobe(nlig->succ))
                  if (asub=findadobe(nlig->sub))
                     if (asucc->texnum>=0)
                        if (asub->texnum>=0) {
                           if (unlabeled) {
                              for (j = ai->texnum; j >= 0; j = nexttex[j])
                                 voutln2("(LABEL %s)", vchar(j)) ;
                              unlabeled = 0 ;
                           }
                           for (j = asucc->texnum; j >= 0; j = nexttex[j]) {
                              voutln4("(%s %s O %o)", vplligops[nlig->op],
                                   vchar(j), (unsigned)asub->texnum) ;
                              if (nlig->boundleft)
                                 break ;
                           }
                        }
         for (nkern = (uppercase[i] ? uppercase[i]->kerns : ai->kerns);
                    nkern; nkern=nkern->next)
            if (asucc=findadobe(nkern->succ))
               for (j = asucc->texnum; j >= 0; j = nexttex[j]) {
                  if (uppercase[j]==NULL) {
                     if (unlabeled) {
                        for (k = ai->texnum; k >= 0; k = nexttex[k])
                           voutln2("(LABEL %s)", vchar(k)) ;
                        unlabeled = 0 ;
                     }
                     if (uppercase[i]) {
                        if (lowercase[j]) {
                           for (k=lowercase[j]->texnum; k >= 0; k = nexttex[k])
                              voutln3("(KRN %s R %.1f)", vchar(k),
                                    0.8*nkern->delta) ;
                        } else voutln3("(KRN %s R %.1f)",
                                 vchar(j), 0.8*nkern->delta) ;
                     } else {
                        voutln3("(KRN %s R %d)", vchar(j),
                                nkern->delta) ;
                        if (lowercase[j])
                           for (k=lowercase[j]->texnum; k >= 0; k = nexttex[k])
                              voutln3("(KRN %s R %.1f)", vchar(k),
                                0.8*nkern->delta) ;
                     }
                  }
               }
         if (! unlabeled) voutln("(STOP)") ;
      }
   vright() ;

   for (i=bc; i<=ec; i++)
      if (ai=texptrs[i]) {
         vleft() ; fprintf(vplout, "CHARACTER %s", vchar(i)) ;
         if (*vcharbuf=='C') {
            voutln("") ;
         } else
            voutln2(" (comment %s)", ai->adobename) ;
         if (uppercase[i]) {
            ai=uppercase[i] ;
            voutln2("(CHARWD R %.1f)", 0.8 * (ai->width)) ;
            if (ht=texheight(ai)) voutln2("(CHARHT R %.1f)", 0.8 * ht) ;
            if (ai->lly) voutln2("(CHARDP R %.1f)", -0.8 * ai->lly) ;
            if (ai->urx > ai->width)
               voutln2("(CHARIC R %.1f)", 0.8 * (ai->urx - ai->width)) ;
         } else {
            voutln2("(CHARWD R %d)", ai->width) ;
            if (ht=texheight(ai)) voutln2("(CHARHT R %d)", ht) ;
            if (ai->lly) voutln2("(CHARDP R %d)", -ai->lly) ;
            if (ai->urx > ai->width)
               voutln2("(CHARIC R %d)", ai->urx - ai->width) ;
         }
         if (ai->adobenum != i || uppercase[i]) {
            vleft() ; voutln("MAP") ;
            if (uppercase[i]) voutln("(SELECTFONT D 1)") ;
            if (ai->pccs && ai->adobenum < 0) {
               xoff = 0 ; yoff = 0 ;
               for (npcc = ai->pccs; npcc; npcc=npcc->next)
                  if (api=findadobe(npcc->partname))
                     if (api->texnum>=0) {
                        if (npcc->xoffset != xoff) {
                           if (uppercase[i]) {
                              voutln2("(MOVERIGHT R %.1f)",
                                      0.8 * (npcc->xoffset - xoff)) ;
                           } else voutln2("(MOVERIGHT R %d)",
                                      npcc->xoffset - xoff) ;
                           xoff = npcc->xoffset ;
                        }
                        if (npcc->yoffset != yoff) {
                           if (uppercase[i]) {
                              voutln2("(MOVEUP R %.1f)",
                                      0.8 * (npcc->yoffset - yoff)) ;
                           } else voutln2("(MOVEUP R %d)",
                                      npcc->yoffset - yoff) ;
                           yoff = npcc->yoffset ;
                        }
                        voutln2("(SETCHAR O %o)", (unsigned)api->adobenum) ;
                        xoff += texptrs[api->texnum]->width ;
                     }
            } else voutln2("(SETCHAR O %o)", (unsigned)ai->adobenum) ;
            vright() ;
         }
         vright() ;
      }
   if (level) error("! I forgot to match the parentheses") ;
}

void usage(f)
FILE *f ;
{
   (void)fprintf(f, 
 "afm2tfm 7.0, Copyright 1990-92 by Radical Eye Software\n") ;
   (void)fprintf(f,
 "Usage: afm2tfm foo[.afm] [-O] [-v|-V bar[.vpl]]\n") ;
   (void)fprintf(f,
 "                 [-p|-t|-T encodingfile] [foo[.tfm]]\n") ;
}

void
openfiles(argc, argv)
int argc ;
char *argv[] ;
{
   register int lastext ;
   register int i ;
   int arginc ;
   extern void exit() ;

   tfmout = (FILE *)NULL ;
   if (argc == 1) {
      usage(stdout) ;
      exit(0) ;
   }

#ifdef MSDOS
   /* Make VPL file identical to that created under Unix */
   (void)sprintf(titlebuf, "afm2tfm %s", argv[1]) ;
#else
   (void)sprintf(titlebuf, "%s %s", argv[0], argv[1]) ;
#endif
   (void)strcpy(inname, argv[1]) ;
   lastext = -1 ;
   for (i=0; inname[i]; i++)
      if (inname[i] == '.')
         lastext = i ;
      else if (inname[i] == '/' || inname[i] == ':')
         lastext = -1 ;
   if (lastext == -1) (void)strcat(inname, ".afm") ;

   while (argc>2 && *argv[2]=='-') {
      arginc = 2 ;
      i = argv[2][1] ;
      if (i == '/')
         i = argv[2][2] - 32 ; /* /a ==> A for VMS */
      switch (i) {
case 'V': makevpl++ ;
case 'v': makevpl++ ;
         (void)strcpy(outname, argv[3]) ;
         lastext = -1 ;
         for (i=0; outname[i]; i++)
            if (outname[i] == '.')
               lastext = i ;
            else if (outname[i] == '/' || outname[i] == ':')
               lastext = -1 ;
         if (lastext == -1) (void)strcat(outname, ".vpl") ;
#ifdef VMCMS
         if ((vplout=fopen(outname, "w"))==NULL)
#else
         if ((vplout=fopen(outname, WRITEBIN))==NULL)
#endif
            error("! can't open vpl output file") ;
         break ;
case 'e': if (sscanf(argv[3], "%f", &efactor)==0 || efactor<0.01)
            error("! Bad extension factor") ;
         efactorparam = argv[3] ;
         break ;
case 's': if (sscanf(argv[3], "%f", &slant)==0)
            error("! Bad slant parameter") ;
         slantparam = argv[3] ;
         break ;
case 'P':
case 'p':
         inenname = argv[3] ;
         break ;
case 'T':
         inenname = outenname = argv[3] ;
         break ;
case 't':
         outenname = argv[3] ;
         break ;
case 'O':
         forceoctal = 1 ;
         arginc = 1 ;
         break ;
default: (void)fprintf(stderr, "Unknown option %s %s will be ignored.\n",
                         argv[2], argv[3]) ;
      }
      for (i=0; i<arginc; i++) {
         (void)sprintf(titlebuf + strlen(titlebuf), " %s", argv[2]) ;
         argv++ ;
         argc-- ;
      }
   }

   if ((afmin=fopen(inname, "r"))==NULL)
      error("! can't open afm input file") ;

   if (argc>3 || (argc==3 && *argv[2]=='-')) {
      usage(stderr) ;
      error("! incorrect usage") ;
   }

   if (argc == 2) (void)strcpy(outname, inname) ;
   else (void)strcpy(outname, argv[2]) ;

   lastext = -1 ;
   for (i=0; outname[i]; i++)
      if (outname[i] == '.')
         lastext = i ;
      else if (outname[i] == '/' || outname[i] == ':' || outname[i] == '\\')
         lastext = -1 ;
   if (argc == 2) {
      outname[lastext] = 0 ;
      lastext = -1 ;
   }
   if (lastext == -1) {
      lastext = strlen(outname) ;
      (void)strcat(outname, ".tfm") ;
   }
   if (tfmout == NULL && (tfmout=fopen(outname, WRITEBIN))==NULL)
      error("! can't open tfm output file") ;
   outname[lastext] = 0 ;
/*
 *   Now we strip off any directory information, so we only use the
 *   base name in the vf file.  We accept any of /, :, or \ as directory
 *   delimiters, so none of these are available for use inside the
 *   base name; this shouldn't be a problem.
 */
   for (i=0, lastext=0; outname[i]; i++)
      if (outname[i] == '/' || outname[i] == ':' || outname[i] == '\\')
         lastext = i + 1 ;
   if (lastext)
      strcpy(outname, outname + lastext) ;
}
/*
 *   Some routines to remove kerns that match certain patterns.
 */
struct kern *rmkernmatch(k, s)
struct kern *k ;
char *s ;
{
   struct kern *nk ;

   while (k && strcmp(k->succ, s)==0)
      k = k->next ;
   if (k) {
      for (nk = k; nk; nk = nk->next)
         while (nk->next && strcmp(nk->next->succ, s)==0)
            nk->next = nk->next->next ;
   }
   return k ;
}
/*
 *   Recursive to one level.
 */
void rmkern(s1, s2, ai)
char *s1, *s2 ;
struct adobeinfo *ai ;
{
   if (ai == 0) {
      if (strcmp(s1, "*") == 0) {
         for (ai=adobechars; ai; ai = ai->next)
            rmkern(s1, s2, ai) ;
         return ;
      } else {
         ai = findadobe(s1) ;
         if (ai == 0)
            return ;
      }
   }
   if (strcmp(s2, "*")==0)
      ai->kerns = 0 ; /* drop them on the floor */
   else
      ai->kerns = rmkernmatch(ai->kerns, s2) ;
}
int sawligkern ;
/*
 *   Reads a ligkern line, if this is one.  Assumes the first character
 *   passed is `%'.
 */
void checkligkern(s)
char *s ;
{
   char *oparam = param ;
   char *mlist[5] ;
   int n ;

   s++ ;
   while (*s && *s <= ' ')
      s++ ;
   if (strncmp(s, "LIGKERN", 7)==0) {
      sawligkern = 1 ;
      s += 7 ;
      while (*s && *s <= ' ')
         s++ ;
      param = s ;
      while (*param) {
         for (n=0; n<5;) {
            if (*param == 0)
               break ;
            mlist[n] = paramstring() ;
            if (strcmp(mlist[n], ";") == 0)
               break ;
            n++ ;
         }
         if (n > 4)
            error("! too many parameters in lig kern data") ;
         if (n < 3)
            error("! too few parameters in lig kern data") ;
         if (n == 3 && strcmp(mlist[1], "{}") == 0) { /* rmkern command */
            rmkern(mlist[0], mlist[2], (struct adobeinfo *)0) ;
         } else if (n == 3 && strcmp(mlist[0], "||") == 0 &&
                              strcmp(mlist[1], "=") == 0) { /* bc command */
            struct adobeinfo *ai = findadobe("||") ;

            if (boundarychar != -1)
               error("! multiple boundary character commands?") ;
            if (sscanf(mlist[2], "%d", &n) != 1)
               error("! expected number assignment for boundary char") ;
            if (n < 0 || n > 255)
               error("! boundary character number must be 0..255") ;
            boundarychar = n ;
            if (ai == 0)
               error("! internal error: boundary char") ;
            ai->texnum = n ; /* prime the pump, so to speak, for lig/kerns */
         } else if (n == 4) {
            int op = -1 ;
            struct adobeinfo *ai ;

            for (n=0; encligops[n]; n++)
               if (strcmp(mlist[2], encligops[n])==0) {
                  op = n ;
                  break ;
               }
            if (op < 0)
               error("! bad ligature op specified") ;
            if (ai = findadobe(mlist[0])) {
               struct lig *lig ;

               if (findadobe(mlist[2]))     /* remove coincident kerns */
                  rmkern(mlist[0], mlist[1], ai) ;
               if (strcmp(mlist[3], "||") == 0)
                  error("! you can't lig to the boundary character!") ;
               if (! fixedpitch) { /* fixed pitch fonts get *0* ligs */
                  for (lig=ai->ligs; lig; lig = lig->next)
                     if (strcmp(lig->succ, mlist[1]) == 0)
                        break ; /* we'll re-use this structure */
                  if (lig == 0) {
                     lig = newlig() ;
                     lig->succ = newstring(mlist[1]) ;
                     lig->next = ai->ligs ;
                     ai->ligs = lig ;
                  }
                  lig->sub = newstring(mlist[3]) ;
                  lig->op = op ;
                  if (strcmp(mlist[1], "||")==0) {
                     lig->boundleft = 1 ;
                     if (strcmp(mlist[0], "||")==0)
                        error("! you can't lig boundarychar boundarychar!") ;
                  } else
                     lig->boundleft = 0 ;
               }
            }
         } else
            error("! bad form in LIGKERN command") ;
      }
   }
   param = oparam ;
}
/*
 *   Here we get a token from the AFM file.  We parse just as much PostScript
 *   as we expect to find in an encoding file.  We allow commented lines and
 *   names like 0, .notdef, _foo_.  We do not allow //abc.
 */
char smbuffer[100] ;    /* for tokens */
char *gettoken() {
   char *p, *q ;

   while (1) {
      while (param == 0 || *param == 0) {
         if (getline() == 0)
            error("! premature end in encoding file") ;
         for (p=buffer; *p; p++)
            if (*p == '%') {
               if (ignoreligkern == 0)
                  checkligkern(p) ;
               *p = 0 ;
               break ;
            }
      }
      while (*param && *param <= ' ')
         param++ ;
      if (*param) {
         if (*param == '[' || *param == ']' ||
             *param == '{' || *param == '}') {
            smbuffer[0] = *param++ ;
            smbuffer[1] = 0 ;
            return smbuffer ;
         } else if (*param == '/' || *param == '-' || *param == '_' ||
                    *param == '.' ||
                    ('0' <= *param && *param <= '9') ||
                    ('a' <= *param && *param <= 'z') ||
                    ('A' <= *param && *param <= 'Z')) {
            smbuffer[0] = *param ;
            for (p=param+1, q=smbuffer+1;
                        *p == '-' || *p == '_' || *p == '.' ||
                        ('0' <= *p && *p <= '9') ||
                        ('a' <= *p && *p <= 'z') ||
                        ('A' <= *p && *p <= 'Z'); p++, q++)
               *q = *p ;
            *q = 0 ;
            param = p ;
            return smbuffer ;
         }
      }
   }
}
void getligkerndefaults() {
   int i ;

   for (i=0; staticligkern[i]; i++) {
      strcpy(buffer, staticligkern[i]) ;
      strcpy(obuffer, staticligkern[i]) ;
      param = buffer ;
      checkligkern(buffer) ;
   }
}
/*
 *   This routine reads in an encoding file, given the name.  It returns
 *   the final total structure.  It performs a number of consistency checks.
 */
struct encoding *readencoding(enc)
char *enc ;
{
   char *p ;
   int i ;
   struct encoding *e =
      (struct encoding *)mymalloc((unsigned long)sizeof(struct encoding)) ;

   sawligkern = 0 ;
   if (afmin)
      error("! oops; internal afmin error") ;
   if (enc) {
      afmin = fopen(enc, "r") ;
      param = 0 ;
      if (afmin == 0)
         error("! couldn't open that encoding file") ;
      p = gettoken() ;
      if (*p != '/' || p[1] == 0)
         error("! first token in encoding must be literal encoding name") ;
      e->name = newstring(p+1) ;
      p = gettoken() ;
      if (strcmp(p, "["))
         error("! second token in encoding must be mark ([) token") ;
      for (i=0; i<256; i++) {
         p = gettoken() ;
         if (*p != '/' || p[1] == 0)
            error("! tokens 3 to 257 in encoding must be literal names") ;
         e->vec[i] = newstring(p+1) ;
      }
      p = gettoken() ;
      if (strcmp(p, "]"))
         error("! token 258 in encoding must be make-array (])") ;
      while (getline()) {
         for (p=buffer; *p; p++)
            if (*p == '%') {
               if (ignoreligkern == 0)
                  checkligkern(p) ;
               *p = 0 ;
               break ;
            }
      }
      fclose(afmin) ;
      afmin = 0 ;
      if (ignoreligkern == 0 && sawligkern == 0)
         getligkerndefaults() ;
   } else {
      e = &staticencoding ;
      getligkerndefaults() ;
   }
   param = 0 ;
   return e ;
}
/*
 *   This routine prints out the line that needs to be added to psfonts.map.
 */
void conspsfonts() {
   (void)printf("%s %s", outname, fontname) ;
   if (slantparam || efactorparam || inenname) {
      (void)printf(" \"") ;
      if (slantparam)
         (void)printf(" %s SlantFont", slantparam) ;
      if (efactorparam)
         (void)printf(" %s ExtendFont", efactorparam) ;
      if (inenname)
         (void)printf(" %s ReEncodeFont", inencoding->name) ;
      (void)printf(" \"") ;
      if (inenname)
         (void)printf(" <%s", inenname) ;
   }
   (void)printf("\n") ;
}
#ifndef VMS
void
#endif
main(argc, argv)
int argc ;
char *argv[] ;
{
   int i ;
   extern void exit() ;

   for (i=0; i<256; i++)
      nexttex[i] = -1 ; /* encoding chains have length 0 */
   tfmdata = (long *)mymalloc((unsigned long)40000L) ;
   openfiles(argc, argv) ;
   readadobe() ;
   handlereencoding() ;
   buildtfm() ;
   writetfm() ;
   conspsfonts() ;
   if (makevpl) {
      assignchars() ;
      if (makevpl>1) upmap() ;
      writevpl() ;
   }
   exit(0) ;
   /*NOTREACHED*/
}
