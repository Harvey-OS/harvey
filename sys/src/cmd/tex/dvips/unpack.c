/*
 *   Unpacks the raster data from the packed buffer.  This code was 
 *   translated from pktopx.web using an automatic translator, then
 *   converted for this purpose.  This little routine can be very useful
 *   in other drivers as well.
 */
#include "dvips.h" /* The copyright notice in that file is included too! */

/*
 * external procedures
 */
extern void error();

/*
 *   Some statics for use here.
 */
static halfword bitweight ; 
static halfword dynf ;
static halfword gpower[17] = { 0 , 1 , 3 , 7 , 15 , 31 , 63 , 127 ,
     255 , 511 , 1023 , 2047 , 4095 , 8191 , 16383 , 32767 , 65535 } ; 
static long repeatcount ;
static quarterword *p ;

/*
 *   We need procedures to get a nybble, bit, and packed word from the
 *   packed data structure.
 */

shalfword
getnyb ()
{
    if ( bitweight == 0 ) 
    { bitweight = 16 ; 
      return(*p++ & 15) ;
    } else {
      bitweight = 0 ;
      return(*p >> 4) ;
    }
} 

Boolean
getbit ()
{
    bitweight >>= 1 ; 
    if ( bitweight == 0 ) 
    { p++ ;
      bitweight = 128 ;
    } 
    return(*p & bitweight) ;
} 

long pkpackednum () {
    register halfword i;
    register long j ; 
    i = getnyb () ; 
    if ( i == 0 ) {
       do { j = getnyb () ; 
          i++ ; 
          } while ( j == 0 ) ; 
       while ( i != 0 ) {
          j = j * 16 + ((long) getnyb ()) ; 
          i-- ; 
          } 
       return ( j - 15 + ( 13 - dynf ) * 16 + dynf ) ; 
    }
    else if ( i <= dynf ) return ( i ) ; 
    else if ( i < 14 ) return ( ( i - dynf - 1 )*16 + getnyb() + dynf + 1 ) ; 
    else {
       if (repeatcount != 0)
          error("! recursive repeat count in pk file") ;
       repeatcount = 1 ;
       if ( i == 14 ) repeatcount = pkpackednum () ; 
       return ( pkpackednum() ) ;
    } 
} 

void flip(s, howmany)
register char *s ;
register long howmany ;
{
   register char t ;

   while (howmany > 0) {
      t = *s ;
      *s = s[1] ;
      s[1] = t ;
      howmany-- ;
      s += 2 ;
   }
}
/*
 *   And now we have our main routine.
 */
static halfword bftest = 1 ;
long
unpack(pack, raster, cwidth, cheight, cmd)
        quarterword *pack ;
        halfword *raster ;
        halfword cwidth, cheight, cmd ;
{ 
  register integer i, j ; 
  shalfword wordwidth ; 
  register halfword word, wordweight ;
  shalfword rowsleft ; 
  Boolean turnon ; 
  shalfword hbit, ww ; 
  long count ; 
  halfword *oraster ;

      oraster = raster ;
      p = pack ;
      dynf = cmd / 16 ; 
      turnon = cmd & 8 ; 
      wordwidth = (cwidth + 15)/16 ;
      if ( dynf == 14 ) 
      { bitweight = 256 ; 
        for ( i = 1 ; i <= cheight ; i ++ ) 
          { word = 0 ; 
            wordweight = 32768 ; 
            for ( j = 1 ; j <= cwidth ; j ++ ) 
              { if ( getbit () ) word += wordweight ; 
                wordweight >>= 1 ;
                if ( wordweight == 0 ) 
                { *raster++ = word ; 
                  word = 0 ;
                  wordweight = 32768 ; 
                  } 
                } 
              if ( wordweight != 32768 ) 
                 *raster++ = word ; 
            } 
      } else {
        rowsleft = cheight ; 
        hbit = cwidth ; 
        repeatcount = 0 ; 
        ww = 16 ; 
        word = 0 ; 
        bitweight = 16 ;
        while ( rowsleft > 0 ) 
          { count = pkpackednum() ; 
            while ( count != 0 ) 
              { if ( ( count <= ww ) && ( count < hbit ) ) 
                { if ( turnon ) word += gpower [ ww ] - gpower 
                  [ ww - count ] ; 
                  hbit -= count ; 
                  ww -= count ; 
                  count = 0 ; 
                  } 
                else if ( ( count >= hbit ) && ( hbit <= ww ) ) 
                { if ( turnon )
                     word += gpower[ww] - gpower[ww-hbit] ; 
                  *raster++ = word ; 
                  for ( i = 1 ; i <= repeatcount ; i ++ ) {
                    for ( j = 1 ; j <= wordwidth ; j ++ ) {
                      *raster = *(raster - wordwidth) ;
                      raster++ ;
                    }
                  }
                  rowsleft -= repeatcount + 1 ; 
                  repeatcount = 0 ; 
                  word = 0 ; 
                  ww = 16 ; 
                  count -= hbit ; 
                  hbit = cwidth ; 
                  } 
                else 
                { if ( turnon ) word += gpower [ ww ] ; 
                  *raster++ = word ;
                  word = 0 ; 
                  count -= ww ; 
                  hbit -= ww ; 
                  ww = 16 ; 
                  } 
                } 
              turnon = ! turnon ; 
            } 
          if ( ( rowsleft != 0 ) || ( (unsigned)hbit != cwidth ) ) 
             error ( "! error while unpacking; more bits than required" ) ; 
        } 
    if (*(char *)&bftest) /* is the hardware LittleEndian? */
       flip((char *)oraster, ((cwidth + 15) >> 4) * (long)cheight) ;
   return(p-pack) ;
}
