/* eofeoln.c: implement Pascal's ideas for end-of-file and end-of-line
   testing.  */

#include "config.h"


/* Return true if we're at the end of FILE, else false.  This implements
   Pascal's `eof' builtin.  */

boolean
eof P1C(FILE *, file)
{
  register int c;

  /* If FILE doesn't exist, return false. This happens when, for
     example, when a user does `mft foo.mf' -- there's no change file,
     so we never open it, so we end up calling this with a null pointer. */
  if (!file)
    return true;
    
  /* Maybe we're already at the end?  */
  if (feof (file))
    return true;

  if ((c = getc (file)) == EOF)
    return true;

  /* We weren't at the end.  Back up.  */
  (void) ungetc (c, file);

  return false;
}


/* Return true on end-of-line in FILE or at the end of FILE, else false.  */

boolean
eoln P1C(FILE*, file)
{
  register int c;

  if (feof (file))
    return true;
  
  c = getc (file);
  
  if (c != EOF)
    (void) ungetc (c, file);
    
  return c == '\n' || c == EOF;
}
