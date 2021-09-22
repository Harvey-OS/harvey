/* printversion.c: Output for the standard GNU option --version.

   Written in 1996 by K. Berry.  Public domain.  */

#include "config.h"

/* We're passed in the original WEB banner string, which has the form
This is PROGRAM, Version VERSION-NUMBER
   We parse the PROGRAM and VERSION-NUMBER out of this.
   
   If COPYRIGHT_HOLDER is specified and AUTHOR isn't, then use the
   former for the latter.  If AUTHOR is specified and COPYRIGHT_HOLDER
   isn't, it means the original program is public domain.
   
   Maybe I should have just done it all inline in each individual
   program, but tangle doesn't allow multiline string constants ...  */

void
printversionandexit P3C(const_string, banner,
                        const_string, copyright_holder,  const_string, author)
{
  extern string versionstring;           /* from web2c/lib/version.c */
  extern DllImport string kpathsea_version_string;/* from kpathsea/version.c */
  string prog_name;
  unsigned len;
  const_string prog_name_end = strchr (banner, ',');
  const_string prog_version = strrchr (banner, ' ');
  assert (prog_name_end && prog_version);
  prog_version++;
  
  len = prog_name_end - banner - sizeof ("This is");
  prog_name = xmalloc (len + 1);
  strncpy (prog_name, banner + sizeof ("This is"), len);
  prog_name[len] = 0;

  /* The Web2c version string starts with a space.  */
  printf ("%s%s %s\n", prog_name, versionstring, prog_version);
  puts (kpathsea_version_string);

  if (copyright_holder) {
    printf ("Copyright (C) 1998 %s.\n", copyright_holder);
    if (!author)
      author = copyright_holder;
  }

  puts ("There is NO warranty.  You may redistribute this software");
  fputs ("under the terms of the GNU General Public License", stdout);
  /* DVIcopy is GPL'd, so no additional words needed. */
  if (copyright_holder && !STREQ (prog_name, "DVIcopy")) {
    printf ("\nand the %s copyright", prog_name);
  }
  puts (".");
  puts ("For more information about these matters, see the files");
  printf ("named COPYING and the %s source.\n", prog_name);
  printf ("Primary author of %s: %s.\n", prog_name, author);

  uexit (0);
}
