/* openclose.c: open and close files for TeX, Metafont, and BibTeX.

   Written 1995, 96 Karl Berry.  Public domain.  */

#include "config.h"
#include <kpathsea/c-pathch.h>
#include <kpathsea/tex-file.h>
#include <kpathsea/variable.h>

/* The globals we use to communicate.  */
extern string nameoffile;
extern unsigned namelength;

/* For TeX and MetaPost.  See below.  Always defined so we don't have to
   #ifdef, and thus this file can be compiled once and go in lib.a.  */
int tfmtemp;
int ocptemp;
int texinputtype;

/* Open an input file F, using the kpathsea format FILEFMT and passing
   FOPEN_MODE to fopen.  The filename is in `nameoffile+1'.  We return
   whether or not the open succeeded.  If it did, `nameoffile' is set to
   the full filename opened, and `namelength' to its length.  */

boolean
open_input P3C(FILE **, f_ptr,  kpse_file_format_type, filefmt,
               const_string, fopen_mode)
{
#ifdef FUNNY_CORE_DUMP
  /* This only applies if a preloaded TeX/Metafont is being made;
     it allows automatic creation of the core dump (typing ^\ loses
     since that requires manual intervention).  */
  if ((filefmt == kpse_tex_format || filefmt == kpse_mf_format
       || filefmt == kpse_mp_format)
      && STREQ (nameoffile + 1, "HackyInputFileNameForCoreDump.tex"))
    funny_core_dump ();
#endif

  /* A negative FILEFMT means don't use a path.  */
  if ((int) filefmt < 0)
    { /* no_file_path, for BibTeX .aux files and MetaPost things.  */
      *f_ptr = fopen (nameoffile + 1, fopen_mode);
    }
  else 
    {
      /* The only exception to `must_exist' being true is \openin, for
         which we set `tex_input_type' to 1 in the change file.  */
      boolean must_exist = filefmt != kpse_tex_format || texinputtype;
      string fname = kpse_find_file (nameoffile + 1, filefmt, must_exist);
      
      if (fname)
        {
          /* If we found the file in the current directory, don't leave
             the `./' at the beginning of `nameoffile', since it looks
             dumb when `tex foo' says `(./foo.tex ... )'.  On the other
             hand, if the user said `tex ./foo', and that's what we
             opened, then keep it -- the user specified it, so we
             shouldn't remove it.  */
          if (fname[0] == '.' && IS_DIR_SEP (fname[1])
              && (nameoffile[1] != '.' || !IS_DIR_SEP (nameoffile[2])))
            {
              unsigned i = 0;
              while (fname[i + 2] != 0)
                {
                  fname[i] = fname[i + 2];
                  i++;
                }
              fname[i] = 0;
            }
          
          if (nameoffile + 1 != fname)
            {
              free (nameoffile);
              namelength = strlen (fname);
              nameoffile = xmalloc (namelength + 2);
              strcpy (nameoffile + 1, fname);
              free (fname);
            }
          
          *f_ptr = xfopen (nameoffile + 1, fopen_mode);
          
          /* If we just opened a TFM file, we have to read the first byte,
             to pretend we're Pascal.  See tex.ch and mp.ch.  Ditto for
             the ocp/ofm Omega file formats.  */
          if (filefmt == kpse_tfm_format) {
            tfmtemp = getc (*f_ptr);
            /* We intentionally do not check for EOF here, i.e., an
               empty TFM file.  TeX will see the 255 byte and complain
               about a bad TFM file, which is what we want.  */
          } else if (filefmt == kpse_ocp_format) {
            ocptemp = getc (*f_ptr);
          } else if (filefmt == kpse_ofm_format) {
            tfmtemp = getc (*f_ptr);
          }
        }
      else
        *f_ptr = NULL;
    } /* search path case */

  return *f_ptr != NULL;
}

/* Open an output file F either in the current directory or in
   $TEXMFOUTPUT/F, if the environment variable `TEXMFOUTPUT' exists.
   (Actually, this also applies to the BibTeX and MetaPost output files,
   but `TEXMFMPBIBOUTPUT' was just too long.)  The filename is in the
   global `nameoffile' + 1.  We return whether or not the open
   succeeded.  If it did, `nameoffile' is reset to the name opened if
   necessary, and `namelength' to its length.  */

boolean
open_output P2C(FILE **, f_ptr,  const_string, fopen_mode)
{
  /* Is the filename openable as given?  */
  *f_ptr = fopen (nameoffile + 1, fopen_mode);

  if (! *f_ptr)
    { /* Can't open as given.  Try the envvar.  */
      string texmfoutput = kpse_var_value ("TEXMFOUTPUT");

      if (texmfoutput && *texmfoutput)
        {
          string fname = concat3 (texmfoutput, DIR_SEP_STRING, nameoffile + 1);
          *f_ptr = fopen (fname, fopen_mode);
          
          /* If this succeeded, change nameoffile accordingly.  */
          if (*f_ptr)
            {
              free (nameoffile);
              namelength = strlen (fname);
              nameoffile = xmalloc (namelength + 2);
              strcpy (nameoffile + 1, fname);
            }

          free (fname);
        }
    }

  return *f_ptr != NULL;
}

/* Close F.  */

void
aclose P1C(FILE *, f)
{
  /* If F is null, just return.  bad_pool might close a file that has
     never been opened.  */
  if (!f)
    return;
    
  if (fclose (f) == EOF) {
    /* It's not always nameoffile, we might have opened something else
       in the meantime.  And it's not easy to extract the filenames out
       of the pool array.  So just punt on the filename.  Sigh.  This
       probably doesn't need to be a fatal error.  */
    perror ("fclose");
  }
}
