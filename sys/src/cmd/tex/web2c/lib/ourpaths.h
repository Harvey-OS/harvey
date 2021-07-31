/* ourpaths.h: declarations for path searching.  The `...PATH' constants
   are used both as indices and just as enumeration values.  Their
   values must match the initialization of `env_var_names'.  The
   `...PATHBIT' constants are used in the argument to `setpaths'.  */

#ifndef OURPATHS_H
#define OURPATHS_H

typedef enum
{
  /* This constant is used in BibTeX, when opening the top-level .aux file.  */
  NO_FILE_PATH = -1,
  BIBINPUTPATH,
  BSTINPUTPATH,
  GFFILEPATH,
  MFBASEPATH,
  MFINPUTPATH,
  MFPOOLPATH,
  PKFILEPATH,
  TEXFORMATPATH,
  TEXINPUTPATH,
  TEXPOOLPATH,
  TFMFILEPATH,
  VFFILEPATH,
  LAST_PATH,
} path_constant_type;

#define BIBINPUTPATHBIT (1 << BIBINPUTPATH)
#define BSTINPUTPATHBIT (1 << BSTINPUTPATH)
#define GFFILEPATHBIT	(1 << GFFILEPATH)
#define MFBASEPATHBIT	(1 << MFBASEPATH)
#define MFINPUTPATHBIT	(1 << MFINPUTPATH)
#define MFPOOLPATHBIT	(1 << MFPOOLPATH)
#define PKFILEPATHBIT	(1 << PKFILEPATH)
#define TEXFORMATPATHBIT (1 << TEXFORMATPATH)
#define TEXINPUTPATHBIT (1 << TEXINPUTPATH)
#define TEXPOOLPATHBIT	(1 << TEXPOOLPATH)
#define TFMFILEPATHBIT	(1 << TFMFILEPATH)
#define VFFILEPATHBIT	(1 << VFFILEPATH)

#endif /* not OURPATHS_H */
