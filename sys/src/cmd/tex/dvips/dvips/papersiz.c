/*
 *   This function calculates approximately (whole + num/den) * sf.
 *   No need for real extreme accuracy; one twenty thousandth of an
 *   inch should be sufficient.
 *
 *   No `sf' parameter means to use an old one; inches are assumed
 *   originally.
 *
 *   Assumptions:
 *
 *      0 <= num < den <= 20000
 *      0 <= whole
 */
void error() ;
static long scale(whole, num, den, sf)
long whole, num, den, sf ;
{
   long v ;

   v = whole * sf + num * (sf / den) ;
   if (v / sf != whole || v < 0 || v > 0x40000000L)
      error("! arithmetic overflow in parameter") ;
   sf = sf % den ;
   v += (sf * num * 2 + den) / (2 * den) ;
   return(v) ;
}
/*
 *   Convert a sequence of digits into a long; return -1 if no digits.
 *   Advance the passed pointer as well.
 */
static long myatol(s)
char **s ;
{
   register char *p ;
   register long result ;

   result = 0 ;
   p = *s ;
   while ('0' <= *p && *p <= '9') {
      if (result > 100000000)
         error("! arithmetic overflow in parameter") ;
      result = 10 * result + *p++ - '0' ;
   }
   if (p == *s) {
      error("expected number!  returning 10") ;
      return 10 ;
   } else {
      *s = p ;
      return(result) ;
   }
}
/*
 *   Get a dimension, allowing all the various extensions, and
 *   defaults.  Returns a value in scaled points.
 */
static long scalevals[] = { 1864680L, 65536L, 786432L, 186468L,
                            1L, 65782L, 70124L, 841489L, 4736286L } ;
static char *scalenames = "cmptpcmmspbpddccin" ;
long myatodim(s)
char **s ;
{
   register long w, num, den, sc ;
   register char *q ;
   char *p ;
   int negative = 0, i ;

   p = *s ;
   if (**s == '-') {
      p++ ;
      negative = 1 ;
   }
   w = myatol(&p) ;
   if (w < 0) {
      error("number too large; 1000 used") ;
      w = 1000 ;
   }
   num = 0 ;
   den = 1 ;
   if (*p == '.') {
      p++ ;
      while ('0' <= *p && *p <= '9') {
         if (den <= 1000) {
            den *= 10 ;
            num = num * 10 + *p - '0' ;
         } else if (den == 10000) {
            den *= 2 ;
            num = num * 2 + (*p - '0') / 5 ;
         }
         p++ ;
      }
   }
   while (*p == ' ')
      p++ ;
   for (i=0, q=scalenames; ; i++, q += 2)
      if (*q == 0) {
         error("expected units!  assuming inches.") ;
         sc = scalevals[8] ;
         break ;
      } else if (*p == *q && p[1] == q[1]) {
         sc = scalevals[i] ;
         p += 2 ;
         break ;
      }
   w = scale(w, num, den, sc) ;
   *s = p ;
   return(negative?-w:w) ;
}
/*
 *   The routine where we handle the paper size special.  We need to pass in
 *   the string after the `papersize=' specification.
 */
void handlepapersize(p, x, y)
char *p ;
long *x, *y ;
{ 
   while (*p == ' ')
      p++ ;
   *x = myatodim(&p) ;
   while (*p == ' ' || *p == ',')
      p++ ;
   *y = myatodim(&p) ;
}
