/* External procedures for these programs.

   02/04/90 Karl Berry
*/

#include "extra.h" /* Includes <stdio.h> and "site.h".  */

#if defined(SYSV) || defined(_POSIX_SOURCE)
#define index strchr
#define rindex strrchr
#endif

extern char *getenv ();
extern char *index (), *rindex ();


static char **gargv;   /* The command line is stored here, since Pascal has 
			  usurped `argv' for a procedure.  */
integer argc;  /* Referenced from the Pascal, so not `static'.  */

/* These deal with Pascal vs. C strings.  */
static void make_c_string ();
static void make_pascal_string ();
static void terminate_with_null ();
static void terminate_with_space ();


/* The entry point.  We just have to set up the command line.  Pascal's
   main block is transformed into the procedure `main_body'.  */

void
main (ac, av)
  int ac;
  char **av;
{
  argc = ac;
  gargv = av;
  main_body ();
  exit (0);
}


/* Read the Nth argument from the command line.  BUF is a ``Pascal''
   string, i.e., it starts at index 1 and ends with a space.  */

void
argv (n, buf)
  int n;
  char buf[];
{
  (void) strcpy(buf+1, gargv[n]);
  (void) strcat(buf+1, " ");
}



/* Round R to the nearest whole number.  */
integer
zround (r)
  double r;
{
  return r >= 0.0 ? (r + 0.5) : (r - 0.5);
}



/* File operations.  */

/* Open a file; don't return if error.  NAME is a Pascal string.  */

FILE *
openf (name, mode)
  char *name;
  char *mode;
{
  FILE *result;
  char *cp;

  make_c_string (&name);
  
  result = fopen (name, mode);
  if (result)
    {}  return result;
    
  perror (name);
  exit (1);
  /*NOTREACHED*/
}


/* Return true if we're at the end of FILE, else false.  */

boolean
testeof (file)
  FILE *file;
{
  register int c;

  /* Maybe we're already at the end?  */
  if (feof (file))
    return true;

  if ((c = getc (file)) == EOF)
    return true;
  
  /* Whoops, we weren't at the end.  Back up.  */
  (void) ungetc (c, file);
  
  return false;
}


/* Return true on end-of-line in FILE or at the end of FILE, else false.  */

boolean
eoln (file)
  FILE *file;
{
  register int c;

  if (feof(file))
    return true;
  
  c = getc (file);
  
  if (c != EOF)
    (void) ungetc(c, file);
    
  return c == '\n' || c == EOF;
}


/* Print real number R in format N:M on stdout. */

void
printreal (r, n, m)
  double r;
  int n, m;
{
  char fmt[50];  /* Surely enough, since N and M won't be more than 25
                    digits each!  */

  (void) sprintf (fmt, "%%%d.%df", n, m);
  (void) printf (fmt, r);
}


/* Print S, a ``Pascal string'', on stdout.  It starts at index 1 and is
   terminated by a space.  */
void
printpascalstring (s)
  char *s;
{
  s++;
  while (*s != ' ') putchar (*s++);
}


/* Change the suffix of BASE (a Pascal string) to be SUFFIX (another
   Pascal string).  We have to change them to C strings to do the work,
   then convert them back to Pascal strings.  */

void
makesuffix (base, suffix)
  char *base;
  char *suffix;
{
  char *last_dot, *last_slash;
  make_c_string (&base);
  
  last_dot = rindex (base, '.');
  last_slash = rindex (base, '/');
  
  if (last_dot == NULL || last_dot < last_slash) /* Add the suffix?  */
    {
      make_c_string (&suffix);
      strcat (base, suffix);
      make_pascal_string (&suffix);
    }
    
  make_pascal_string (&base);
}
   
   
/* Deal with C and Pascal strings.  */

/* Change the Pascal string P_STRING into a C string; i.e., make it
   start after the leading character Pascal puts in, and terminate it
   with a null.  */

static void
make_c_string (p_string)
  char **p_string;
{
  (*p_string)++;
  terminate_with_null (*p_string);
}


/* Change the C string C_STRING into a Pascal string; i.e., make it
   start one character before it does now (so C_STRING had better have
   been a Pascal string originally), and terminate with a space.  */

static void
make_pascal_string (c_string)
  char **c_string;
{
  terminate_with_space (*c_string);
  (*c_string)--;
}


/* Replace the first space we come to with a null.  */

static void
terminate_with_null (s)
  char *s;
{
  while (*s++ != ' ')
    ;
  
  *(s-1) = 0;
}


/* Replace the first null we come to with a space.  */

static void
terminate_with_space (s)
  char *s;
{
  while (*s++ != 0)
    ;
  
  *(s-1) = ' ';
}



/* Path searching.  I don't know why they don't start at zero.  */

#define NUMBER_OF_PATHS 6

static char *path[NUMBER_OF_PATHS];
static char *env_var_names[NUMBER_OF_PATHS]
  = { "TEXINPUTS", "TEXFONTS", "TEXFORMATS", "TEXPOOL", "GFFONTS", "PKFONTS" };

#define READ_ACCESS 4   /* Used in access(2) call.  */
#define PATH_DELIMITER ':'  /* For the sake of DOS, which wants `;'.  */

static void next_component ();


/* This sets up the paths, by either copying from an environment variable
   or using the default path, which is defined as a preprocessor symbol
   (of the same name as the environment variable) in site.h.  */

#define DO_PATH(p, default) \
  path[p] = getenv (env_var_names[p]); \
  if (path[p] == NULL) path[p] = default

extern void
setpaths ()
{
  /* DO_PATH (TEXINPUTPATH, TEXINPUTS); */
  DO_PATH (TFMFILEPATH, TEXFONTS);
  /* DO_PATH (TEXFORMATPATH, TEXFORMATS); */
  /* DO_PATH (TEXPOOLPATH, TEXPOOL); */
  /* If the environment variables GFFONTS/PKFONTS aren't set, we want to
     use TEXFONTS.  */
  DO_PATH (GFFILEPATH, path[TFMFILEPATH]);
  DO_PATH (PKFILEPATH, path[TFMFILEPATH]);
}


/* Look for NAME, a Pascal string, in a colon-separated list of
   directories.  The path to use is given (indirectly) by PATH_INDEX.
   If the search is successful, leave the full pathname in NAME (which
   therefore must have enough room for such a pathname), padded with
   blanks.  Otherwise, or if NAME is an absolute path, just leave it
   alone.  */

boolean
testreadaccess (name, path_index)
  char *name;
  int path_index;
{
  int ok;
  char potential[FILENAMESIZE];
  char *the_path = path[path_index];
  
  if (*(name+1) == '/') return true;  /* Absolute path.  */
  make_c_string (&name);
  
  do
    {
      next_component (potential, &the_path);

  if (*potential == 0) break;
  
      strcat (potential, "/");
      strcat (potential, name);
      ok = (access (potential, READ_ACCESS) == 0);
    }
  while (! ok);
  
  if (ok) strcpy (name, potential);

  make_pascal_string (&name);
  
  return ok;
}


/* Return, in NAME, the next component of PATH, i.e., the characters up
   to the next PATH_DELIMITER.  */
   
static void
next_component (name, path)
  char name[];
  char **path;
{
  unsigned count = 0;
  
  while (**path != 0 && **path != PATH_DELIMITER)
    {
      name[count++] = **path;
      (*path)++; /* Move further along, even between calls.  */
    }
  
  name[count] = 0;
  if (**path == PATH_DELIMITER)
    (*path)++; /* Move past the delimiter.  */
}
