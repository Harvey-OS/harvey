/*
 *  Code to output PostScript commands for one section of the document.
 */
#include "structures.h" /* The copyright notice in that file is included too! */
/*
 *   These are the external routines we call.
 */
extern void dopage() ;
extern void download() ;
extern integer signedquad() ;
extern void skipover() ;
extern void cmdout() ;
extern void numout() ;
extern void newline() ;
extern void setup() ;
extern void skipover() ;
extern int skipnop() ;
/*
 *   These are the external variables we access.
 */
extern shalfword linepos ;
extern FILE *dvifile ;
extern FILE *bitfile ;
extern integer pagenum ;
extern long bytesleft ;
extern quarterword *raster ;
extern int quiet ;
extern Boolean reverse, multiplesects, disablecomments ;
extern int actualdpi ;
extern int vactualdpi ;
extern int prettycolumn ;
extern integer hpapersize, vpapersize ;
extern integer pagecopies ;
static int psfont ;
extern integer mag ;
/*
 *   Now we have the main procedure.
 */
void
dosection(s, c)
        sectiontype *s ;
        int c ;
{
   charusetype *cu ;
   integer prevptr ;
   int np ;
   int k ;
   integer thispage = 0 ;
   extern void cleanres() ;

   if (multiplesects) {
      setup() ;
   } else {
      cmdout("TeXDict") ;
      cmdout("begin") ;
   }
   numout(hpapersize) ;
   numout(vpapersize) ;
   numout(mag) ;
   numout((integer)DPI) ;
   numout((integer)VDPI) ;
   cmdout("@start") ;
   if (multiplesects)
      cmdout("bos") ;
/*
 *   We insure raster is even-word aligned, because download might want that.
 */
   if (bytesleft & 1) {
      bytesleft-- ;
      raster++ ;
   }
   cleanres() ;
   cu = (charusetype *) (s + 1) ;
   psfont = 1 ;
   while (cu->fd) {
      if (cu->psfused)
         cu->fd->psflag = EXISTS ;
      download(cu++, psfont++) ;
   }
   if (! multiplesects) {
      cmdout("end") ;
      setup() ;
   }
   for (cu=(charusetype *)(s+1); cu->fd; cu++)
      cu->fd->psflag = 0 ;
   while (c > 0) {
      c-- ;
      prevptr = s->bos ;
      if (! reverse)
         (void)fseek(dvifile, (long)prevptr, 0) ;
      np = s->numpages ;
      while (np-- != 0) {
         if (reverse)
            (void)fseek(dvifile, (long)prevptr, 0) ;
         pagenum = signedquad() ;
/*
 *   We want to take the base 10 log of the number.  It's probably
 *   small, so we do it quick.
 */
         if (! quiet) {
            int t = pagenum, i = 0 ;
            if (t < 0) {
               t = -t ;
               i++ ;
            }
            do {
               i++ ;
               t /= 10 ;
            } while (t > 0) ;
            if (pagecopies < 20)
               i += pagecopies - 1 ;
            if (i + prettycolumn > STDOUTSIZE) {
               fprintf(stderr, "\n") ;
               prettycolumn = 0 ;
            }
            prettycolumn += i + 1 ;
#ifdef SHORTINT
            (void)fprintf(stderr, "[%ld", pagenum) ;
#else  /* ~SHORTINT */
            (void)fprintf(stderr, "[%d", pagenum) ;
#endif /* ~SHORTINT */
            (void)fflush(stderr) ;
         }
         skipover(36) ;
         prevptr = signedquad()+1 ;
         for (k=0; k<pagecopies; k++) {
            if (k == 0) {
               if (pagecopies > 1)
                  thispage = ftell(dvifile) ;
            } else {
               (void)fseek(dvifile, (long)thispage, 0) ;
               if (prettycolumn + 1 > STDOUTSIZE) {
                  (void)fprintf(stderr, "\n") ;
                  prettycolumn = 0 ;
               }
               (void)fprintf(stderr, ".") ;
               (void)fflush(stderr) ;
               prettycolumn++ ;
            }
            dopage() ;
         }
         if (! quiet) {
            (void)fprintf(stderr, "] ") ;
            (void)fflush(stderr) ;
            prettycolumn += 2 ;
         }
         if (! reverse)
            (void)skipnop() ;
      }
   }
   if (! multiplesects && ! disablecomments) {
      newline() ;
      (void)fprintf(bitfile, "%%%%Trailer\n") ;
   }
   if (multiplesects) {
      if (! disablecomments) {
         newline() ;
         (void)fprintf(bitfile, "%%DVIPSSectionTrailer\n") ;
      }
      cmdout("eos") ;
   }
   cmdout("end") ;
   if (multiplesects && ! disablecomments) {
      newline() ;
      (void)fprintf(bitfile, "%%DVIPSEndSection\n") ;
      linepos = 0 ;
   }
}
