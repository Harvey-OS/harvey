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
 *   11/3/92: more corrections to VM/CMS port. Now it looks correct
 *   and will be supported by J. Hafner.
 *
 */
/*
 *   More changes, primarily from Karl Berry, enough for a new version
 *   number to 8.0; 1 December 1996.  Note that this version computes
 *   checksums differently (more intelligently).
 */

#ifdef KPATHSEA
#include "config.h"
#include <kpathsea/c-ctype.h>
#include <kpathsea/progname.h>
#else /* ! KPATHSEA */
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#if defined(SYSV) || defined(VMS) || defined(__THINK__) || defined(MSDOS) || defined(OS2) || defined(ATARIST) || defined(WIN32)
#include <string.h>
#else
#include <strings.h>
#endif
#include <math.h>
#ifdef ATARIST
#include <float.h>
#endif
#endif /* KPATHSEA */

/* JLH: added these to make the code easier to read and remove some
   ascii<->ebcdic dependencies */
#define ASCII_A 65
#define ASCII_Z 90
#define ASCII_a 97
#define ASCII_z 122
#define ASCII_0 48
#define ASCII_9 57

#ifdef VMCMS
#define interesting lookstr  /* for 8 character truncation conflicts */
#include "dvipscms.h"
extern FILE *cmsfopen() ;
extern char ebcdic2ascii[] ;
extern char ascii2ebcdic[] ;
#ifdef fopen
#undef fopen
#endif
#define fopen cmsfopen
#endif

#include "dvips.h"
/* debug.h redefines fopen to my_real_fopen */
#ifdef fopen
#undef fopen
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
   "% LIGKERN space {} * ; * {} space ; zero {} * ; * {} zero ;",
   "% LIGKERN one {} * ; * {} one ; two {} * ; * {} two ;",
   "% LIGKERN three {} * ; * {} three ; four {} * ; * {} four ;",
   "% LIGKERN five {} * ; * {} five ; six {} * ; * {} six ;",
   "% LIGKERN seven {} * ; * {} seven ; eight {} * ; * {} eight ;",
   "% LIGKERN nine {} * ; * {} nine ;",
/* Kern accented characters the same way as their base. */
  "% LIGKERN Aacute <> A ; aacute <> a ;",
  "% LIGKERN Acircumflex <> A ; acircumflex <> a ;",
  "% LIGKERN Adieresis <> A ; adieresis <> a ;",
  "% LIGKERN Agrave <> A ; agrave <> a ;",
  "% LIGKERN Aring <> A ; aring <> a ;",
  "% LIGKERN Atilde <> A ; atilde <> a ;",
  "% LIGKERN Ccedilla <> C ; ccedilla <> c ;",
  "% LIGKERN Eacute <> E ; eacute <> e ;",
  "% LIGKERN Ecircumflex <> E ; ecircumflex <> e ;",
  "% LIGKERN Edieresis <> E ; edieresis <> e ;",
  "% LIGKERN Egrave <> E ; egrave <> e ;",
  "% LIGKERN Iacute <> I ; iacute <> i ;",
  "% LIGKERN Icircumflex <> I ; icircumflex <> i ;",
  "% LIGKERN Idieresis <> I ; idieresis <> i ;",
  "% LIGKERN Igrave <> I ; igrave <> i ;",
  "% LIGKERN Ntilde <> N ; ntilde <> n ;",
  "% LIGKERN Oacute <> O ; oacute <> o ;",
  "% LIGKERN Ocircumflex <> O ; ocircumflex <> o ;",
  "% LIGKERN Odieresis <> O ; odieresis <> o ;",
  "% LIGKERN Ograve <> O ; ograve <> o ;",
  "% LIGKERN Oslash <> O ; oslash <> o ;",
  "% LIGKERN Otilde <> O ; otilde <> o ;",
  "% LIGKERN Scaron <> S ; scaron <> s ;",
  "% LIGKERN Uacute <> U ; uacute <> u ;",
  "% LIGKERN Ucircumflex <> U ; ucircumflex <> u ;",
  "% LIGKERN Udieresis <> U ; udieresis <> u ;",
  "% LIGKERN Ugrave <> U ; ugrave <> u ;",
  "% LIGKERN Yacute <> Y ; yacute <> y ;",
  "% LIGKERN Ydieresis <> Y ; ydieresis <> y ;",
  "% LIGKERN Zcaron <> Z ; zcaron <> z ;",
/*
 *   These next are only included for deficient afm files that
 *   have the lig characters but not the lig commands.
 */
   "% LIGKERN f i =: fi ; f l =: fl ; f f =: ff ; ff i =: ffi ;",
   "% LIGKERN ff l =: ffl ;",
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
   struct adobeptr *kern_equivs ;
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
struct adobeptr {
   struct adobeptr *next;
   struct adobeinfo *ch;
};
struct pcc {
   struct pcc *next ;
   char * partname ;
   int xoffset, yoffset ;
} ;

FILE *afmin, *vplout, *tfmout ;
char inname[200], outname[200] ; /* names of input and output files */
char tmpstr[200] ; /* a buffer for one string */
char buffer[255]; /* input buffer (modified while parsing) */
char obuffer[255] ; /* unmodified copy of input buffer */
char *param ; /* current position in input buffer */
char *fontname = "Unknown" ;
char *codingscheme = "Unspecified" ;
#ifdef VMCMS
char *ebfontname ;
char *ebcodingscheme ;
#endif
float italicangle = 0.0 ;
char fixedpitch ;
char makevpl ;
char pedantic ;
int xheight = 400 ;
int fontspace ;
int bc, ec ;
long cksum ;
float efactor = 1.0, slant = 0.0 ;
float capheight = 0.8 ;
char *efactorparam, *slantparam ;
double newslant ;
char titlebuf[500] ;

void
error P1C(register char *, s)
{
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
transform P2C(register int, x, register int, y)
{
   register double acc ;
   acc = efactor * x + slant *y ;
   return (int)(acc>=0? floor(acc+0.5) : ceil(acc-0.5) ) ;
}

int
getline P1H(void) {
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
interest P1C(char *, s)
{
   register char **p ;
   register int n ;

   for (p=interesting, n=0; *p; p++, n++)
      if (strcmp(s, *p)==0)
         return(n) ;
   return(NONE) ;
}

char *
mymalloc P1C(unsigned long, len)
{
   register char *p ;
   int i ;

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
newstring P1C(char *, s)
{
   char *q = mymalloc((unsigned long)(strlen(s) + 1)) ;
   (void)strcpy(q, s) ;
   return q ;
}

char *
paramnewstring P1H(void) {
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
paramstring P1H(void) {
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
paramnum P1H(void) {
   register char *p ;
   int i ;

   p = paramstring() ;
   if (sscanf(p, "%d", &i) != 1)
      error("! integer expected") ;
   return(i) ;
}

float
paramfloat P1H(void) {
   register char *p ;
   float i ;

   p = paramstring() ;
   if (sscanf(p, "%f", &i) != 1)
      error("! number expected") ;
   return(i) ;
}

struct adobeinfo *
newchar P1H(void) {
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
   ai->kern_equivs = NULL ;
   ai->pccs = NULL ;
   ai->next = adobechars ;
   adobechars = ai ;
   return(ai) ;
}

struct kern *
newkern P1H(void) {
   register struct kern *nk ;

   nk = (struct kern *)mymalloc((unsigned long)sizeof(struct kern)) ;
   nk->next = NULL ;
   nk->succ = NULL ;
   nk->delta = 0 ;
   return(nk) ;
}

struct pcc *
newpcc P1H(void) {
   register struct pcc *np ;

   np = (struct pcc *)mymalloc((unsigned long)sizeof(struct pcc)) ;
   np->next = NULL ;
   np->partname = NULL ;
   np->xoffset = 0 ;
   np->yoffset = 0 ;
   return(np) ;
}

struct lig *
newlig P1H(void) {
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
expect P1C(char *, s)
{
   if (strcmp(paramstring(), s) != 0) {
      (void)fprintf(stderr, "%s expected: ", s) ;
      error("! syntax error") ;
   }
}

void
handlechar P1H(void) { /* an input line beginning with C */
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
findadobe P1C(char *, p)
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
handlekern P1H(void) { /* an input line beginning with KPX */
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
handleconstruct P1H(void) { /* an input line beginning with CC */
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
makeaccentligs P1H(void) {
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
readadobe P1H(void) {
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
/* fontname comes in as ebcdic but we need it asciified for tfm file
   so we save it in ebfontname and change it in fontname */
   ebfontname = newstring(fontname) ;
   i=0;
   while(fontname[i] != '\0') {
      fontname[i]=ebcdic2ascii[fontname[i]];
      i++;
   };
#endif
         break ;
case EncodingScheme:
         codingscheme = paramnewstring() ;
#ifdef VMCMS
/* for codingscheme, we do the same as we did for fontname */
   ebcodingscheme = newstring(codingscheme) ;
   i=0;
   while(codingscheme[i] != '\0') {
      codingscheme[i]=ebcdic2ascii[codingscheme[i]];
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
handlereencoding P1H(void) {
   if (inenname) {
      int i ;
      struct adobeinfo *ai ;
      char *p ;

      ignoreligkern = 1 ;
      inencoding = readencoding(inenname) ;
      for (i=0; i<256; i++)
         if (0 != (ai=adobeptrs[i])) {
            ai->adobenum = -1 ;
            adobeptrs[i] = NULL ;
         }
      for (i=0; i<256; i++) {
         p = inencoding->vec[i] ;
         if (p && *p && 0 != (ai = findadobe(p))) {
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
   return q ;
}

void
assignchars P1H(void) {
   register char **p ;
   register int i, j ;
   register struct adobeinfo *ai, *pai ;
   int nextfree = 128 ;
   struct pcc *pcp ;

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
   if (pedantic)
      return ;
/*
 *   Next, we assign all the others, retaining the adobe positions, possibly
 *   multiply assigning characters. Unless the output encoding was
 *   precisely specified.
 */
   for (ai=adobechars; ai; ai=ai->next)
      if (ai->adobenum >= 0 && ai->adobenum <256
          && ai->texnum < 0 && texptrs[ai->adobenum] == 0) {
         ai->texnum = ai->adobenum ;
         texptrs[ai->adobenum] = ai ;
      }
/*
 *   Finally, we map all remaining characters into free locations beginning
 *   with 128, if we know how to construct those characters.  We need to
 *   make sure the component pieces are mapped.
 */
   adobechars = revlist(adobechars) ;
   for (ai=adobechars; ai; ai=ai->next)
      if (ai->texnum<0 && (ai->adobenum>=0 || ai->pccs != NULL)) {
         while (texptrs[nextfree]) {
            nextfree=(nextfree+1)&255 ;
            if (nextfree==128) goto finishup ; /* all slots full */
         }
         ai->texnum = nextfree ;
         texptrs[nextfree] = ai ;
      }
finishup:
/*
 *   We now check all of the composite characters.  If any of the
 *   components are not mapped, we unmap the composite character.
 */
   for (i=0; i<256; i++) {
      ai = texptrs[i] ;
      if (ai && ai->pccs != NULL && ai->texnum >= 0) {
         for (pcp = ai->pccs; pcp; pcp = pcp->next) {
            pai = findadobe(pcp->partname) ;
            if (pai == NULL || pai->texnum < 0) {
               texptrs[ai->texnum] = 0 ;
               ai->texnum = -1 ;
               break ;
            }
         }
      }
   }
/*
 *   Now, if any of the characters are encoded multiple times, we want
 *   ai->texnum to be the first one assigned, since that is most likely
 *   to be the most important one.  So we reverse the above lists.
 */
   for (ai=adobechars; ai; ai=ai->next)
      if (ai->texnum >= 0 && ai->texnum < 256) {
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
upmap P1H(void) { /* Compute uppercase mapping, when making a small caps font */
   register struct adobeinfo *ai, *Ai ;
   register char *p, *q ;
   register struct pcc *np, *nq ;
   int i ;
   char lwr[50] ;

/* JLH: changed some lines below to be ascii<->ebcdic independent
   any reason we don't use 'isupper'?.
   Looks like we should use isupper to me --karl.  */
   for (Ai=adobechars; Ai; Ai=Ai->next) {
      p = Ai->adobename ;
      if (isupper (*p)) {
         q = lwr ;
         for (; *p; p++)
            *q++ = TOLOWER (*p);
         *q = '\0';   /* changed this too! */

         if (0 != (ai=findadobe(lwr))) {
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

   if (0 != (ai=findadobe("germandbls")))
      if (0 != (Ai=findadobe("S"))) { /* we also construct SS */
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
write16 P1C(register short, what)
{
   (void)fputc(what >> 8, tfmout) ;
   (void)fputc(what & 255, tfmout) ;
}

void
writearr P2C(register long *, p, register int, n)
{
   while (n) {
      write16((short)(*p >> 16)) ;
      write16((short)(*p & 65535)) ;
      p++ ;
      n-- ;
   }
}

void
makebcpl P3C(register long *, p, register char *, s, register int, n)
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
mincover P2C(long *, what, 
	     register long, d) /* tells how many clusters result, given max difference d */
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
remap P3C(long *, what, int, oldn, int, newn)
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

long
checksum P1H(void) {
   int i ;
   unsigned long s1 = 0, s2 = 0 ;
   char *p ;
   struct adobeinfo *ai ;

   for (i=0; i<256; i++)
      if (0 != (ai=adobeptrs[i])) {
         s1 = ((s1 << 1) ^ (s1>>31)) ^ ai->width ; /* cyclic left shift */
         s1 &= 0xffffffff; /* in case we're on a 64-bit machine */
         for (p=ai->adobename; *p; p++)
#ifndef VMCMS
            s2 = (s2 * 3) + *p ;
#else
            s2 = (s2 * 3) + ebcdic2ascii[*p] ;
#endif
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
scale P1C(long, what)
{
   return(((what / 1000) << 20) +
          (((what % 1000) << 20) + 500) / 1000) ;
}

long *header, *charinfo, *width, *height, *depth, *ligkern, *kern, *tparam,
     *italic ;
long *tfmdata ;

void
buildtfm P1H(void) {
   register int i, j ;
   register struct adobeinfo *ai ;

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
      if (0 != (ai=adobeptrs[i])) {
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
      if (0 != (ai=adobeptrs[i])) {
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
         if (0 != (ai=adobeptrs[i]))
            ai->dptr = unsort[ai->dptr] ;
   }
   height = depth + nd ;
   height[0] = 0 ;
   nh = 1 ;
   for (i=bc; i<=ec; i++)
      if (0 != (ai=adobeptrs[i])) {
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
         if (0 != (ai=adobeptrs[i]))
            ai->hptr = unsort[ai->hptr] ;
   }
   italic  = height + nh ;
   italic[0] = 0 ;
   ni = 1 ;
   for (i=bc; i<=ec; i++)
      if (0 != (ai=adobeptrs[i])) {
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
         if (0 != (ai=adobeptrs[i]))
            ai->iptr = unsort[ai->iptr] ;
   }

   for (i=bc; i<=ec; i++)
      if (0 != (ai=adobeptrs[i]))
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
writesarr P2C(long *, what, int, len)
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
writetfm P1H(void) {
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
/*
 *   We only do this if the xheight has a reasonable value.
 *   (>50)
 */
char *accents[] = { "acute", "tilde", "caron", "dieresis", NULL} ;
int
texheight P1C(register struct adobeinfo *, ai)
{
   register char **p;
   register struct adobeinfo *aci, *acci ;
   if (xheight <= 50 || *(ai->adobename + 1)) return (ai->ury) ;
                                           /* that was the simple case */
   for (p=accents; *p; p++)  /* otherwise we look for accented letters */
      if (0 != (aci=findadobe(*p))) {
         strcpy(buffer,ai->adobename) ;
         strcat(buffer,*p) ;
         if (0 != (acci=findadobe(buffer)))
            return (acci->ury - aci->ury + xheight) ;
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
vleft P1H(void)
{
   level++ ;
   vout("(") ;
}

void
vright P1H(void)
{
   level-- ;
   voutln(")") ;
}

int forceoctal = 0 ;

char vcharbuf[6] ;
char *vchar(c)
int c ;
{
   if (forceoctal == 0 && ISALNUM (c))
      (void) sprintf(vcharbuf,"C %c",
#ifndef VMCMS
      c) ;
#else
      ascii2ebcdic[c]) ;
#endif
   else (void) sprintf(vcharbuf,"O %o", (unsigned)c) ;
   return (vcharbuf) ;
}

char vnamebuf[100];
char *
vname P1C(int, c)
{
  if (!forceoctal && ISALNUM (c)) {
    vnamebuf[0] = 0;
  } else if (c >= 0 && c < 256) {
    sprintf (vnamebuf, " (comment %s)", texptrs[c]->adobename);
  }
  return vnamebuf;
}

void
writevpl P1H(void)
{
   register int i, j, k ;
   register struct adobeinfo *ai ;
   register struct lig *nlig ;
   register struct kern *nkern ;
   register struct pcc *npcc ;
   struct adobeinfo *asucc, *asub, *api ;
   struct adobeptr *kern_eq;
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
   {
      char tbuf[300] ;
      char *base_encoding = 
#ifndef VMCMS
         codingscheme ;
#else
         ebcodingscheme ;
#endif
      
      if (strcmp (outencoding->name, base_encoding) == 0) {
        sprintf(tbuf, "%s", outencoding->name);
      } else {
        sprintf(tbuf, "%s + %s", base_encoding, outencoding->name);
      }
      
      if (strlen(tbuf) > 39) {
         error("Coding scheme too long; shortening to 39 characters.") ;
         tbuf[39] = 0 ;
      }
      voutln2("(CODINGSCHEME %s)", tbuf) ;
   }
   voutln("(DESIGNSIZE R 10.0)") ;
   voutln("(DESIGNUNITS R 1000)") ;
   voutln("(COMMENT DESIGNSIZE (1 em) IS IN POINTS)") ;
   voutln("(COMMENT OTHER DIMENSIONS ARE MULTIPLES OF DESIGNSIZE/1000)") ;
   /* Let vptovf compute the checksum. */
   /* voutln2("(CHECKSUM O %lo)",cksum ^ 0xffffffff) ; */
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
   /* voutln2("(FONTCHECKSUM O %lo)", (unsigned long)cksum) ; */
   vright() ;
   if (makevpl>1) {
      vleft() ; voutln("MAPFONT D 1");
      voutln2("(FONTNAME %s)", outname) ;
      voutln2("(FONTAT D %d)", (int)(1000.0*capheight+0.5)) ;
      /* voutln2("(FONTCHECKSUM O %lo)", (unsigned long)cksum) ; */
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
      if (0 != (asucc=findadobe(nlig->succ))) {
         if (0 != (asub=findadobe(nlig->sub)))
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
       }
   if (! unlabeled) voutln("(STOP)") ;
   for (i=bc; i<=ec; i++)
      if ((ai=texptrs[i]) && ai->texnum == i) {
         unlabeled = 1 ;
         if (uppercase[i]==NULL) /* omit ligatures from smallcap lowercase */
            for (nlig=ai->ligs; nlig; nlig=nlig->next)
               if (0 != (asucc=findadobe(nlig->succ)))
                  if (0 != (asub=findadobe(nlig->sub)))
                     if (asucc->texnum>=0)
                        if (asub->texnum>=0) {
                           if (unlabeled) {
                              for (j = ai->texnum; j >= 0; j = nexttex[j])
                                 voutln3("(LABEL %s)%s", vchar(j), vname(j)) ;
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
            if (0 != (asucc=findadobe(nkern->succ)))
               for (j = asucc->texnum; j >= 0; j = nexttex[j]) {
                  if (uppercase[j]==NULL) {
                     if (unlabeled) {
                        for (k = ai->texnum; k >= 0; k = nexttex[k])
                           voutln3("(LABEL %s)%s", vchar(k), vname(k)) ;
                        unlabeled = 0 ;
                     }
                     /* If other characters have the same kerns as this
                        one, output the label here.  This makes the TFM
                        file much smaller than if we output all the
                        kerns again under a different label.  */
                     for (kern_eq = ai->kern_equivs; kern_eq;
                          kern_eq = kern_eq->next) {
                        k = kern_eq->ch->texnum;
                        if (k >= 0 && k < 256)
                          voutln3("(LABEL %s)%s", vchar(k), vname(k)) ;
                     }
                     ai->kern_equivs = 0; /* Only output those labels once. */
                     if (uppercase[i]) {
                        if (lowercase[j]) {
                           for (k=lowercase[j]->texnum; k >= 0; k = nexttex[k])
                              voutln4("(KRN %s R %.1f)%s", vchar(k),
                                    capheight*nkern->delta, vname(k)) ;
                        } else voutln4("(KRN %s R %.1f)%s",
                                 vchar(j), capheight*nkern->delta, vname(j)) ;
                     } else {
                        voutln4("(KRN %s R %d)%s", vchar(j),
                                nkern->delta, vname(j)) ;
                        if (lowercase[j])
                           for (k=lowercase[j]->texnum; k >= 0; k = nexttex[k])
                              voutln4("(KRN %s R %.1f)%s", vchar(k),
                                capheight*nkern->delta, vname(k)) ;
                     }
                  }
               }
         if (! unlabeled) voutln("(STOP)") ;
      }
   vright() ;

   for (i=bc; i<=ec; i++)
      if (0 != (ai=texptrs[i])) {
         vleft() ; fprintf(vplout, "CHARACTER %s", vchar(i)) ;
         if (*vcharbuf=='C') {
            voutln("") ;
         } else
            voutln2(" (comment %s)", ai->adobename) ;
         if (uppercase[i]) {
            ai=uppercase[i] ;
            voutln2("(CHARWD R %.1f)", capheight * (ai->width)) ;
            if (0 != (ht=texheight(ai)))
               voutln2("(CHARHT R %.1f)", capheight * ht) ;
            if (ai->lly)
               voutln2("(CHARDP R %.1f)", -capheight * ai->lly) ;
            if (ai->urx > ai->width)
               voutln2("(CHARIC R %.1f)", capheight * (ai->urx - ai->width)) ;
         } else {
            voutln2("(CHARWD R %d)", ai->width) ;
            if (0 != (ht=texheight(ai)))
               voutln2("(CHARHT R %d)", ht) ;
            if (ai->lly)
               voutln2("(CHARDP R %d)", -ai->lly) ;
            if (ai->urx > ai->width)
               voutln2("(CHARIC R %d)", ai->urx - ai->width) ;
         }
         if (ai->adobenum != i || uppercase[i]) {
            vleft() ; voutln("MAP") ;
            if (uppercase[i]) voutln("(SELECTFONT D 1)") ;
            if (ai->pccs && ai->adobenum < 0) {
               xoff = 0 ; yoff = 0 ;
               for (npcc = ai->pccs; npcc; npcc=npcc->next)
                  if (0 != (api=findadobe(npcc->partname)))
                     if (api->texnum>=0) {
                        if (npcc->xoffset != xoff) {
                           if (uppercase[i]) {
                              voutln2("(MOVERIGHT R %.1f)",
                                      capheight * (npcc->xoffset - xoff)) ;
                           } else voutln2("(MOVERIGHT R %d)",
                                      npcc->xoffset - xoff) ;
                           xoff = npcc->xoffset ;
                        }
                        if (npcc->yoffset != yoff) {
                           if (uppercase[i]) {
                              voutln2("(MOVEUP R %.1f)",
                                      capheight * (npcc->yoffset - yoff)) ;
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

#ifdef KPATHSEA
void version P1C(FILE *, f)
{
  extern DllImport char *kpathsea_version_string;
  fputs ("afm2tfm(k) (dvips(k) 5.78) 8.1\n", f);
  fprintf (f, "%s\n", kpathsea_version_string);
  fputs ("Copyright (C) 1998 Radical Eye Software.\n\
There is NO warranty.  You may redistribute this software\n\
under the terms of the GNU General Public License\n\
and the Dvips copyright.\n\
For more information about these matters, see the files\n\
named COPYING and afm2tfm.c.\n\
Primary author of afm2tfm: T. Rokicki; -k maintainer: K. Berry.\n", f);
}

#define USAGE "\
  Convert an Adobe font metric file to TeX font metric format.\n\
\n\
-c REAL             use REAL for height of small caps made with -V [0.8]\n\
-e REAL             widen (extend) characters by a factor of REAL\n\
-O                  use octal for all character codes in the vpl file\n\
-p ENCFILE          read/download ENCFILE for the PostScript encoding\n\
-s REAL             oblique (slant) characters by REAL, generally <<1\n\
-t ENCFILE          read ENCFILE for the encoding of the vpl file\n\
-T ENCFILE          equivalent to -p ENCFILE -t ENCFILE\n\
-u                  output only characters from encodings, nothing extra\n\
-v FILE[.vpl]       make a VPL file for conversion to VF\n\
-V SCFILE[.vpl]     like -v, but synthesize smallcaps as lowercase\n\
--help              print this message and exit.\n\
--version           print version number and exit.\n\
"

void usage P1C(FILE *, f)
{
   extern DllImport char *kpse_bug_address;
   
   fputs ("Usage: afm2tfm FILE[.afm] [OPTION]... [FILE[.tfm]]\n", f);
   fputs (USAGE, f);
   putc ('\n', f);
   fputs (kpse_bug_address, f);
}
#else /* ! KPATHSEA */
void usage P1C(FILE *, f)
{
   (void)fprintf(f,
 "afm2tfm 8.1, Copyright 1990-97 by Radical Eye Software\n") ;
   (void)fprintf(f,
 "Usage: afm2tfm foo[.afm] [-O] [-u] [-v|-V bar[.vpl]]\n") ;
   (void)fprintf(f,
 "                 [-e expansion] [-s slant] [-c capheight]\n") ;
   (void)fprintf(f,
 "                 [-p|-t|-T encodingfile] [foo[.tfm]]\n") ;
}
#endif

void
openfiles P2C(int, argc, char **, argv)
{
   register int lastext ;
   register int i ;
   char *p ;
   int arginc ;

   tfmout = (FILE *)NULL ;

   if (argc == 1) {
      usage(stdout) ;
      exit(0) ;
   }

#if defined(MSDOS) || defined(OS2) || defined(ATARIST)
   /* Make VPL file identical to that created under Unix */
   (void)sprintf(titlebuf, "afm2tfm %s", argv[1]) ;
#else
#ifdef VMCMS
   /* Make VPL file identical to that created under Unix */
   (void)sprintf(titlebuf, "afm2tfm %s", argv[1]) ;
#else
   (void)sprintf(titlebuf, "%s %s", argv[0], argv[1]) ;
#endif
#endif
   (void)strcpy(inname, argv[1]) ;
#ifdef KPATHSEA
   if (find_suffix(inname) == NULL)
       (void)strcat(inname, ".afm") ;
#else
   lastext = -1 ;
   for (i=0; inname[i]; i++)
      if (inname[i] == '.')
         lastext = i ;
      else if (inname[i] == '/' || inname[i] == ':')
         lastext = -1 ;
   if (lastext == -1) (void)strcat(inname, ".afm") ;
#endif
   while (argc>2 && *argv[2]=='-') {
      arginc = 2 ;
      i = argv[2][1] ;
      if (i == '/')
         i = argv[2][2] - 32 ; /* /a ==> A for VMS */
      switch (i) {
case 'V': makevpl++ ;
case 'v': makevpl++ ;
         (void)strcpy(outname, argv[3]) ;
#ifdef KPATHSEA
	 if (find_suffix(outname) == NULL)
	    (void)strcat(outname, ".vpl") ;
#else
         lastext = -1 ;
         for (i=0; outname[i]; i++)
            if (outname[i] == '.')
               lastext = i ;
            else if (outname[i] == '/' || outname[i] == ':')
               lastext = -1 ;
         if (lastext == -1) (void)strcat(outname, ".vpl") ;
#endif
#ifndef VMCMS
#ifndef ATARIST
         if ((vplout=fopen(outname, WRITEBIN))==NULL)
#else
         if ((vplout=fopen(outname, "w"))==NULL)
#endif
#else
         if ((vplout=fopen(outname, "w"))==NULL)
#endif
            error("! can't open vpl output file") ;
         break ;
case 'e': if (sscanf(argv[3], "%f", &efactor)==0 || efactor<0.01)
            error("! Bad extension factor") ;
         efactorparam = argv[3] ;
         break ;
case 'c':
         if (sscanf(argv[3], "%f", &capheight)==0 || capheight<0.01)
            error("! Bad small caps height") ;
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
case 'u':
         pedantic = 1 ;
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

#ifdef KPATHSEA
   afmin = kpse_open_file (inname, kpse_afm_format);
#else /* ! KPATHSEA */
   if ((afmin=fopen(inname, "r"))==NULL)
      error("! can't open afm input file") ;
#endif /* KPATHSEA */

   if (argc>3 || (argc==3 && *argv[2]=='-')) {
     error("! need at most two non-option arguments") ;
     usage(stderr) ;
   }

   if (argc == 2) (void)strcpy(outname, inname) ;
   else (void)strcpy(outname, argv[2]) ;

#ifdef KPATHSEA
   if ((p = find_suffix(outname)) != NULL)
      *(p-1) = 0;
   (void)strcat(outname, ".tfm") ;
   if (tfmout == NULL && (tfmout=fopen(outname, WRITEBIN))==NULL)
      error("! can't open tfm output file") ;
/*
 *   Now we strip off any directory information, so we only use the
 *   base name in the vf file.
 */
   if (p == NULL)
     p = find_suffix(outname);
   *(p-1) = 0;

   p = (char *)basename(outname) ;
   strcpy(tmpstr, p);        /* be careful, p and outname are overlapping */
   strcpy(outname, tmpstr);
#else
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
#endif
}
/*
 *   Some routines to remove kerns that match certain patterns.
 */
struct kern *rmkernmatch P2C(struct kern *, k, char *, s)
{
   struct kern *nkern ;

   while (k && strcmp(k->succ, s)==0)
      k = k->next ;
   if (k) {
      for (nkern = k; nkern; nkern = nkern->next)
         while (nkern->next && strcmp(nkern->next->succ, s)==0)
            nkern->next = nkern->next->next ;
   }
   return k ;
}
/*
 *   Recursive to one level.
 */
void rmkern P3C(char *, s1, char *, s2, struct adobeinfo *, ai)
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

/* Make the kerning for character S1 equivalent to that for S2.
   If either S1 or S2 do not exist, do nothing.
   If S1 already has kerning, do nothing.  */
void
addkern P2C(char *, s1, char *, s2)
{
  struct adobeinfo *ai1 = findadobe (s1);
  struct adobeinfo *ai2 = findadobe (s2);
  if (ai1 && ai2 && !ai1->kerns) {
    /* Put the new one at the head of the list, since order is immaterial.  */
    struct adobeptr *ap 
      = (struct adobeptr *) mymalloc((unsigned long)sizeof(struct adobeptr));
    ap->next = ai2->kern_equivs;
    ap->ch = ai1;
    ai2->kern_equivs = ap;
  }
}
int sawligkern ;
/*
 *   Reads a ligkern line, if this is one.  Assumes the first character
 *   passed is `%'.
 */
void checkligkern P1C(char *, s)
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
         } else if (n == 3 && strcmp(mlist[1], "<>") == 0) { /* addkern */
            addkern(mlist[0], mlist[2]) ;
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
            if (0 != (ai = findadobe(mlist[0]))) {
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
struct encoding *readencoding P1C(char *, enc)
{
   char *p ;
   int i ;
   struct encoding *e =
      (struct encoding *)mymalloc((unsigned long)sizeof(struct encoding)) ;

   sawligkern = 0 ;
   if (afmin)
      error("! oops; internal afmin error") ;
   if (enc) {
#ifdef KPATHSEA
     afmin = kpse_open_file(enc, kpse_tex_ps_header_format);
#else
      afmin = fopen(enc, "r") ;
#endif
      param = 0 ;
      if (afmin == 0)
#ifdef KPATHSEA
         FATAL1 ("couldn't open encoding file `%s'", enc) ;
#else
         error("! couldn't open that encoding file") ;
#endif
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
   (void)printf("%s %s", outname,
#ifndef VMCMS
   fontname) ;
#else /* VM/CMS: fontname is ascii, so we use ebfontname */
   ebfontname) ;
#endif
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
int
#endif
main P2C(int, argc, char **, argv)
{
   int i ;

#ifdef KPATHSEA
   kpse_set_progname (argv[0]);

   if (argc == 1) {
      fputs ("afm2tfm: Need at least one file argument.\n", stderr);
      fputs ("Try `afm2tfm --help' for more information.\n", stderr);
      exit(1);
   }     
   if (argc == 2) {
      if (strcmp (argv[1], "--help") == 0) {
        usage (stdout);
        exit (0);
      } else if (strcmp (argv[1], "--version") == 0) {
        version (stdout);
        exit (0);
      }
   }
#endif /* KPATHSEA */
   for (i=0; i<256; i++)
      nexttex[i] = -1 ; /* encoding chains have length 0 */
   tfmdata = (long *)mymalloc((unsigned long)40000L) ;
   openfiles(argc, argv) ;
   readadobe() ;
   if (fontspace == 0) {
      struct adobeinfo *ai ;

      if (0 != (ai = findadobe("space")))
         fontspace = ai->width ;
      else if (adobeptrs[32])
         fontspace = adobeptrs[32]->width ;
      else
         fontspace = transform(500, 0) ;
   }
   handlereencoding() ;
   buildtfm() ;
   writetfm() ;
   conspsfonts() ;
   if (makevpl) {
      assignchars() ;
      if (makevpl>1) upmap() ;
      writevpl() ;
   }
   return 0 ;
   /*NOTREACHED*/
}

