/* usage.c: Output a help message (from help.h).

   Written in 1995 by K. Berry.  Public domain.  */

#include "config.h"

/* We're passed in zero for STATUS if this is from --help, else nonzero
   if it was from some kind of error.  In the latter case, we say `See
   HELP_STR --help for more information.', i.e., STR is supposed to be
   the program name.  */

void
usage P2C(int, status,  const_string, str)
{
  if (status == 0) {
    extern DllImport char *kpse_bug_address;
    fputs (str, stdout);
    putchar ('\n');
    fputs (kpse_bug_address, stdout);
  } else {
    fprintf (stderr, "Try `%s --help' for more information.\n", str);
  }
  uexit (status);
}
