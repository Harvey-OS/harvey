/*
 *   This software is Copyright 1988 by Radical Eye Software.
 */
/*
 *   dpicheck of dvips.c.  Checks the dots per inch to make sure
 *   it exists.  The fonts which exist are DPI at the appropriate
 *   magnifications.
 */
#include "structures.h"
extern int actualdpi ;
static integer existsizes[30] = { 0 , 99999999 } ;
static int checkdpi ;
/*
 *   This routine adds some sizes to `existsizes'.
 */
void addsiz(rhsize)
int rhsize ;
{
   register integer *p ;
   long t, hsize ;

   hsize = rhsize ;
   for (p=existsizes; *p < hsize; p++) ;
   if (*p == hsize)
      return ;
   do {
      t = *p ;
      *p++ = hsize ;
      hsize = t ;
   } while (hsize) ;
}
void adddpi(hsize)
int hsize ;
{
   long a, b, c ;
   int i ;

   addsiz(hsize) ;
   addsiz((int)((hsize * 116161L + 53020L) / 106040L)) ;
   a = hsize ;
   b = 0 ;
   c = 1 ;
   for (i=0; i<9; i++) {
      b = 6 * b + (a % 5) * c ;
      a = a + a / 5 ;
      c = c * 5 ;
      if (b > c) {
         b -= c ;
         a++ ;
      }
      if (b + b >= c)
         addsiz((int)(a+1)) ;
      else
         addsiz((int)a) ;
   }
}
/*
 *   Finally, the routine that checks a size for match.
 */
halfword
dpicheck(dpi)
halfword dpi ;
{
   integer i ;
   integer margin = 1 + dpi / 500 ;

   if (! checkdpi) {
      adddpi(DPI) ;
      checkdpi = 1 ;
   }
   for (i=0; existsizes[i] < dpi; i++) ;
   if (existsizes[i]-dpi <= margin)
      return(existsizes[i]) ;
   else if (dpi-existsizes[i-1] <= margin)
      return(existsizes[i-1]) ;
   else
      return(dpi) ;
}
