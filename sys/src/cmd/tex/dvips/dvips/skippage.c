/*
 *   Skips over a page, collecting possible font definitions.  A very simple
 *   case statement insures we maintain sync with the dvi file by collecting
 *   the necessary parameters; but font definitions must be processed normally.
 */
#include "structures.h" /* The copyright notice in that file is included too! */
/*
 *   These are the external routines called.
 */
extern shalfword dvibyte() ;
extern halfword twobytes() ;
extern integer threebytes() ;
extern integer signedquad() ;
extern shalfword signedbyte() ;
extern shalfword signedpair() ;
extern integer signedtrio() ;
extern void skipover() ;
extern void fontdef() ;
extern void predospecial() ;
extern void error() ;
extern void bopcolor() ;
/*
 *   These are the external variables accessed.
 */
#ifdef DEBUG
extern integer debug_flag;
#endif  /* DEBUG */
extern integer pagenum ;
extern char errbuf[] ;
/*
 *   And now the big routine.
 */
void
skippage()
{
   register shalfword cmd ;
   register integer i ;

#ifdef DEBUG
   if (dd(D_PAGE))
#ifdef SHORTINT
   (void)fprintf(stderr,"Skipping page %ld\n", pagenum) ;
#else   /* ~SHORTINT */
   (void)fprintf(stderr,"Skipping page %d\n", pagenum) ;
#endif  /* ~SHORTINT */
#endif  /* DEBUG */
   skipover(40) ; /* skip rest of bop command */
   bopcolor(0) ;
   while ((cmd=dvibyte())!=140) {
     switch (cmd) {
/* illegal options */
case 129: case 130: case 131: case 134: case 135: case 136: case 139: 
case 247: case 248: case 249: case 250: case 251: case 252: case 253:
case 254: case 255:
         (void)sprintf(errbuf,
#ifdef SHORTINT
            "! DVI file contains unexpected command (%ld)",cmd) ;
#else   /* ~SHORTINT */
            "! DVI file contains unexpected command (%d)",cmd) ;
#endif  /* ~SHORTINT */
         error(errbuf) ;
/* eight byte commands */
case 132: case 137:
   cmd = dvibyte() ;
   cmd = dvibyte() ;
   cmd = dvibyte() ;
   cmd = dvibyte() ;
/* four byte commands */
case 146: case 151: case 156: case 160: case 165: case 170: case 238:
   cmd = dvibyte() ;
/* three byte commands */
case 145: case 150: case 155: case 159: case 164: case 169: case 237:
   cmd = dvibyte() ;
/* two byte commands */
case 144: case 149: case 154: case 158: case 163: case 168: case 236:
   cmd = dvibyte() ;
/* one byte commands */
case 128: case 133: case 143: case 148: case 153: case 157: case 162:
case 167: case 235:
   cmd = dvibyte() ;
   break ;
/* specials */
case 239: i = dvibyte() ; predospecial(i, 0) ; break ;
case 240: i = twobytes() ; predospecial(i, 0) ; break ;
case 241: i = threebytes() ; predospecial(i, 0) ; break ;
case 242: i = signedquad() ; predospecial(i, 0) ; break ;
/* font definition */
case 243: case 244: case 245: case 246:
   fontdef(cmd - 242) ;
   break ;
default: ;
      }
   }
}
