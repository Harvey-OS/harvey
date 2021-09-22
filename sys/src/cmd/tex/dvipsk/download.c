/*
 *   Code to download a font definition at the beginning of a section.
 * hacked by SPQR 8.95 to allow for non-partial downloading
 *
 */
#include "dvips.h" /* The copyright notice in that file is included too! */
#include "t1part.h"
#define DVIPS
/*
 *   These are the external routines we call.
 */
#include "protos.h"
/*
 *   These are the external variables we access.
 */
extern char *nextstring, *infont ;
extern FILE *bitfile ;
extern fontdesctype *curfnt ;
extern long bytesleft ;
extern long mbytesleft ;
extern quarterword *raster ;
extern quarterword *mraster ;
extern Boolean compressed ;
extern Boolean partialdownload ;
extern double mag ;
extern int actualdpi ;
static unsigned char dummyend[8] = { 252 } ;
extern int prettycolumn ;
extern int quiet ;
extern Boolean disablecomments ;

/*
 *   We have a routine that downloads an individual character.
 */
static int lastccout ;
void downchar P2C(chardesctype *, c, shalfword, cc)
{
   register long i, j ;
   register halfword cheight, cwidth ;
   register long k ;
   register quarterword *p ;
   register halfword cmd ;
   register shalfword xoff, yoff ;
   halfword wwidth = 0 ;
   register long len ;
   int smallchar ;

   p = c->packptr ;
   cmd = *p++ ;
   if (cmd & 4) {
      if ((cmd & 7) == 7) {
         cwidth = getlong(p) ;
         cheight = getlong(p + 4) ;
         xoff = getlong(p + 8) ;
         yoff = getlong(p + 12) ;
         p += 16 ;
      } else {
         cwidth = p[0] * 256 + p[1] ;
         cheight = p[2] * 256 + p[3] ;
         xoff = p[4] * 256 + p[5] ; /* N.B.: xoff, yoff are signed halfwords */
         yoff = p[6] * 256 + p[7] ;
         p += 8 ;
      }
   } else {
      cwidth = *p++ ;
      cheight = *p++ ;
      xoff = *p++ ;
      yoff = *p++ ;
      if (xoff > 127)
         xoff -= 256 ;
      if (yoff > 127)
         yoff -= 256 ;
   }
   if (c->flags & BIGCHAR)
      smallchar = 0 ;
   else
      smallchar = 5 ;
   if (compressed) {
      len = getlong(p) ;
      p += 4 ;
   } else {
      wwidth = (cwidth + 15) / 16 ;
      i = 2 * cheight * (long)wwidth ;
      if (i <= 0)
         i = 2 ;
      i += smallchar ;
      if (mbytesleft < i) {
         if (mbytesleft >= RASTERCHUNK)
            (void) free((char *) mraster) ;
         if (RASTERCHUNK > i) {
            mraster = (quarterword *)mymalloc((integer)RASTERCHUNK) ;
            mbytesleft = RASTERCHUNK ;
         } else {
            k = i + i / 4 ;
            mraster = (quarterword *)mymalloc((integer)k) ;
            mbytesleft = k ;
         }
      }
      k = i;
      while (k > 0)
         mraster[--k] = 0 ;
      unpack(p, (halfword *)mraster, cwidth, cheight, cmd) ;
      p = mraster ;
      len = i - smallchar ;
   }
   if (cheight == 0 || cwidth == 0 || len == 0) {
      cwidth = 1 ;
      cheight = 1 ;
      wwidth = 1 ;
      len = 2 ;
      if (compressed)
         p = dummyend ;  /* CMD(END); see repack.c */
      else
         mraster[0] = 0 ;
   }
   if (smallchar) {
      p[len] = cwidth ;
      p[len + 1] = cheight ;
      p[len + 2] = xoff + 128 ;
      p[len + 3] = yoff + 128 ;
      p[len + 4] = c->pixelwidth ;
   } else
/*
 *   Now we actually send out the data.
 */
      specialout('[') ;
   if (compressed) {
      specialout('<') ;
      mhexout(p, len + smallchar) ;
      specialout('>') ;
   } else {
      i = (cwidth + 7) / 8 ;
      if (i * cheight > 65520) {
         long bc = 0 ;
         specialout('<') ;
         for (j=0; j<cheight; j++) {
            if (bc + i > 65520) {
               specialout('>') ;
               specialout('<') ;
               bc = 0 ;
            }
            mhexout(p, i) ;
            bc += i ;
            p += 2*wwidth ;
         }
         specialout('>') ;
      } else {
         specialout('<') ;
         if (2 * wwidth == i)
            mhexout(p, ((long)cheight) * i + smallchar) ;
         else {
            for (j=0; j<cheight; j++) {
               mhexout(p, i) ;
               p += 2*wwidth ;
            }
            if (smallchar)
               mhexout(p, (long)smallchar) ;
         }
         specialout('>') ;
      }
   }
   if (smallchar == 0) {
      numout((integer)cwidth) ;
      numout((integer)cheight) ;
      numout((integer)xoff + 128) ; /* not all these casts needed. */
      numout((integer)yoff + 128) ;
      numout((integer)(c->pixelwidth)) ;
   }
   if (lastccout + 1 == cc) {
      cmdout("I") ;
   } else {
      numout((integer)cc) ;
      cmdout("D") ;
   }
   lastccout = cc ;
}
/*
 * Output the literal name of the font change command with PostScript index n
 */
static char goodnames[] =
   "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789" ;
void
makepsname P2C(register char *, s, register int, n)
{
   n-- ;
   *s++ = 'F' + n / (sizeof(goodnames)-1) ;
   *s++ = goodnames[n % (sizeof(goodnames)-1)] ;
   *s++ = 0 ;
}
void
lfontout P1C(int, n)
{
	char buf[10];
        char *b = buf ;
        *b++ = '/' ;
        makepsname(b, n) ;
	cmdout(buf);
}

/*
 *   And the download procedure.
 */
void download P2C(charusetype *, p, int, psfont)
{
   register int b, i ;
   register halfword bit ;
   register chardesctype *c ;
   int cc, maxcc = -1, numcc ;
   double fontscale ;
   char name[10] ;
   int non_empty=0;
   lastccout = -5 ;
   name[0] = '/' ;
   makepsname(name + 1, psfont) ;
   curfnt = p->fd ;
   curfnt->psname = psfont ;
   if (curfnt->resfont) {
      struct resfont *rf = curfnt->resfont ;
      for (b=0; b<16; b++)
        if(p->bitmap[b] !=0)
            non_empty =1;
      if(non_empty==0)
        return;
      cmdout(name) ;
/* following code re-arranged - Rob Hutchings 1992Apr02 */
      c = curfnt->chardesc + 255 ;
      cc = 255 ;
      numcc = 0 ;
      i = 0 ;
      for (b=15; b>=0; b--) {
         for (bit=1; bit; bit<<=1) {
             if (p->bitmap[b] & bit) {
               if (i > 0) {
                  numout((integer)i) ;
                  specialout('[') ;
                  i = 0 ;
               }
               numout((integer)c->pixelwidth) ;
               c->flags |= EXISTS ;
               numcc++ ;
            } else {
               i++ ;
               c->flags &= ~EXISTS ;
            }
            c-- ;
            cc-- ;
         }
      }
      if (i > 0) {
         numout((integer)i) ;
         specialout('[') ;
      }
      specialout ('{') ;
      if (rf->specialinstructions)
         cmdout(rf->specialinstructions) ;
      specialout ('}') ;
      numout((integer)numcc) ;
      fontscale = ((double)(curfnt->scaledsize)) / 655360.0 ;
/*   A long-standing bug here; was 7200.  Caught by Sergey Lesenko. */
      fontscale *= (mag/7227.0) ;
      fontscale *= actualdpi ;
      (void)sprintf(nextstring, "%g", fontscale) ;
      cmdout(nextstring) ;
      (void)strcpy(nextstring, "/") ;
      (void)strcat(nextstring, rf->PSname) ;
      cmdout(nextstring) ;
/* end of changed section - Rob */
      cmdout("rf") ;
      rf->sent = 1 ;
      return ;
   }
/*
 *   Here we calculate the largest character actually used, and
 *   send it as a parameter to df.
 */
   cc = 0 ;
   numcc = 0 ;
   for (b=0; b<16; b++) {
      for (bit=32768; bit!=0; bit>>=1) {
         if (p->bitmap[b] & bit) {
            maxcc = cc ;
            numcc++ ;
         }
         cc++ ;
      }
   }
   if (numcc <= 0)
      return ;
   fontscale = ((double)(curfnt->scaledsize)) / 65536.0 ;
   fontscale *= (mag/1000.0) ;
   newline() ;
   fprintf(bitfile, "%%DVIPSBitmapFont: %s %s %g %d\n", name+1, curfnt->name,
                     fontscale, numcc) ;
   cmdout(name) ;
   numout((integer)numcc) ;
   numout((integer)maxcc + 1) ;
/*
 *   If we need to scale the font, we say so by using dfs
 *   instead of df, and we give it a scale factor.  We also
 *   scale the character widths; this is ugly, but scaling
 *   fonts is ugly, and this is the best we can probably do.
 */
   if (curfnt->dpi != curfnt->loadeddpi) {
      numout((integer)curfnt->dpi) ;
      numout((integer)curfnt->loadeddpi) ;
      if (curfnt->alreadyscaled == 0) {
         for (b=0, c=curfnt->chardesc; b<256; b++, c++)
            c->pixelwidth = (c->pixelwidth * 
      (long)curfnt->dpi * 2 + curfnt->loadeddpi) / (2 * curfnt->loadeddpi) ;
         curfnt->alreadyscaled = 1 ;
      }
      cmdout("dfs") ;
   } else
      cmdout("df") ;
   c = curfnt->chardesc ;
   cc = 0 ;
   for (b=0; b<16; b++) {
      for (bit=32768; bit; bit>>=1) {
         if (p->bitmap[b] & bit) {
            downchar(c, cc) ;
            c->flags |= EXISTS ;
         } else
            c->flags &= ~EXISTS ;
         c++ ;
         cc++ ;
      }
   }
   cmdout("E") ;
   newline() ;
   fprintf(bitfile, "%%EndDVIPSBitmapFont\n") ;
}

void downpsfont P2C(charusetype *, p, charusetype *, all)
{
    int GridCount ;
    register int b;
    register halfword bit ;
    register chardesctype *c ;
    struct resfont *rf ;
    int cc;

    curfnt = p->fd ;
    rf = curfnt->resfont ;
    if (rf == 0 || rf->Fontfile == NULL)
       return ;
    for (; all->fd; all++)
       if (all->fd->resfont &&
           strcmp(rf->PSname, all->fd->resfont->PSname) == 0)
          break ;
    if (all != p)
       return ;
    if (all->fd == 0)
       error("! internal error in downpsfont") ;
    if (!partialdownload) {
        infont = all->fd->resfont->PSname ;
	copyfile(all->fd->resfont->Fontfile);
        infont = 0 ;
	return;
    }
    for (cc=0; cc<256; cc++)
#ifdef SHIFTLOWCHARS
      grid[T1Char(cc)] = 0 ;
#else
    grid[cc] = 0 ;
#endif
    for (; all->fd; all++) {
        if (all->fd->resfont == 0 ||
            strcmp(rf->PSname, all->fd->resfont->PSname))
           continue ;
        curfnt = all->fd ;
        c = curfnt->chardesc + 255 ;
        cc = 255 ;
        for (b=15; b>=0; b--) {
            for (bit=1; bit; bit<<=1) {
                if (all->bitmap[b] & bit) {
                    grid[cc]=1;
                }
                c-- ;
                cc-- ;
            }
        }
    }

    for (GridCount=0,cc=0; cc<256; cc++) {
/*      tmpgrid[cc]=grid[cc]; */
        if(grid[cc] ==1) {
            GridCount++;
        }
    }
    if(GridCount!=0) {
        if (!quiet) {
           if (strlen(rf->Fontfile) + prettycolumn > STDOUTSIZE) {
              fprintf(stderr, "\n") ;
              prettycolumn = 0 ;
           }
           (void)fprintf(stderr, "<%s>", rf->Fontfile);
	   prettycolumn += strlen(rf->Fontfile) + 2 ;
	}
        newline() ;
        if (! disablecomments)
           (void)fprintf(bitfile, "%%%%BeginFont: %s\n",  rf->PSname);
        if(FontPart((void*)bitfile, (void*)rf->Fontfile, (void*)rf->Vectfile) < 0)
            exit(1);
        if (! disablecomments)
           (void)fprintf(bitfile, "%%%%EndFont \n");
   }
}

void dopsfont P1C(sectiontype *, fs)
{
    charusetype *cu ;

    cu = (charusetype *) (fs + 1) ;
    line = getmem(BUFSIZ);
    tmpline=line;
    while (cu->fd) {
       if (cu->psfused)
          cu->fd->psflag = EXISTS ;
       downpsfont(cu++, (charusetype *)(fs + 1)) ;
    }
    loadbase = ~FLG_LOAD_BASE;
    FirstCharB=UnDefineChars(FirstCharB);
    free(tmpline);
}
