/*
 *   Code to scale dimensions.  Takes two thirty-two bit integers, multiplies
 *   them, divides them by 2^20, and returns the thirty-two bit result.
 *   The first integer, the width in FIXes, can lie between -2^24 and 2^24-1.
 *   The second integer, the scale factor, can lie between 0 and 2^27-1.  The
 *   arithmetic must be exact.  The answer is truncated to an integer.
 *
 *   Since this math is special, we put it in its own file.  It is the only
 *   place in the program where such accuracy is required.
 */
#include "structures.h" /* The copyright notice in that file is included too! */

integer
scalewidth(a, b)
        register integer a, b ;
{
  register integer al, bl ;

  if (a < 0)
     return -scalewidth(-a, b) ;
  if (b < 0)
     return -scalewidth(a, -b) ;
  al = a & 32767 ;
  bl = b & 32767 ;
  a >>= 15 ;
  b >>= 15 ;
  return ( ((al*bl/32768) + a*bl+al*b)/32 + a*b*1024) ;
}
