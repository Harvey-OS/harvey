/*
 *   Main page drawing procedure.  Interprets the page commands.  A simple
 *   (if lengthy) case statement interpreter.  
 */
#include "dvips.h" /* The copyright notice in that file is included too! */
#include <math.h>
/*
 *   The external routines we use:
 */
extern void error() ;
extern void pageinit() ;
extern void drawrule() ;
extern shalfword dvibyte() ;
extern halfword twobytes() ;
extern integer threebytes() ;
extern integer signedquad() ;
extern shalfword signedbyte() ;
extern shalfword signedpair() ;
extern integer signedtrio() ;
extern void dospecial() ;
extern void drawchar() ;
extern integer scalewidth() ;
extern void skipover() ;
extern void bopcolor() ; /* IBM: color */
extern void fontdef() ;
extern void pageend() ;
/*
 *   Now the external variables.
 */
extern fontdesctype *curfnt ;
extern fontdesctype *baseFonts[] ;
extern fontmaptype *ffont ;
extern quarterword *curpos, *curlim ;
extern integer hh, vv ;
extern integer hoff, voff ;
/*
 * CONVENTION:  conv -> horizontial converter
 *		vconv -> vertical converter
 */
extern real conv ;
extern real vconv ;

extern FILE *bitfile ;
extern int actualdpi ;
extern int vactualdpi ;
extern frametype frames[] ;
extern int maxdrift ;
extern int vmaxdrift ;
#ifdef EMTEX
extern void emclear() ;
#endif

#ifdef XENIX
#define PixRound(x) ((integer)(x + (iconv >> 1)) / iconv)
#define VPixRound(x) ((integer)(x + (viconv >> 1)) / viconv)
#else
#ifdef __THINK__
#define  PixRound(x) ((integer)(x +  (iconv >> 1)) /  iconv)
#define VPixRound(x) ((integer)(x + (viconv >> 1)) / viconv)
#else
#define PixRound(x) ((integer)(floor(((x) * conv) + 0.5)))
#define VPixRound(x) ((integer)(floor(((x) * vconv) + 0.5)))
#endif
#endif
/*
 *   Now we have the dopage procedure.
 *   Most error checking is suppressed because the prescan has already
 *   verified that the DVI data is OK....except for stack over/underflow.
 */
struct dvistack {
  integer hh, vv ;
  integer h, v, w, x, y, z ;
} stack[STACKSIZE] ;
void
dopage()
{
   register shalfword cmd ;
   register integer p ;
   register chardesctype *cd ;
   register integer h ;
   register fontmaptype *cfnt ;
   register frametype *frp = frames ;
   integer fnt ;
   int charmove ;
   struct dvistack *sp = stack ;
   integer v, w, x, y, z ;
   integer roundpos ;
   integer thinspace ;
   integer vertsmallspace ;
#ifdef XENIX
   integer iconv ;
   integer viconv ;

   iconv = (integer)(1.0 / conv + 0.5) ;
   viconv = (integer)(1.0 / vconv + 0.5) ;
#else
#ifdef __THINK__
   integer  iconv ;
   integer viconv ;

    iconv = (integer)(1.0 /  conv + 0.5) ;
   viconv = (integer)(1.0 / vconv + 0.5) ;
#endif
#endif
#ifdef EMTEX
   emclear() ;
#endif
   pageinit() ;

   bopcolor(1) ;
   thinspace =  (integer)(0.025*DPI/conv) ; /* 0.025 inches */
   vertsmallspace = (integer)(0.025*VDPI/vconv) ; /* 0.025 inches */

   w = x = y = z = 0 ;
   h = DPI / conv * hoff / 4736286.72 ;
   v = DPI / conv * voff / 4736286.72 ;
   hh = PixRound(h) ;
   vv = PixRound(v) ;
   curfnt = NULL ;
   curpos = NULL ;
   charmove = 0 ;
beginloop:
   switch (cmd=dvibyte()) {
case 138: goto beginloop ; /* nop command does nuttin */
/*
 *   For put1 commands, we subtract the width of the character before
 *   dropping through to the normal character setting routines.  This
 */
case 133: /* put1 */
   cmd = dvibyte() ;
   charmove = 0 ;
   goto dochar ;
case 128: cmd = dvibyte() ; /* set1 command drops through to setchar */
default: /* these are commands 0 (setchar0) thru 127 (setchar127) */
   charmove = 1 ;
dochar:
   cd = &(curfnt->chardesc[cmd]) ;
   if (cd->flags & EXISTS) {
      if (curfnt->loaded == 2) { /* virtual character being typeset */
         if (charmove) {
            sp->hh = hh + cd->pixelwidth ;
            sp->h = h + cd->TFMwidth ;
         } else {
            sp->hh = hh ; sp->h = h ;
         }
         sp->vv = vv ; sp-> v = v ;
         sp->w = w ; sp->x = x ; sp->y = y ; sp->z = z ;
         if (++sp >= &stack[STACKSIZE]) error("! Out of stack space") ;
         w = x = y = z = 0 ; /* will be in relative units at new stack level */
         frp->curp = curpos ;
         frp->curl = curlim ;
         frp->ff = ffont ;
         frp->curf = curfnt ;
         if (++frp == &frames[MAXFRAME] )
            error("! virtual recursion stack overflow") ;
         curpos = cd->packptr + 2 ;
         curlim = curpos + (256*(long)(*cd->packptr)+(*(cd->packptr+1))) ;
         ffont = curfnt->localfonts ;
         if (ffont) {
            curfnt = ffont->desc ;
            thinspace = curfnt->thinspace ;
         } else {
            curfnt = NULL ;
            thinspace = vertsmallspace ;
         }
         goto beginloop ;
      }
      drawchar(cd, cmd) ;
   }
   if (charmove) {
      h += cd->TFMwidth ;
      hh += cd->pixelwidth ;
   }
   goto setmotion ;
case 129: case 130: case 131: case 134: case 135: case 136: case 139: 
case 247: case 248: case 249: case 250: case 251: case 252: case 253:
case 254: case 255: /* unimplemented or illegal commands */
   error("! synch") ;
case 132: case 137: /* rules */
 { integer ry, rx , rxx, ryy ;
   ry = signedquad() ; rx = signedquad() ;
   if (rx>0 && ry>0) {
      if (curpos) {
         rx = scalewidth(rx, (frp-1)->curf->scaledsize) ;
         ry = scalewidth(ry, (frp-1)->curf->scaledsize) ;
      }
      rxx = (int)(conv * rx + 0.9999999) ;
      ryy = (int)(vconv * ry + 0.9999999) ;
      drawrule(rxx, ryy) ;
   } else
      rxx = 0 ;
   if (cmd == 137) goto beginloop ;
   h += rx ; hh += rxx ;
   goto setmotion ;
 }
case 141: /* push */
   sp->hh = hh ; sp->vv = vv ; sp->h = h ; sp->v = v ;
   sp->w = w ; sp->x = x ; sp->y = y ; sp->z = z ;
   if (++sp >= &stack[STACKSIZE]) error("! Out of stack space") ;
   goto beginloop ;
case 140: /* eop or end of virtual character */
   if (curpos == NULL) break ; /* eop */
   --frp ;
   curfnt = frp->curf ;
   thinspace = (curfnt) ? curfnt->thinspace : vertsmallspace ;
   ffont = frp->ff ;
   curlim = frp->curl ;
   curpos = frp->curp ;
   if (hh < (sp-1)->hh+2 && hh > (sp-1)->hh-2)
      (sp-1)->hh = hh; /* retain `intelligence' of pixel width, if close */ 
   /* falls through */
case 142: /* pop */
   if (--sp < stack) error("! More pops than pushes") ;
   hh = sp->hh ; vv = sp->vv ; h = sp->h ; v = sp->v ;
   w = sp->w ; x = sp->x ; y = sp->y ; z = sp->z ;
   goto beginloop ;
case 143: /* right1 */
   p = signedbyte() ; goto horizontalmotion ;
case 144: /* right2 */
   p = signedpair() ; goto horizontalmotion ;
case 145: /* right3 */
   p = signedtrio() ; goto horizontalmotion ;
case 146: /* right4 */
   p = signedquad() ; goto horizontalmotion ;
case 147: /* w0 */
   p = w ; goto horizontalmotion ;
case 148: /* w1 */
   p = w = signedbyte() ; goto horizontalmotion ;
case 149: /* w2 */
   p = w = signedpair() ; goto horizontalmotion ;
case 150: /* w3 */
   p = w = signedtrio() ; goto horizontalmotion ;
case 151: /* w4 */
   p = w = signedquad() ; goto horizontalmotion ;
case 152: /* x0 */
   p = x ; goto horizontalmotion ;
case 153: /* x1 */
   p = x = signedbyte() ; goto horizontalmotion ;
case 154: /* x2 */
   p = x = signedpair() ; goto horizontalmotion ;
case 155: /* x3 */
   p = x = signedtrio() ; goto horizontalmotion ;
case 156: /* x4 */
   p = x = signedquad() ; goto horizontalmotion ;
case 157: /* down1 */
   p = signedbyte() ; goto verticalmotion ;
case 158: /* down2 */
   p = signedpair() ; goto verticalmotion ;
case 159: /* down3 */
   p = signedtrio() ; goto verticalmotion ;
case 160: /* down4 */
   p = signedquad() ; goto verticalmotion ;
case 161: /* y0 */
   p = y ; goto verticalmotion ;
case 162: /* y1 */
   p = y = signedbyte() ; goto verticalmotion ;
case 163: /* y2 */
   p = y = signedpair() ; goto verticalmotion ;
case 164: /* y3 */
   p = y = signedtrio() ; goto verticalmotion ;
case 165: /* y4 */
   p = y = signedquad() ; goto verticalmotion ;
case 166: /* z0 */
   p = z ; goto verticalmotion ;
case 167: /* z1 */
   p = z = signedbyte() ; goto verticalmotion ;
case 168: /* z2 */
   p = z = signedpair() ; goto verticalmotion ;
case 169: /* z3 */
   p = z = signedtrio() ; goto verticalmotion ;
case 170: /* z4 */
   p = z = signedquad() ; goto verticalmotion ;
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
   if (curpos || fnt > 255) {
      for (cfnt=ffont; cfnt; cfnt = cfnt->next)
         if (cfnt->fontnum == fnt) break ;
      curfnt = cfnt->desc ;
   } else
      curfnt = baseFonts[fnt] ;
   thinspace = curfnt->thinspace ;
   goto beginloop ;
case 243: case 244: case 245: case 246: /*fntdef1 */
   skipover(cmd - 230) ;
   skipover(dvibyte() + dvibyte()) ;
   goto beginloop ;
case 239: /* xxx1 */
   p = dvibyte() ;
   dospecial(p) ;
   goto beginloop ;
case 240: /* xxx2 */
   p = twobytes() ;
   dospecial(p) ;
   goto beginloop ;
case 241: /* xxx3 */
   p = threebytes() ;
   dospecial(p) ;
   goto beginloop ;
case 242: /* xxx4 */
   p = signedquad() ;
   dospecial(p) ;
   goto beginloop ;

/*
 *   The calculations here are crucial to the appearance of the document.
 *   If the motion is small, we round the amount of relative motion; otherwise,
 *   we update the position and round the new position.  Then we check to
 *   insure that the rounded position didn't accumulate an error that was
 *   greater than maxdrift.
 */
verticalmotion:
/* vertical motion cases */
      if (curpos)
         p = scalewidth(p, (frp-1)->curf->scaledsize) ;
      v += p ;
      if (p >= vertsmallspace) vv = VPixRound(v) ;
      else if (p <= -vertsmallspace) vv = VPixRound(v) ;
      else 
      { vv += VPixRound(p) ;
        roundpos = VPixRound(v) ;
        if (roundpos - vv > vmaxdrift) vv = roundpos - vmaxdrift ;
        else if (vv - roundpos > vmaxdrift) vv = roundpos + vmaxdrift ;
      }
      goto beginloop ;
/*
 *   Horizontal motion is analogous. We know the exact width of each
 *   character in pixels. Kerning is distinguished from space between
 *   words if it's less than a thinspace and not more negative than would
 *   occur when an accent is being positioned by backspacing.
 */
horizontalmotion:
/* horizontal motion cases */
      if (curpos)
         p = scalewidth(p, (frp-1)->curf->scaledsize) ;
      h += p ;
      if (p >= thinspace || p <= -6 * thinspace) {
         hh = PixRound(h) ; goto beginloop ;
      }
      else hh += PixRound(p) ;
setmotion:
      roundpos = PixRound(h) ;
      if (roundpos - hh > maxdrift) { hh = roundpos - maxdrift ; }
      else if (hh - roundpos > maxdrift) { hh = roundpos + maxdrift ; }
goto beginloop ;

   } /* end of the big switch */
   pageend() ;
}
