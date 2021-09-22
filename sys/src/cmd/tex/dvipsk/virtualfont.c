/*
 *   Here's the code to load a VF file into memory.
 *   Any resemblance between this file and loadfont.c is purely uncoincidental.
 */
#include "dvips.h" /* The copyright notice in that file is included too! */
#ifdef KPATHSEA
#include <kpathsea/c-pathmx.h>
#endif
/*
 *   These are the external routines we use.
 */
#include "protos.h"
/*
 *   These are the external variables we use.
 */
#ifdef DEBUG
extern integer debug_flag;
#endif  /* DEBUG */
extern long bytesleft ;
extern quarterword *raster ;
#ifndef KPATHSEA
extern char *vfpath ;
#endif
extern char errbuf[200] ;
extern real conv ;
extern real vconv ;
extern real alpha ;
extern int actualdpi ;
extern int vactualdpi ;
extern char *nextstring, *maxstring ;
extern fontdesctype *fonthead ;
extern real alpha ;
/*
 *   Now we have some routines to get stuff from the VF file.
 *   Subroutine vfbyte returns the next byte.
 */
static FILE *vffile ;
static char name[50] ;
void
badvf P1C(char *, s)
{
   (void)sprintf(errbuf,"! Bad VF file %s: %s",name,s) ;
   error(errbuf);
}

shalfword
vfbyte P1H(void)
{
   register shalfword i ;

   if ((i=getc(vffile))==EOF)
      badvf("unexpected eof") ;
   return(i) ;
}

integer
vfquad P1H(void)
{
   register integer i ;

   i = vfbyte() ;
   if (i > 127)
      i -= 256 ;
   i = i * 256 + vfbyte() ;
   i = i * 256 + vfbyte() ;
   i = i * 256 + vfbyte() ;
   return(i) ;
}

integer
vftrio P1H(void)
{
   register integer i ;

   i = vfbyte() ;
   i = i * 256 + vfbyte() ;
   i = i * 256 + vfbyte() ;
   return(i) ;
}

Boolean
vfopen P1C(register fontdesctype *, fd)
{
   register char *d, *n ;

   n = fd->name ;
#ifndef KPATHSEA
   d = fd->area ;
   if (*d==0)
      d = vfpath ;
#endif
#ifdef MVSXA   /* IBM: MVS/XA */
   (void)sprintf(name, "vf(%s)", n) ;
#else
   (void)sprintf(name, "%s.vf", n) ;
#endif
#ifdef KPATHSEA
   if (0 != (vffile=search(vfpath, name, READBIN)))
#else
   if (0 != (vffile=search(d, name, READBIN)))
#endif
      return(1) ;
#ifdef Omega
#ifdef KPATHSEA
   if (0 != (vffile=search(ovfpath, n, READBIN)))
#else
   if (0 != (vffile=search(d, n, READBIN)))
#endif
      return(2) ;
#endif
   return(0) ;
}

/*
 * The following routine is like fontdef, but for local fontdefs in VF files.
 */
fontmaptype *
vfontdef P2C(integer, s, int, siz)
{
   register integer i, j, fn ;
   register fontdesctype *fp ;
   register fontmaptype *cfnt ;
   char *nam, *area ;
   integer cksum, scsize, dssize ;

   fn = vfbyte() ;
   while (siz-- > 1)
      fn = (fn << 8) + vfbyte() ;
   cfnt = (fontmaptype *)mymalloc((integer)sizeof(fontmaptype)) ;
   cfnt->fontnum = fn ;
   cksum = vfquad() ;
   scsize = scalewidth(s, vfquad()) ;
   dssize = (integer)(alpha * (real)vfquad()) ;
   i = vfbyte() ; j = vfbyte() ;
   if (nextstring + i + j > maxstring)
      error("! out of string space") ;
   area = nextstring ;
   for (; i>0; i--)
      *nextstring++ = vfbyte() ;
   *nextstring++ = 0 ;
   nam = nextstring ;
   for (; j>0; j--)
      *nextstring++ = vfbyte() ;
   *nextstring++ = 0 ;
   fp = matchfont(nam, area, scsize, (char *)0) ;
   if (fp) {
      nextstring = nam ;
      fp->checksum = cksum ;
   } else {
      fp = newfontdesc(cksum, scsize, dssize, nam, area) ;
      fp->next = fonthead ;
      fonthead = fp ;
   }
   cfnt->desc = fp ;
   return (cfnt) ;
}

/*
 *   Now our virtualfont routine.
 */
Boolean
virtualfont P1C(register fontdesctype *, curfnt)
{
#ifdef Omega
   register integer i ;
#else
   register shalfword i ;
#endif
   register shalfword cmd ;
   register integer k ;
   register integer length ;
#ifdef Omega
   register integer cc ;
#else
   register shalfword cc ;
#endif
   register chardesctype *cd ;
   integer scaledsize = curfnt->scaledsize ;
#ifdef Omega
   integer no_of_chars=256;
   integer maxcc=0;
   chardesctype *newchardesc;
#endif
   register quarterword *tempr ;
   fontmaptype *fm, *newf ;
#ifdef Omega
   integer kindfont;
   kindfont=vfopen(curfnt);  /* 1 for TeX, 2 for Omega */
   if (!kindfont)
      return (0) ;
#else
   if (!vfopen(curfnt))
      return (0) ;
#endif
#ifdef DEBUG
   if (dd(D_FONTS))
      (void)fprintf(stderr,"Loading virtual font %s at %.1fpt\n",
         name, (real)scaledsize/(alpha*0x100000)) ;
#endif /* DEBUG */

/*
 *   We clear out some pointers:
 */
#ifdef Omega
   if (kindfont==2) {
     no_of_chars = 65536;
     curfnt->maxchars=65536;
     free(curfnt->chardesc);
     curfnt->chardesc = (chardesctype *)
                        mymalloc(65536 * (integer)sizeof(chardesctype));
   }
   for (i=0; i<no_of_chars; i++) {
#else
   for (i=0; i<256; i++) {
#endif
      curfnt->chardesc[i].TFMwidth = 0 ;
      curfnt->chardesc[i].packptr = NULL ;
      curfnt->chardesc[i].pixelwidth = 0 ;
      curfnt->chardesc[i].flags = 0 ;
   }
   if (vfbyte()!=247)
      badvf("expected pre") ;
   if (vfbyte()!=202)
      badvf("wrong id byte") ;
   for(i=vfbyte(); i>0; i--)
      (void)vfbyte() ;
   k = vfquad() ;
   check_checksum (k, curfnt->checksum, curfnt->name);
   k = (integer)(alpha * (real)vfquad()) ;
   if (k > curfnt->designsize + 2 || k < curfnt->designsize - 2) {
      (void)sprintf(errbuf,"Design size mismatch in font %s", name) ;
      error(errbuf) ;
   }
/*
 * Now we look for font definitions.
 */
   fm = NULL ;
   while ((cmd=vfbyte())>=243) {
      if (cmd>246)
         badvf("unexpected command in preamble") ;
      newf = vfontdef(scaledsize, cmd-242) ;
      if (fm)
         fm->next = newf ;
      else curfnt->localfonts = newf ;
      fm = newf ;
      fm->next = NULL ; /* FIFO */
   }
/*
 *   Now we get down to the serious business of reading character definitions.
 */
   do {
      if (cmd==242) {
         length = vfquad() + 2 ;
         if (length<2) badvf("negative length packet") ;
         if (length>65535) badvf("packet too long") ;
         cc = vfquad() ;
#ifdef Omega
         if (cc<0 || cc>65535) badvf("character code out of range") ;
#else
         if (cc<0 || cc>255) badvf("character code out of range") ;
#endif
         cd = curfnt->chardesc + cc ;
         cd->TFMwidth = scalewidth(vfquad(), scaledsize) ;
      } else {
         length = cmd + 2;
         cc = vfbyte() ;
         cd = curfnt->chardesc + cc ;
         cd->TFMwidth = scalewidth(vftrio(), scaledsize) ;
      }
#ifdef Omega
      maxcc = (maxcc<cc) ? cc : maxcc;
#endif
      if (cd->TFMwidth >= 0)
         cd->pixelwidth = ((integer)(conv*cd->TFMwidth+0.5)) ;
      else
         cd->pixelwidth = -((integer)(conv*-cd->TFMwidth+0.5)) ;
      cd->flags = EXISTS ;
      if (bytesleft < length) {
#ifdef DEBUG
          if (dd(D_MEM))
             (void)fprintf(stderr,
#ifdef SHORTINT
                   "Allocating new raster memory (%ld req, %ld left)\n",
#else
                   "Allocating new raster memory (%d req, %ld left)\n",
#endif
                                length, bytesleft) ;
#endif /* DEBUG */
          if (length > MINCHUNK) {
             tempr = (quarterword *)mymalloc((integer)length) ;
             bytesleft = 0 ;
          } else {
             raster = (quarterword *)mymalloc((integer)RASTERCHUNK) ;
             tempr = raster ;
             bytesleft = RASTERCHUNK - length ;
             raster += length ;
         }
      } else {
         tempr = raster ;
         bytesleft -= length ;
         raster += length ;
      }
      cd->packptr = tempr ;
      length -= 2 ;
      *tempr++ = length / 256 ;
      *tempr++ = length % 256 ;
         for (; length>0; length--)
            *tempr++ = vfbyte() ;
      cmd = vfbyte() ;
   } while (cmd < 243) ;
   if (cmd != 248)
      badvf("missing postamble") ;
   (void)fclose(vffile) ;
   curfnt->loaded = 2 ;
#ifdef Omega
   newchardesc = (chardesctype *)
                 mymalloc((maxcc+1) * (integer)sizeof(chardesctype));
   for (i=0; i<=maxcc; i++) {
     newchardesc[i] = curfnt->chardesc[i];
   }
   free(curfnt->chardesc);
   curfnt->chardesc = newchardesc;
   curfnt->maxchars=maxcc+1;
#endif
   return (1) ;
}
