/*
 *   Input bytes from the dvi file or the current virtual character.
 *   These routines could probably be sped up significantly; but they are
 *   very machine dependent, so I will leave such tuning to the installer.
 *   They simply get and return bytes in batches of one, two, three, and four,
 *   updating the current position as necessary.
 */
#include "structures.h" /* The copyright notice in that file is included too! */
void error() ;
extern FILE *dvifile ;
extern quarterword *curpos, *curlim ;

void
abortpage()
{
   error("! unexpected eof on DVI file") ;
}

shalfword  /* the value returned is, however, between 0 and 255 */
dvibyte()
{
  register shalfword i ;
  if (curpos) {
     if (curpos>=curlim) return((shalfword)140) ;
     return (*curpos++) ;
  }
  if ((i=getc(dvifile))==EOF)
    abortpage() ;
  return(i) ;
}

halfword
twobytes()
{
  register halfword i ;
  i = dvibyte() ;
  return(i*256+dvibyte()) ; }

integer
threebytes()
{
  register integer i ;
  i = twobytes() ;
  return(i*256+dvibyte()) ; }

shalfword
signedbyte()
{
  register shalfword i ;
  if (curpos) {
     if (curpos>=curlim)
       error("! unexpected end of virtual packet") ;
     i = *curpos++ ;
  } else if ((i=getc(dvifile))==EOF)
    abortpage() ;
  if (i<128) return(i) ;
  else return(i-256) ;
}

shalfword
signedpair()
{
  register shalfword i ;
  i = signedbyte() ;
  return(i*256+dvibyte()) ;
}

integer
signedtrio()
{
  register integer i ;
  i = signedpair() ;
  return(i*256+dvibyte()) ;
}

integer
signedquad()
{
  register integer i ;
  i = signedpair() ;
  return(i*65536+twobytes()) ;
}

void
skipover(i)
        int i ;
{
  while (i-->0) (void)dvibyte() ;
}
