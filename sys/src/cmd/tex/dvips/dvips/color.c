/*  
 *  This is a set of routines for dvips that are used to process color 
 *  commands in the TeX file (passed by \special commands).  This was
 *  orignally written by J. Hafner, E. Blanz and M. Flickner of IBM
 *  Research, Almaden Research Center.  Contact hafner@almaden.ibm.com.
 *  And then it was completely rewritten by Tomas Rokicki to:
 *
 *      - Be easier on the memory allocator (malloc/free)
 *      - Use less memory overall (by a great deal) and be faster
 *      - Work with the -C, -a, and other options
 *      - Be more adaptable to other drivers and previewers.
 *
 *  The motivating idea:  we want to be able to process 5,000 page
 *  documents using lots of colors on each page on a 640K IBM PC
 *  relatively quickly.
 */
#include "structures.h" /* The copyright notice in that file is included too! */
#include <stdio.h>
/*
 *   Externals we use.
 */
extern void cmdout() ;
extern integer pagenum ;
extern FILE *dvifile ;
/*
 *   Here we set some limits on some color stuff.
 */
#define COLORHASH (89)
#define MAXCOLORLEN (120)     /* maximum color length for background */
#define TOTALCOLORLEN (3000)  /* sum of lengths of pending colors */
/*
 *   This is where we store the color information for a particular page.
 *   If we free all of these, we free all of the allocated color
 *   stuff; we do no allocations (but one) other than when we allocate
 *   these.
 */
static struct colorpage {
   struct colorpage *next ;
   integer boploc ; /* we use the bop loc as a page indicator */
   char *bg ;
   char colordat[2] ;
} *colorhash[COLORHASH] ;
static char cstack[TOTALCOLORLEN], *csp, *cend, *bg ;
/*
 *   This routine sends a color command out.  If the command is a
 *   single `word' or starts with a double quote, we send it out
 *   exactly as is (except for any double quote.)  If the command
 *   is a word followed by arguments, we send out the arguments and
 *   then the word prefixed by "TeXcolor".
 */
void colorcmdout(s)
char *s ;
{
   char *p ;
   char tempword[100] ;

   while (*s <= ' ')
      s++ ;
   if (*s == '"') {
      cmdout(s+1) ;
      return ;
   }
   for (p=s; *p && *p > ' '; p++) ;
   for (; *p && *p <= ' '; p++) ;
   if (*p == 0) {
      cmdout(s) ;
      return ;
   }
   cmdout(p) ;
   strcpy(tempword, "TeXcolor") ;
   for (p=tempword + strlen(tempword); *s && *s > ' '; p++, s++)
      *p = *s ;
   *p = 0 ;
   cmdout(tempword) ;
   return ;
}
/*
 *   For a new dvi file, call this.  Frees all allocated memory.
 */
#define DEFAULTCOLOR "Black"
void initcolor() {
   int i ;
   struct colorpage *p, *q ;

   for (i=0; i<COLORHASH; i++) {
      for (p=colorhash[i]; p; p = q) {
         q = p->next ;
         free(p) ;
      }
      colorhash[i] = 0 ;
   }
   strcpy(cstack, "\n") ;
   strcat(cstack, DEFAULTCOLOR) ;
   csp = cstack + strlen(cstack) ;
   cend = cstack + TOTALCOLORLEN - 3 ; /* for conservativeness */
   bg = 0 ;
}
/*
 * This takes a call from predospecial to set the background color for
 * the current page.  It is saved in stackdepth and backed down the
 * stack during popcolors.
 */
void
background(bkgrnd)
char *bkgrnd ;
{
   if (bkgrnd && *bkgrnd) {
      if (strlen(bkgrnd) > MAXCOLORLEN)
         error(" color name too long; ignored") ;
      else
         strcpy(bg, bkgrnd) ;
   }
}
/*
 * This routine puts a call from \special for color on the colorstack
 * and sets the color in the PostScript.
 */
void
pushcolor(p,outtops)
char *p ;
Boolean outtops ;
{
   if (strlen(p) + csp > cend)
      error("! out of color stack space") ;
   *csp++ = '\n' ;
   strcpy(csp, p) ;
   csp += strlen(p) ;
   if (outtops) {
      colorcmdout(p) ;
   }
}
/*
 * This routine removes a color from the colorstack and resets the color
 * in the PostScript to the previous color.
 */
void
popcolor(outtops)
Boolean outtops ;
{
   char *p = csp - 1 ;

   while (p >= cstack && *p != '\n')
      p-- ;
   if (p == cstack)
      return ;  /* We don't pop the last color as that is global */
   *p = 0 ;
   csp = p ;
   for (p--; p >= cstack && *p != '\n'; p--) ;
   p++ ;
   if ( outtops ) {
      colorcmdout(p) ;
   }
}
/*
 * This routine clears the colorstack, pushes a new color onto the stack
 * (this is now the root or global color).
 */
void
resetcolorstack(p,outtops)
char *p ;
int outtops ;
{
   char *q = csp - 1 ;

   while (q > cstack && *q != '\n')
      q-- ;
   if (q > cstack && outtops == 0) {
#ifdef SHORTINT
     (void)fprintf(stderr, "You've mistakenly made a global color change ") ;
     (void)fprintf(stderr, "to %s within nested colors\n", p) ;
     (void)fprintf(stderr, "on page %ld. Will try to recover.\n", pagenum) ;
#else   /* ~SHORTINT */
     (void)fprintf(stderr, "You've mistakenly made a global color change ") ;
     (void)fprintf(stderr, "to %s within nested colors\n", p) ;
     (void)fprintf(stderr, "on page %d. Will try to recover.\n", pagenum) ;
#endif  /* ~SHORTINT */
   }
   csp = cstack ;
   *csp = 0 ;
   pushcolor(p, outtops) ;
}
/*
 *   This routine is a bit magic.  It looks up the page in the current
 *   hash table.  If the page is already entered in the hash table, then
 *   it restores the color to what that page had, and sets the last
 *   color.  This will occur if this is not the first time that this
 *   page has been encountered.
 *
 *   If, on the other hand, this is a subsequent time that the page has
 *   been encountered, then it will create a new hash entry and copy the
 *   current color information into it.  Since we can only encounter a
 *   new page after having just finished scanning the previous page,
 *   this is safe.
 */
void
bopcolor(outtops)
int outtops ;
{
   integer pageloc = ftell(dvifile) ;
   int h = pageloc % COLORHASH ;
   struct colorpage *p = colorhash[h] ;

   while (p) {
      if (p->boploc == pageloc)
         break ;
      else
         p = p->next ;
   }
   if (p) {
      strcpy(cstack, p->colordat) ;
      csp = cstack + strlen(cstack) ;
      bg = p->bg ;
      if (outtops && strcmp(bg, "White")!=0 && bg[0]) {
         cmdout("gsave") ;
         colorcmdout(bg) ;
         cmdout("clippath fill grestore") ;
      }
   } else {
      p = (struct colorpage *)mymalloc((integer)
                  (strlen(cstack) + sizeof(struct colorpage) + MAXCOLORLEN)) ;
      p->next = colorhash[h] ;
      p->boploc = pageloc ;
      strcpy(p->colordat, cstack) ;
      p->bg = p->colordat + strlen(cstack) + 1 ;
      if (bg)
         strcpy(p->bg, bg) ;
      else
         *(p->bg) = 0 ;
      bg = p->bg ;
      colorhash[h] = p ;
   }
   if (outtops) {
      char *p = csp - 1 ;
      while (p >= cstack && *p != '\n')
         p-- ;
      p++ ;
      if (strcmp(p, DEFAULTCOLOR)!=0) {
         colorcmdout(p) ;
      }
   }
}
