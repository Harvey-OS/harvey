/* ourpaths.c: path searching.  */

#include "config.h"
#include "pathsrch.h"


/* `path_dirs' is initialized in `setpaths', to a null-terminated array
   of directories to search for.  */
static string *path_dirs[LAST_PATH];


/* This sets up the paths, by either copying from an environment variable
   or using the default path, which is defined as a preprocessor symbol
   (with the same name as the environment variable) in `site.h'.  The
   parameter PATH_BITS is a logical or of the paths we need to set.  */

extern void
setpaths (path_bits)
  int path_bits;
{
  if (path_bits & BIBINPUTPATHBIT)
    path_dirs[BIBINPUTPATH] = initialize_path_list ("BIBINPUTS", BIBINPUTS);

  if (path_bits & BSTINPUTPATHBIT)
    {
      string bst_var = getenv ("BSTINPUTS") ? "BSTINPUTS" : "TEXINPUTS";
      path_dirs[BSTINPUTPATH] = initialize_path_list (bst_var, BSTINPUTS);
    }

  if (path_bits & GFFILEPATHBIT)
    {
      string gf_var = getenv ("GFFONTS") ? "GFFONTS" : "TEXFONTS";
      path_dirs[GFFILEPATH] = initialize_path_list (gf_var, GFFONTS);
    }

  if (path_bits & MFBASEPATHBIT)
    path_dirs[MFBASEPATH] = initialize_path_list ("MFBASES", MFBASES);

  if (path_bits & MFINPUTPATHBIT)
    path_dirs[MFINPUTPATH] = initialize_path_list ("MFINPUTS", MFINPUTS);

  if (path_bits & MFPOOLPATHBIT)
    path_dirs[MFPOOLPATH] = initialize_path_list ("MFPOOL", MFPOOL);

  if (path_bits & PKFILEPATHBIT)
    {
      string pk_var = getenv ("PKFONTS") ? "PKFONTS"
                      : getenv ("TEXPKS") ? "TEXPKS" : "TEXFONTS";
      path_dirs[PKFILEPATH] = initialize_path_list (pk_var, PKFONTS);
    }

  if (path_bits & TEXFORMATPATHBIT)
    path_dirs[TEXFORMATPATH]
      = initialize_path_list ("TEXFORMATS", TEXFORMATS);

  if (path_bits & TEXINPUTPATHBIT)
    path_dirs[TEXINPUTPATH]
      = initialize_path_list ("TEXINPUTS", TEXINPUTS);

  if (path_bits & TEXPOOLPATHBIT)
    path_dirs[TEXPOOLPATH] = initialize_path_list ("TEXPOOL", TEXPOOL);

  if (path_bits & TFMFILEPATHBIT)
      path_dirs[TFMFILEPATH] = initialize_path_list ("TEXFONTS", TEXFONTS);

  if (path_bits & VFFILEPATHBIT)
    {
      string vf_var = getenv ("VFFONTS") ? "VFFONTS" : "TEXFONTS";
      path_dirs[VFFILEPATH] = initialize_path_list (vf_var, VFFONTS);
    }
}


/* Look for NAME, a Pascal string, in the colon-separated list of
   directories given by `path_dirs[PATH_INDEX]'.  If the search is
   successful, leave the full pathname in NAME (which therefore must
   have enough room for such a pathname), padded with blanks.
   Otherwise, or if NAME is an absolute or relative pathname, just leave
   it alone.  */

boolean
testreadaccess (name, path_index)
  char *name;
  int path_index;
{
  string found;  
  
  make_c_string (&name);

  found = find_path_filename (name, path_dirs[path_index]);
  if (found != NULL)
    strcpy (name, found);
  
  make_pascal_string (&name);
  
  return found != NULL;
}
