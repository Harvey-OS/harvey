/*
 *   Here's the code to load a PK file into memory.
 *   Individual bitmaps won't be unpacked until they prove to be needed.
 */
#include "structures.h" /* The copyright notice in that file is included too! */
/*
 *   These are the external routines we use.
 */
extern void makefont() ;
extern void error() ;
extern integer scalewidth() ;
extern int tfmload() ;
extern FILE *pksearch() ;
/*
 *   These are the external variables we use.
 */
#ifdef DEBUG
extern integer debug_flag;
#endif  /* DEBUG */
extern long bytesleft ;
extern quarterword *raster ;
extern real conv ;
extern int actualdpi, vactualdpi ;
extern real alpha ;
extern char *pkpath ;
char errbuf[200] ;
int lastresortsizes[40] ;
extern integer fsizetol ;
extern Boolean nosmallchars ;
extern Boolean compressed ;
extern Boolean dopprescan ;
#ifdef FONTLIB
extern Boolean flib ;
extern FILE *flisearch() ;
#endif
/*
 *   Now we have some routines to get stuff from the PK file.
 *   Subroutine pkbyte returns the next byte.
 */
FILE *pkfile ;
char name[50] ;
void
badpk(s)
   char *s ;
{
   (void)sprintf(errbuf,"! Bad PK file %s: %s",name,s) ;
   error(errbuf);
}

shalfword
pkbyte()
{
   register shalfword i ;

   if ((i=getc(pkfile))==EOF)
      badpk("unexpected eof") ;
   return(i) ;
}

integer
pkquad()
{
   register integer i ;

   i = pkbyte() ;
   if (i > 127)
      i -= 256 ;
   i = i * 256 + pkbyte() ;
   i = i * 256 + pkbyte() ;
   i = i * 256 + pkbyte() ;
   return(i) ;
}

integer
pktrio()
{
   register integer i ;

   i = pkbyte() ;
   i = i * 256 + pkbyte() ;
   i = i * 256 + pkbyte() ;
   return(i) ;
}


/*
 *   pkopen opens the pk file.  This is system dependent.  We work really
 *   hard to open some sort of PK file.
 */
#ifdef VMCMS /* IBM: VM/CMS - we let system administrator handle this on
                             VMCMS since some printers can't get to user
                             fonts anyway */
int dontmakefont = 1 ;
#else
int dontmakefont = 0 ; /* if makefont fails once we won't try again */
#endif  /* IBM: VM/CMS */

void
lectureuser() {
   static int userwarned = 0 ;

   if (! userwarned) {
      error("Such scaling will generate extremely poor output.") ;
      userwarned = 1 ;
   }
}
Boolean
pkopen(fd)
        register fontdesctype *fd ;
{
   register char *d, *n ;
   int vdpi ;

   d = fd->area ;
   n = fd->name ;
   if (*d==0)
      d = pkpath ;
#ifdef FONTLIB
   if (*(fd->area) == 0) { 
      int del ;
      for (del=0; del<=RES_TOLERANCE(fd->dpi); del=del>0?-del:-del+1) {
        if ((pkfile=flisearch(n, fd->dpi + del)) != (FILE *)NULL )
          return(1);
      }
   }
#endif
   {  
      int del ;
      for (del=0; del<=RES_TOLERANCE(fd->dpi); del=del>0?-del:-del+1) {
         if (actualdpi == vactualdpi) {
            vdpi = 0 ;
         } else {
            vdpi = (2 * ((long)vactualdpi) * (fd->dpi + del) + actualdpi)
                                                       / (2 * actualdpi) ;
         }
         (void)sprintf(name, "%s.%dpk", n, fd->dpi + del) ;
         if (pkfile=pksearch(d, name, READBIN, n, fd->dpi + del, vdpi))
            return(1) ;
      }
   }
   if (d == pkpath) {
      if (actualdpi == vactualdpi) {
         vdpi = 0 ;
      } else {
         vdpi = (2 * ((long)vactualdpi) * fd->dpi + actualdpi)
                                                    / (2 * actualdpi) ;
      }
      (void)sprintf(name, "%s.%dpk", n, fd->dpi) ;
      makefont(n, (int)fd->dpi, DPI) ;
      if (dontmakefont == 0 &&
          (pkfile = pksearch(d, name, READBIN, n, fd->dpi, vdpi)))
               return(1) ;
#ifndef MSDOS
      dontmakefont = 1 ;
#endif
   }
/*
 *   If nothing above worked, then we get desparate.  We attempt to
 *   open the stupid font at one of a small set of predefined sizes,
 *   and then use PostScript scaling to generate the correct size.
 *
 *   We much prefer scaling up to scaling down, since scaling down
 *   can omit character features, so we try the larger sizes first,
 *   and then work down.
 */
   {
      int i, j ;

      if (lastresortsizes[0] && fd->dpi < 30000) {
         for (i=0; lastresortsizes[i] < fd->dpi; i++) ;
         for (j = i-1; j >= 0; j--) {
            if (actualdpi == vactualdpi) {
               vdpi = 0 ;
            } else {
               vdpi = (2 * ((long)vactualdpi) * lastresortsizes[j] + actualdpi)
                                                       / (2 * actualdpi) ;
            }
            (void)sprintf(name, "%s.%dpk", n, lastresortsizes[j]) ;
#ifdef FONTLIB
            if ((pkfile=flisearch(n,(halfword)lastresortsizes[j]))
             || (pkfile=pksearch(d, name, READBIN, n,
                         (halfword)lastresortsizes[j], vdpi))) {
#else
            if (pkfile=pksearch(d, name, READBIN, n,
                         (halfword)lastresortsizes[j], vdpi)) {
#endif
               fd->loadeddpi = lastresortsizes[j] ;
               fd->alreadyscaled = 0 ;
               (void)sprintf(errbuf,
                       "Font %s at %d not found; scaling %d instead.",
                                         name, fd->dpi, lastresortsizes[j]) ;
               error(errbuf) ;
               lectureuser() ;
               return 1 ;
            }
         }
         for (j = i; lastresortsizes[j] < 30000; j++) {
            if (actualdpi == vactualdpi) {
               vdpi = 0 ;
            } else {
               vdpi = (2 * ((long)vactualdpi) * lastresortsizes[j] + actualdpi)
                                                       / (2 * actualdpi) ;
            }
            (void)sprintf(name, "%s.%dpk", n, lastresortsizes[j]) ;
#ifdef FONTLIB
            if ((pkfile=flisearch(n, (halfword)lastresortsizes[j]))
                || (pkfile=pksearch(d, name, READBIN, n,
                      (halfword)lastresortsizes[j], vdpi))) {
#else
            if (pkfile=pksearch(d, name, READBIN, n, 
                            (halfword)lastresortsizes[j], vdpi)) {
#endif
               fd->loadeddpi = lastresortsizes[j] ;
               fd->alreadyscaled = 0 ;
               (void)sprintf(errbuf,
                       "Font %s at %d not found; scaling %d instead.",
                                         name, fd->dpi, lastresortsizes[j]) ;
               error(errbuf) ;
               lectureuser() ;
               return 1 ;
            }
         }
      }
   }
   (void)sprintf(name, "%s.%dpk", n, fd->dpi) ;
   (void)sprintf(errbuf,
      "Font %s%s not found, characters will be left blank.",
      fd->area, name) ;
   error(errbuf) ;
   return(0) ;
}

/*
 *   Now our loadfont routine.  We return an integer indicating the
 *   highest character code in the font, so we know how much space
 *   to reserve for the character.  (It's returned in the font
 *   structure, along with everything else.)
 */
void
loadfont(curfnt)
        register fontdesctype *curfnt ;
{
   register shalfword i ;
   register shalfword cmd ;
   register integer k ;
   register integer length = 0 ;
   register shalfword cc = 0 ;
   register integer scaledsize = curfnt->scaledsize ;
   register quarterword *tempr ;
   register chardesctype *cd = 0 ;
   int maxcc = 0 ;
   int munged = 0 ;
/*
 *   We clear out some pointers:
 */
   if (curfnt->loaded == 3) {
      for (i=0; i<256; i++) {
         curfnt->chardesc[i].TFMwidth = 0 ;
         curfnt->chardesc[i].packptr = NULL ;
         curfnt->chardesc[i].pixelwidth = 0 ;
         curfnt->chardesc[i].flags &= EXISTS ;
      }
   } else {
      for (i=0; i<256; i++) {
         curfnt->chardesc[i].TFMwidth = 0 ;
         curfnt->chardesc[i].packptr = NULL ;
         curfnt->chardesc[i].pixelwidth = 0 ;
         curfnt->chardesc[i].flags = 0 ;
      }
   }
   curfnt->maxchars = 256 ; /* just in case we return before the end */
   if (!pkopen(curfnt)) {
      tfmload(curfnt) ;
      return ;
   }
#ifdef DEBUG
   if (dd(D_FONTS))
      (void)fprintf(stderr,"Loading pk font %s at %.1fpt\n",
         name, (real)scaledsize/(alpha*0x100000)) ;
#endif /* DEBUG */
   if (pkbyte()!=247)
      badpk("expected pre") ;
   if (pkbyte()!=89)
      badpk("wrong id byte") ;
   for(i=pkbyte(); i>0; i--)
      (void)pkbyte() ;
   k = (integer)(alpha * (real)pkquad()) ;
   if (k > curfnt->designsize + fsizetol ||
       k < curfnt->designsize - fsizetol) {
      (void)sprintf(errbuf,"Design size mismatch in font %s", name) ;
      error(errbuf) ;
   }
   k = pkquad() ;
   if (k && curfnt->checksum)
      if (k!=curfnt->checksum) {
         (void)sprintf(errbuf,"Checksum mismatch in font %s", name) ;
         error(errbuf) ;
       }
   k = pkquad() ; /* assume that hppp is correct in the PK file */
   k = pkquad() ; /* assume that vppp is correct in the PK file */
/*
 *   Now we get down to the serious business of reading character definitions.
 */
   while ((cmd=pkbyte())!=245) {
      if (cmd < 240) {
         switch (cmd & 7) {
case 0: case 1: case 2: case 3:
            length = (cmd & 7) * 256 + pkbyte() - 3 ;
            cc = pkbyte() ;
            cd = curfnt->chardesc+cc ;
            if (nosmallchars || curfnt->dpi != curfnt->loadeddpi)
               cd->flags |= BIGCHAR ;
            cd->TFMwidth = scalewidth(pktrio(), scaledsize) ;
            cd->pixelwidth = pkbyte() ;
            break ;
case 4: case 5: case 6:
            length = (cmd & 3) * 65536L + pkbyte() * 256L ;
            length = length + pkbyte() - 4L ;
            cc = pkbyte() ;
            cd = curfnt->chardesc+cc ;
            cd->TFMwidth = scalewidth(pktrio(), scaledsize) ;
            cd->flags |= BIGCHAR ;
            i = pkbyte() ;
            cd->pixelwidth = i * 256 + pkbyte() ;
            break ;
case 7:
            length = pkquad() - 11 ;
            cc = pkquad() ;
            if (cc<0 || cc>255) badpk("character code out of range") ;
            cd = curfnt->chardesc + cc ;
            cd->flags |= BIGCHAR ;
            cd->TFMwidth = scalewidth(pkquad(), scaledsize) ;
            cd->pixelwidth = (pkquad() + 32768) >> 16 ;
            k = pkquad() ;
         }
         if (cd->pixelwidth == 0 && cd->TFMwidth != 0) {
            if (cd->TFMwidth > 0)
               k = (integer)(cd->TFMwidth * conv + 0.5) ;
            else
               k = -(integer)(-cd->TFMwidth * conv + 0.5) ;
            if (k != 0) {
               cd->pixelwidth = k ;
               munged++ ;
            }
         }
         if (length <= 0)
            badpk("packet length too small") ;
         if (dopprescan && ((cd->flags & EXISTS) == 0)) {
            for (length--; length>0; length--)
               (void)pkbyte() ;
         } else {
            if (cc > maxcc)
               maxcc = cc ;
            if (bytesleft < length || (length > MINCHUNK && compressed)) {
#ifdef DEBUG
                if (dd(D_FONTS))
                   (void)fprintf(stderr,
#ifdef SHORTINT
                      "Allocating new raster memory (%ld req, %ld left)\n",
                                   length, bytesleft) ;
#else
                      "Allocating new raster memory (%d req, %d left)\n",
                                   (int)length, (int)bytesleft) ;
#endif
#endif /* DEBUG */
                if (length > MINCHUNK) {
                   tempr = (quarterword *)mymalloc((integer)length) ;
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
            *tempr++ = cmd ;
            for (length--; length>0; length--)
               *tempr++ = pkbyte() ;
         }
      } else {
         k = 0 ;
         switch (cmd) {
case 243:
            k = pkbyte() ;
            if (k > 127)
               k -= 256 ;
case 242:
            k = k * 256 + pkbyte() ;
case 241:
            k = k * 256 + pkbyte() ;
case 240:
            k = k * 256 + pkbyte() ;
            while (k-- > 0)
               i = pkbyte() ;
            break ;
case 244:
            k = pkquad() ;
            break ;
case 246:
            break ;
default:
            badpk("! unexpected command") ;
         }
      }
   }
#ifdef FONTLIB
   if (flib)
      flib = 0 ;
   else
#endif
   (void)fclose(pkfile) ;
   curfnt->loaded = 1 ;
   curfnt->maxchars = maxcc + 1 ;
   if (munged > 0) {
      static int seen = 0 ;
      sprintf(errbuf,
          "Font %s at %d dpi has most likely been made improperly;",
           curfnt->name, curfnt->dpi) ;
      error(errbuf) ;
      if (seen)
         return ;
      seen = 1 ;
      sprintf(errbuf,
     "%d characters have 0 escapements but non-trivial TFM widths.", munged) ;
      error(errbuf) ;
      error(
          "I'm stumbling along as best I can, but I recommend regenerating") ;
      error(
          "these fonts; the problem is probably that they are non-CM fonts") ;
      error(
          "(such as circle10 or line10) created with a MF with the CM base") ;
      error("preloaded .") ;
   }
}
