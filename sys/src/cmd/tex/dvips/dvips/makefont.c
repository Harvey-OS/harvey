/*
 *   This software is Copyright 1988 by Radical Eye Software.
 */
#include "structures.h"
extern int quiet ;
extern int filter ;
extern int dontmakefont ;
extern int system() ;
extern char *getenv(), *newstring() ;
extern char *mfmode ;
/*
 *   Calculate magstep values.
 */
static int
magstep(n, bdpi)
register int n, bdpi ;
{
   register float t ;
   int neg = 0 ;

   if (n < 0) {
      neg = 1 ;
      n = -n ;
   }
   if (n & 1) {
      n &= ~1 ;
      t = 1.095445115 ;
   } else
      t = 1.0 ;
   while (n > 8) {
      n -= 8 ;
      t = t * 2.0736 ;
   }
   while (n > 0) {
      n -= 2 ;
      t = t * 1.2 ;
   }
   if (neg)
      return((int)(0.5 + bdpi / t)) ;
   else
      return((int)(0.5 + bdpi * t)) ;
}
#ifdef MSDOS
static char *defcommand = "command /c MakeTeXPK %n %d %b %m" ;
#else
#ifdef RESEARCH
static char *defcommand = TEXBIN "/MakeTeXPK %n %d %b %m" ;
#else
static char *defcommand = "MakeTeXPK %n %d %b %m" ;
#endif
#endif
char *command = 0 ;
/*
 *   This routine tries to create a font by executing a command, and
 *   then opening the font again if possible.
 */
static char buf[125] ;
void
makefont(name, dpi, bdpi)
   char *name ;
   int dpi, bdpi ;
{
   register char *p, *q ;
   register int m, n ;
#ifdef MSDOS
   double t;
#endif

   if (command == 0)
      if ((command=getenv("MAKETEXPK")))
         command = newstring(command) ;
      else
         command = defcommand ;
   for (p=command, q=buf; *p; p++)
      if (*p != '%')
         *q++ = *p ;
      else {
         switch (*++p) {
case 'n' : case 'N' :
            (void)strcpy(q, name) ;
            break ;
case 'd' : case 'D' :
            (void)sprintf(q, "%d", dpi) ;
            break ;
case 'b' : case 'B' :
            (void)sprintf(q, "%d", bdpi) ;
            break ;
case 'm' : case 'M' :
/*
 *   Here we want to return a string.  If we can find some integer
 *   m such that floor(0.5 + bdpi * 1.2 ^ (m/2)) = dpi, we write out
 *      magstep(m/2)
 *   where m/2 is a decimal number; else we write out
 *      dpi/bdpi
 *   We do this for the very slight improvement in accuracy that
 *   magstep() gives us over the rounded dpi/bdpi.
 */
            m = 0 ;
            if (dpi < bdpi) {
               while (1) {
                  m-- ;
                  n = magstep(m, bdpi) ;
                  if (n == dpi)
                     break ;
                  if (n < dpi || m < -40) {
                     m = 9999 ;
                     break ;
                  }
               }
            } else if (dpi > bdpi) {
               while (1) {
                  m++ ;
                  n = magstep(m, bdpi) ;
                  if (n == dpi)
                     break ;
                  if (n > dpi || m > 40) {
                     m = 9999 ;
                     break ;
                  }
               }
            }
#ifdef MSDOS
/* write out magnification as decimal number */
            if (m == 9999) {
               (void)sprintf(q, "%f", (double)dpi/bdpi) ;
            } else {
		if (m < 0) 
			n = -m;
		else
			n = m;
		if (n & 1) {
			n &= ~1 ;
			t = 1.095445115 ;
   		} else
			t = 1.0 ;
		while (n > 0) {
			n -= 2 ;
			t = t * 1.2 ;
		}
		if (m < 0)
			t = 1 / t ;
		(void)sprintf(q, "%12.9f", t) ;
            }
#else
#ifdef PLAN9
            if (m == 9999) {
               (void)sprintf(q, "'%d+%d/%d'", dpi/bdpi, dpi%bdpi, bdpi) ;
            } else if (m >= 0) {
               (void)sprintf(q, "'magstep(%d.%d)'", m/2, (m&1)*5) ;
            } else {
               (void)sprintf(q, "'magstep(-%d.%d)'", (-m)/2, (m&1)*5) ;
            }
#else
            if (m == 9999) {
               (void)sprintf(q, "%d+%d/%d", dpi/bdpi, dpi%bdpi, bdpi) ;
            } else if (m >= 0) {
               (void)sprintf(q, "magstep\\(%d.%d\\)", m/2, (m&1)*5) ;
            } else {
               (void)sprintf(q, "magstep\\(-%d.%d\\)", (-m)/2, (m&1)*5) ;
            }
#endif
#endif
            break ;
case 0 :    *q = 0 ;
            break ;
default:    *q++ = *p ;
            *q = 0 ;
            break ;
         }
         q += strlen(q) ;
      }
   *q = 0 ;
   if (mfmode) {
      strcpy(q, " ") ;
      strcat(q, mfmode) ;
   }
#ifndef MSDOS
   if (filter)
      (void)strcat(buf, quiet ? " >/dev/null" : " 1>&2") ;
#endif
   if (! quiet)
      (void)fprintf(stderr, "- %s\n", buf) ;
   if (dontmakefont == 0)
      (void)system(buf) ;
   else {
      static FILE *fontlog = 0 ;

      if (fontlog == 0) {
         fontlog = fopen("missfont.log", "a") ;
         if (fontlog != 0) {
            (void)fprintf(stderr,
                     "Appending font creation commands to missfont.log\n") ;
         }
      }
      if (fontlog != 0)
         (void)fprintf(fontlog, "%s\n", buf) ;
   }
}
