/*
 *   Calculates the bounding box for a one-page .dvi file.
 *   Based on a lot of other dvips stuff.  Requires a total re-scan
 *   of the page.
 *
 *   If you request an EPSF file and specify -a, you may end up
 *   processing a single page four times!
 */
#include "structures.h"
extern char *mymalloc() ;
extern integer scalewidth() ;
extern void tfmopen(), skipover(), error() ;
extern shalfword tfmbyte(), dvibyte(), signedbyte(), signedpair() ;
extern halfword tfm16(), twobytes() ;
extern integer tfm32(), threebytes(), signedtrio(), signedquad() ;
extern double floor() ;

extern char *nextstring, errbuf[] ;
extern FILE *tfmfile, *dvifile ;
extern fontdesctype *baseFonts[] ;
extern integer firstboploc, num, den, mag ;

typedef struct {
   integer llx, lly, urx, ury ;
} bbchardesctype ;
typedef struct {
   bbchardesctype bbchardesc[256] ;
} bbfontdesctype ;

static bbfontdesctype *bbaseFonts[256] ;

integer nscalewidth(a, b)
register integer a, b ;
{
   if (a < 0)
      return - nscalewidth(-a, b) ;
   else if (b < 0)
      return - scalewidth(a, -b) ;
   else
      return scalewidth(a, b) ;
}

void bbtfmload(n)
int n ;
{
   register shalfword i ;
   register integer li ;
   integer scaledsize ;
   shalfword nw, nh, nd, ns, hd ;
   shalfword bc, ec ;
   integer scaled[300] ;
   integer chardat[256] ;
   integer slant = 0 ;
   bbchardesctype *cc ;
   register fontdesctype *curfnt = baseFonts[n] ;
   register bbfontdesctype *bbcurfnt =
      (bbfontdesctype *)mymalloc(sizeof(bbfontdesctype)) ;

   bbaseFonts[n] = bbcurfnt ;
   tfmopen(curfnt) ;
/*
 *   Next, we read the font data from the tfm file, and store it in
 *   our own arrays.
 */
   li = tfm16() ; hd = tfm16() ;
   bc = tfm16() ; ec = tfm16() ;
   nw = tfm16() ; nh = tfm16() ; nd = tfm16() ;
   ns = tfm16() ;
   ns += tfm16() ;
   ns += tfm16() ;
   ns += tfm16() ;
   li = tfm16() ;
   li = tfm32() ;
   if (li && curfnt->checksum)
      if (li!=curfnt->checksum) {
         (void)sprintf(errbuf,"Checksum mismatch in %s", curfnt->name) ;
         error(errbuf) ;
       }
   li = tfm32() ;
   for (i=2; i<hd; i++)
      li = tfm32() ;
   for (i=0; i<256; i++)
      chardat[i] = -1 ;
   for (i=bc; i<=ec; i++) {
      chardat[i] = tfm16() ;
      li = tfm16() ;
   }
   scaledsize = curfnt->scaledsize ;
   for (i=0; i<nw + nh + nd; i++)
      scaled[i] = scalewidth(tfm32(), scaledsize) ;
   for (i=0; i<ns; i++)
      tfm32() ;
   slant = tfm32() ;
   (void)fclose(tfmfile) ;
   for (i=0; i<256; i++) {
      cc = &(bbcurfnt->bbchardesc[i]) ;
      if (chardat[i] != -1) {
         cc->ury = scaled[((chardat[i] >> 4) & 15) + nw] ;
         cc->lly = - scaled[(chardat[i] & 15) + nw + nh] ;
         cc->llx = 0 ;
         cc->urx = curfnt->chardesc[i].TFMwidth ;
      } else {
         cc->llx = cc->lly = cc->urx = cc->ury = 0 ;
      }
   }
   if (slant) {
      for (i=0; i<256; i++) {
         cc = &(bbcurfnt->bbchardesc[i]) ;
         cc->llx += nscalewidth(cc->lly, slant) ;
         cc->urx += nscalewidth(cc->ury, slant) ;
      }
   }
}
extern struct dvistack {
  integer hh, vv ;
  integer h, v, w, x, y, z ;
} stack[] ;
static integer llx, lly, urx, ury ;
void bbdopage()
{
   register shalfword cmd ;
   register bbchardesctype *bcd ;
   register chardesctype *cd ;
   register integer h ;
   integer fnt ;
   int charmove ;
   struct dvistack *sp = stack ;
   integer v, w, x, y, z ;
   register fontdesctype *curfnt = 0 ;
   register bbfontdesctype *bbcurfnt = 0 ;

   w = x = y = z = 0 ;
   h = 0 ; v = 0 ;
   llx = lly = 1000000000 ;
   urx = ury = -1000000000 ;
   charmove = 0 ;
   while (1) {
      switch (cmd=dvibyte()) {
case 138: break ;
case 133: /* put1 */
         cmd = dvibyte() ;
         charmove = 0 ;
         goto dochar ;
case 128: cmd = dvibyte() ; /* set1 command drops through to setchar */
default: /* these are commands 0 (setchar0) thru 127 (setchar127) */
         charmove = 1 ;
dochar:
         cd = &(curfnt->chardesc[cmd]) ;
         bcd = &(bbcurfnt->bbchardesc[cmd]) ;
         if (h + bcd->llx < llx) llx = h + bcd->llx ;
         if (h + bcd->urx > urx) urx = h + bcd->urx ;
         if (v - bcd->ury < lly) lly = v - bcd->ury ;
         if (v - bcd->lly > ury) ury = v - bcd->lly ;
         if (charmove)
            h += cd->TFMwidth ;
         break ;
case 129: case 130: case 131: case 134: case 135: case 136: case 139: 
case 247: case 248: case 249: case 250: case 251: case 252: case 253:
case 254: case 255: /* unimplemented or illegal commands */
         error("! synch") ;
case 132: case 137: /* rules */
         {  integer ry, rx ;
            ry = signedquad() ; rx = signedquad() ;
            if (rx>0 && ry>0) {
               if (h < llx) llx = h ;
               if (v - ry < lly) lly = v - ry ;
               if (h + rx > urx) urx = h + rx ;
               if (v > ury) ury = v ;
            } else
               rx = 0 ;
            if (cmd != 137)
               h += rx ;
            break ;
         }
case 141: /* push */
         sp->h = h ; sp->v = v ;
         sp->w = w ; sp->x = x ; sp->y = y ; sp->z = z ;
         if (++sp >= &stack[STACKSIZE]) error("! Out of stack space") ;
         break ;
case 140: /* eop or end of virtual character */
         return ;
case 142: /* pop */
         if (--sp < stack) error("! More pops than pushes") ;
         h = sp->h ; v = sp->v ;
         w = sp->w ; x = sp->x ; y = sp->y ; z = sp->z ;
         break ;
case 143: h += signedbyte() ; break ;
case 144: h += signedpair() ; break ;
case 145: h += signedtrio() ; break ;
case 146: h += signedquad() ; break ;
case 147: h += w ; break ;
case 148: h += (w = signedbyte()) ; break ;
case 149: h += (w = signedpair()) ; break ;
case 150: h += (w = signedtrio()) ; break ;
case 151: h += (w = signedquad()) ; break ;
case 152: h += x ; break ;
case 153: h += (x = signedbyte()) ; break ;
case 154: h += (x = signedpair()) ; break ;
case 155: h += (x = signedtrio()) ; break ;
case 156: h += (x = signedquad()) ; break ;
case 157: v += signedbyte() ; break ;
case 158: v += signedpair() ; break ;
case 159: v += signedtrio() ; break ;
case 160: v += signedquad() ; break ;
case 161: v += y ; break ;
case 162: v += (y = signedbyte()) ; break ;
case 163: v += (y = signedpair()) ; break ;
case 164: v += (y = signedtrio()) ; break ;
case 165: v += (y = signedtrio()) ; break ;
case 166: v += z ; break ;
case 167: v += (z = signedbyte()) ; break ;
case 168: v += (z = signedpair()) ; break ;
case 169: v += (z = signedtrio()) ; break ;
case 170: v += (z = signedquad()) ; break ;
case 171: case 172: case 173: case 174: case 175: case 176: case 177:
case 178: case 179: case 180: case 181: case 182: case 183: case 184:
case 185: case 186: case 187: case 188: case 189: case 190: case 191:
case 192: case 193: case 194: case 195: case 196: case 197: case 198:
case 199: case 200: case 201: case 202: case 203: case 204: case 205:
case 206: case 207: case 208: case 209: case 210: case 211: case 212:
case 213: case 214: case 215: case 216: case 217: case 218: case 219:
case 220: case 221: case 222: case 223: case 224: case 225: case 226:
case 227: case 228: case 229: case 230: case 231: case 232: case 233:
case 234: case 235: case 236: case 237: case 238: /* font selection commands */
         if (cmd < 235) fnt = cmd - 171 ; /* fntnum0 thru fntnum63 */
         else {
            fnt = dvibyte() ;
            while (cmd-- > 235)
               fnt = (fnt << 8) + dvibyte() ;
         }
         curfnt = baseFonts[fnt] ;
         bbcurfnt = bbaseFonts[fnt] ;
         if (bbcurfnt == 0) {
            bbtfmload(fnt) ;
            bbcurfnt = bbaseFonts[fnt] ;
         }
         break ;
case 243: case 244: case 245: case 246: /*fntdef1 */
         skipover(cmd - 230) ;
         skipover(dvibyte() + dvibyte()) ;
         break ;
case 239: skipover((int)dvibyte()) ; break ;
case 240: skipover((int)twobytes()) ; break ;
case 241: skipover((int)threebytes()) ; break ;
case 242: skipover((int)signedquad()) ; break ;
      }
   }
}
void findbb() {
   integer curpos = ftell(dvifile) ;
   real conv = 72.0 * (real)num / (real)den * (real)mag / 254000000.0 ;
   real off = 72.0 / conv ;
   real margin = 1.0 / conv ;
   real vsize = 792.0 / conv ;

   fseek(dvifile, firstboploc, 0) ;
   bbdopage() ;
   fseek(dvifile, curpos, 0) ;
   lly = vsize - 2 * off - lly ;
   ury = vsize - 2 * off - ury ;
   llx = (int)floor((llx + off - margin) * conv + 0.5) ;
   lly = (int)floor((lly + off + margin) * conv + 0.5) ;
   urx = (int)floor((urx + off + margin) * conv + 0.5) ;
   ury = (int)floor((ury + off - margin) * conv + 0.5) ;
/* no marks on the page? */
   if (llx >= urx || lly <= ury)
      llx = lly = urx = ury = 72 ;
#ifdef SHORTINT
   sprintf(nextstring, "%ld %ld %ld %ld", llx, ury, urx, lly) ;
#else
   sprintf(nextstring, "%d %d %d %d", llx, ury, urx, lly) ;
#endif
}
