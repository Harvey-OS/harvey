/* inputint.c: read integers from text files.  These routines are only
   used for debugging and such, so perfect error checking isn't
   necessary.  */

#include "config.h"


/* Read an integer from the file F, reading past the subsequent end of
   line.  */

integer
inputint (f)
  FILE *f;
{
  char buffer[50]; /* Long enough for anything reasonable.  */

  return
    fgets (buffer, sizeof (buffer), f)
    ? atoi (buffer)
    : 0;
}


/* Read two integers from stdin.  */

void
zinput2ints (a, b)
  integer *a, *b;
{
  int ch;

  while (scanf ("%ld %ld", a, b) != 2)
    {
      while ((ch = getchar ()) != EOF && ch != '\n');
      if (ch == EOF) return;
      (void) fprintf (stderr, "Please enter two integers.\n");
    }

  while ((ch = getchar ()) != EOF && ch != '\n');
}


/* Read three integers from stdin.  */

void
zinput3ints (a,b,c)
  integer *a, *b, *c;
{
  int ch;

  while (scanf ("%ld %ld %ld", a, b, c) != 3)
    {
      while ((ch = getchar ()) != EOF && ch != '\n');
      if (ch == EOF) return;
      (void) fprintf (stderr, "Please enter three integers.\n");
    }

  while ((ch = getchar ()) != EOF && ch != '\n');
}
