/* openinout.c: open input and output files.  These routines used by
   TeX, Metafont, and BibTeX.  */

#include "config.h"

#ifdef BibTeX
/* See comments in bibtex.ch for why we need these.  */
FILE *standardinput = stdin;
FILE *standardoutput = stdout;

/* Because we don't generate a .h file with all the global definitions
   for BibTeX, as we do with TeX and Metafont, we must declare these
   variables.  */
extern char nameoffile[];
extern integer namelength;

#else /* not BibTeX */

#define EXTERN /* Don't instantiate data here.  */

#ifdef TeX
#include "texd.h"
/* See comments in ctex.ch for why we need this.  */
extern integer tfmtemp;
#else /* Metafont */
#include "mfd.h"
#endif

#ifdef FUNNY_CORE_DUMP
/* This is defined in ./texmf.c.  */
extern void funny_core_dump ();
#endif /* FUNNY_CORE_DUMP */

#endif /* not BibTeX */



/* Open an input file F, using the path PATHSPEC.  The filename is in
   `nameoffile', as a Pascal string. We return whether or not the open
   succeeded.  If it did, we also set `namelength' to the length of the
   full pathname that we opened.  */

boolean
open_input (f, path_index)
  FILE **f;
  path_constant_type path_index;
{
  boolean openable = false;

#if defined (FUNNY_CORE_DUMP) && !defined (BibTeX)
  /* This only applies if a preloaded TeX (or Metafont) is being made;
     it allows for automatic creation of the core dump (typing ^\
     requires manual intervention).  */
  if (path_index == TEXINPUTPATH
      && strncmp (nameoffile+1, "HackyInputFileNameForCoreDump.tex", 33) == 0)
    funny_core_dump ();
#endif /* FUNNY_CORE_DUMP and not BibTeX */

  if (path_index == NO_FILE_PATH)
    {
      unsigned temp_length;

      null_terminate (nameoffile + 1);
      *f = fopen (nameoffile + 1, "r");
      temp_length = strlen (nameoffile + 1);
      space_terminate (nameoffile + 1);
      if (*f != NULL)
        {
          namelength = temp_length;
          openable = true;
        }
    }

  else if (testreadaccess (nameoffile, path_index))
    {
      unsigned i;

      /* We should always be able open `nameoffile', since
         `testreadaccess' just returned true.  */
      *f = xfopen_pas (nameoffile, "r");
      
      /* If we found the file in the current directory, don't leave the
         `./' at the beginning of `nameoffile', since it looks dumb when
         TeX says `(./foo.tex ... )', and analogously for Metafont.  */
      if (nameoffile[1] == '.' && nameoffile[2] == '/')
        {
          i = 1;
          while (nameoffile[i + 2] != ' ')
            {
              nameoffile[i] = nameoffile[i + 2];
              i++;
            }
          nameoffile[i] = ' ';
          namelength = i - 1;
        }
      else
        namelength = strchr (nameoffile + 1, ' ') - nameoffile - 1;
      
#ifdef TeX
      /* If we just opened a TFM file, we have to read the first byte,
         since TeX wants to look at it.  What a kludge.  */
      if (path_index == TFMFILEPATH)
        tfmtemp = getc (*f);
#endif /* TeX */

      openable = true;
    }

  return openable;
}


/* Open an output file F either in the current directory or in
   $TEXMFOUTPUT/F, if the environment variable `TEXMFOUTPUT' exists.
   (Actually, this applies to the BibTeX output files, also, but
   `TEXMFBIBOUTPUT' was just too long.)  The filename is in the global
   `nameoffile', as a Pascal string.  We return whether or not the open
   succeeded.  If it did, the global `namelength' is set to the length
   of the actual filename.  */

boolean
open_output (f)
  FILE **f;
{
  unsigned temp_length;
  boolean openable = false;

  null_terminate (nameoffile + 1);
  *f = fopen (nameoffile + 1, "w");
  
  if (*f == NULL)
    {
      /* Can't open in the current directory.  Try the directory
         specified by the environment variable.  */
      char *temp_dir = getenv ("TEXMFOUTPUT");

      if (temp_dir != NULL && chdir (temp_dir) == 0)
        *f = fopen (nameoffile + 1, "w");
    }

  temp_length = strlen (nameoffile + 1);
  space_terminate (nameoffile + 1);

  if (*f != NULL)
    {
      namelength = temp_length;
      openable = true;
    }
  
  return openable;
}



/* Test if the Pascal string BASE concatenated with the extension
   `.SUFFIX' is the same file as just BASE.  SUFFIX is a C string.  */

boolean
extensionirrelevantp (base, suffix)
  char *base;
  char *suffix;
{
  boolean ret;
  char temp[PATH_MAX];
  
  make_c_string (&base);
  strcpy (temp, base);
  strcat (temp, ".");
  strcat (temp, suffix);
  
  ret = same_file_p (base, temp);
  make_pascal_string (&base);
  
  return ret;
}
