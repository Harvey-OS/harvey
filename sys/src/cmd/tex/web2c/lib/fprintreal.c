/* fprintreal.c: print the real number R in the Pascal format N:M on the
   file F.  */

#include "config.h"

void
fprintreal (f, r, n, m)
  FILE *f;
  double r;
  int n, m;
{
  char fmt[50];  /* Surely enough, since N and M won't be more than 25
                    digits each!  */

  sprintf (fmt, "%%%d.%dlf", n, m);
  fprintf (f, fmt, r);
}
