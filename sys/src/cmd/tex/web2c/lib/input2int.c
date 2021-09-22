/* input2int.c: read two or three integers from text files.  These
   routines are only used in patgen.  */

#include "config.h"


/* Read two integers from stdin.  */

void
zinput2ints P2C(integer *, a,  integer *, b)
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
zinput3ints P3C(integer *, a,  integer *, b,  integer *, c)
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
