/* uexit.c: define uexit to do an exit with the right status.  We can't
   just call `exit' from the web files, since the webs use `exit' as a
   loop label.  */

#include "config.h"

void
uexit (unix_code)
  int unix_code;
{
  int final_code;
  
  if (unix_code == 0)
    final_code = EXIT_SUCCESS_CODE;
  else if (unix_code == 1)
    final_code = !EXIT_SUCCESS_CODE;
  else
    final_code = unix_code;
  
  exit (final_code);
}
