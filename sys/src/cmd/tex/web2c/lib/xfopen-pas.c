/* xfopen-pas.c: Open a file; don't return if any error occurs.  NAME
   should be a Pascal string; it is changed to a C string and then
   changed back.  */

#include "config.h"

FILE *
xfopen_pas (name, mode)
  char *name;
  char *mode;
{
  FILE *result;
  char *cp;

  make_c_string (&name);
  result = fopen (name, mode);

  if (result != NULL)
    {
      make_pascal_string (&name);
      return result;
    }
  
  FATAL_PERROR (name);
  return NULL; /* Stop compiler warnings.  */
}
