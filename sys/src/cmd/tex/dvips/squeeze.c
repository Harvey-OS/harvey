/*
 *   This software is Copyright 1988 by Radical Eye Software.
 */
/*
 *   This routine squeezes a PostScript file down to its
 *   minimum.  We parse and then output it.
 */
#include <stdio.h>
#define LINELENGTH (72)
#define BUFLENGTH (1000)
#undef putchar
#define putchar(a) (void)putc(a, out) ;
FILE *in, *out ;
static int linepos = 0 ;
static int lastspecial = 1 ;
extern int strlen() ;
/*
 *   This next routine writes out a `special' character.  In this case,
 *   we simply put it out, since any special character terminates the
 *   preceding token.
 */
void specialout(c)
char c ;
{
   if (linepos + 1 > LINELENGTH) {
      putchar('\n') ;
      linepos = 0 ;
   }
   putchar(c) ;
   linepos++ ;
   lastspecial = 1 ;
}
void strout(s)
char *s ;
{
   if (linepos + strlen(s) > LINELENGTH) {
      putchar('\n') ;
      linepos = 0 ;
   }
   linepos += strlen(s) ;
   while (*s != 0)
      putchar(*s++) ;
   lastspecial = 1 ;
}
void cmdout(s)
char *s ;
{
   int l ;

   l = strlen(s) ;
   if (linepos + l + 1 > LINELENGTH) {
      putchar('\n') ;
      linepos = 0 ;
      lastspecial = 1 ;
   }
   if (! lastspecial) {
      putchar(' ') ;
      linepos++ ;
   }
   while (*s != 0) {
      putchar(*s++) ;
   }
   linepos += l ;
   lastspecial = 0 ;
}
char buf[BUFLENGTH] ;
#ifndef VMS
void
#endif
main(argc, argv)
int argc ;
char *argv[] ;
{
   int c ;
   char *b ;
   char seeking ;
   extern void exit() ;

   if (argc > 3 || (in=(argc < 2 ? stdin : fopen(argv[1], "r")))==NULL ||
                    (out=(argc < 3 ? stdout : fopen(argv[2], "w")))==NULL) {
      (void)fprintf(stderr, "Usage:  squeeze [infile [outfile]]\n") ;
      exit(1) ;
   }
   (void)fprintf(out, "%%!\n") ;
   while (1) {
      c = getc(in) ;
      if (c==EOF)
         break ;
      if (c=='%') {
         while ((c=getc(in))!='\n') ;
      }
      if (c <= ' ')
         continue ;
      switch (c) {
case '{' :
case '}' :
case '[' :
case ']' :
         specialout(c) ;
         break ;
case '<' :
case '(' :
         if (c=='(')
            seeking = ')' ;
         else
            seeking = '>' ;
         b = buf ;
         *b++ = c ;
         do {
            c = getc(in) ;
            if (b > buf + BUFLENGTH-2) {
               (void)fprintf(stderr, "Overran buffer seeking %c", seeking) ;
               exit(1) ;
            }
            *b++ = c ;
            if (c=='\\')
               *b++ = getc(in) ;
         } while (c != seeking) ;
         *b++ = 0 ;
         strout(buf) ;
         break ;
default:
         b = buf ;
         while ((c>='A'&&c<='Z')||(c>='a'&&c<='z')||
                (c>='0'&&c<='9')||(c=='/')||(c=='@')||
                (c=='!')||(c=='"')||(c=='&')||(c=='*')||(c==':')||
                (c==',')||(c==';')||(c=='?')||(c=='^')||(c=='~')||
                (c=='-')||(c=='.')||(c=='#')||(c=='|')||(c=='_')||
                (c=='=')||(c=='$')||(c=='+')) {
            *b++ = c ;
            c = getc(in) ;
         }
         if (b == buf) {
            (void)fprintf(stderr, "Oops!  Missed a case: %c.\n", c) ;
            exit(1) ;
         }
         *b++ = 0 ;
         (void)ungetc(c, in) ;
         cmdout(buf) ;
      }
   }
   if (linepos != 0)
      putchar('\n') ;
   exit(0) ;
   /*NOTREACHED*/
}
