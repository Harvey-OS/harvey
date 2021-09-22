#include <stdio.h>
#include <kpathsea/c-proto.h>
/* splitup -- take TeX or MF in C as a single stream on stdin,
   and it produces several .c and .h files in the current directory
   as its output.

   Tim Morgan  September 19, 1987.  */

#include "config.h"

#ifdef VMS
#define unlink delete
#endif

int filenumber = 0, ifdef_nesting = 0, lines_in_file = 0;
char *output_name = NULL;
int has_ini;

#define	TEMPFILE "splitemp.c"

/* This used to be 2000, but since bibtex.c is almost 10000 lines
   (200+K), we may as well decrease the number of split files we create.
   Probably faster for the compiler, definitely faster for the linker,
   simpler for the Makefiles, and generally better.  */
#define	MAXLINES 10000

/* Don't need long filenames, since we generate them all.  */
char buffer[1024], filename[100], ini_name[100];

FILE *out, *ini, *temp;


/*
 * Read a line of input into the buffer, returning `false' on EOF.
 * If the line is of the form "#ifdef INI...", we set "has_ini"
 * `true' else `false'.  We also keep up with the #ifdef/#endif nesting
 * so we know when it's safe to finish writing the current file.
 */
int
read_line P1H(void)
{
  if (fgets (buffer, sizeof (buffer), stdin) == NULL)
    return false;
  if (strncmp (buffer, "#ifdef", 6) == 0
      || strncmp (buffer, "#ifndef", 7) == 0)
    {
      ++ifdef_nesting;
      if (strncmp (&buffer[7], "INI", 3) == 0)
	has_ini = true;
    }
  else if (strncmp (buffer, "#endif", 6) == 0)
    --ifdef_nesting;
  return true;
}

int
main P2C(int, argc, string *, argv)
{
  string coerce;
  unsigned coerce_len;
  
  kpse_set_progname (argv[0]); /* In case we use FATAL.  */

  if (argc > 1)
    output_name = argv[1];

  sprintf (filename, "%sd.h", output_name);
  out = xfopen (filename, FOPEN_W_MODE);
  fputs ("#undef TRIP\n#undef TRAP\n", out);
  /* We have only one binary that can do both ini stuff and vir stuff.  */
  fputs ("#define STAT\n#define INI\n", out);
  
  if (STREQ (output_name, "mf")) {
    fputs ("#define INIMF\n#define MF\n", out);
    coerce = "mfcoerce.h";
  } else if (STREQ (output_name, "tex")) {
    fputs ("#define INITEX\n#define TeX\n", out);
    coerce = "texcoerce.h";
  } else if (STREQ (output_name, "etex")) {
    fputs ("#define INITEX\n#define TeX\n#define eTeX\n", out);
    coerce = "etexcoerce.h";
  } else if (STREQ (output_name, "omega")) {
    fputs ("#define INITEX\n#define TeX\n#define Omega\n", out);
    coerce = "omegacoerce.h";
  } else if (STREQ (output_name, "pdftex")) {
    fputs ("#define INITEX\n#define TeX\n#define PDFTeX\n", out);
    coerce = "pdftexcoerce.h";
  } else if (STREQ (output_name, "mp")) {
    fputs ("#define INIMP\n#define MP\n", out);
    coerce = "mpcoerce.h";
  } else
    FATAL1 ("Can only split mf, mp, tex, etex, or pdftex, not %s", output_name);
  
  coerce_len = strlen (coerce);
  
  /* Read everything up to coerce.h.  */
  while (fgets (buffer, sizeof (buffer), stdin))
    {
      if (strncmp (&buffer[10], coerce, coerce_len) == 0)
	break;

      if (buffer[0] == '#' || buffer[0] == '\n' || buffer[0] == '}'
	  || buffer[0] == '/' || buffer[0] == ' '
	  || strncmp (buffer, "typedef", 7) == 0)
	/*nothing */ ;
      else
	fputs ("EXTERN ", out);

      fputs (buffer, out);
    }

  if (strncmp (&buffer[10], coerce, coerce_len) != 0)
    FATAL1 ("No #include %s line", coerce);

  fputs (buffer, out);
  xfclose (out, filename);

  sprintf (ini_name, "%sini.c", output_name);
  ini = xfopen (ini_name, FOPEN_W_MODE);
  fputs ("#define EXTERN extern\n", ini);
  fprintf (ini, "#include \"%sd.h\"\n\n", output_name);

  sprintf (filename, "%s0.c", output_name);
  out = xfopen (filename, FOPEN_W_MODE);
  fputs ("#define EXTERN extern\n", out);
  fprintf (out, "#include \"%sd.h\"\n\n", output_name);

  do
    {
      /* Read one routine into a temp file */
      has_ini = false;
      temp = xfopen (TEMPFILE, "w+");

      while (read_line ())
	{
	  fputs (buffer, temp);
	  if (buffer[0] == '}')
	    break;		/* End of procedure */
	}
      while (ifdef_nesting > 0 && read_line ())
	fputs (buffer, temp);
      rewind (temp);

      if (has_ini)
	{			/* Contained "#ifdef INI..." */
	  while (fgets (buffer, sizeof (buffer), temp))
	    fputs (buffer, ini);
	}
      else
	{			/* Doesn't contain "#ifdef INI..." */
	  while (fgets (buffer, sizeof (buffer), temp))
	    {
	      fputs (buffer, out);
	      lines_in_file++;
	    }
	}
      xfclose (temp, TEMPFILE);

      /* Switch to new output file.  */
      if (lines_in_file > MAXLINES)
	{
	  xfclose (out, filename);
	  sprintf (filename, "%s%d.c", output_name, ++filenumber);
	  out = xfopen (filename, FOPEN_W_MODE);
	  fputs ("#define EXTERN extern\n", out);
	  fprintf (out, "#include \"%sd.h\"\n\n", output_name);
	  lines_in_file = 0;
	}
    }
  while (!feof (stdin));

  xfclose (out, filename);
  if (lines_in_file == 0)
    unlink (filename);

  xfclose (ini, ini_name);

  if (unlink (TEMPFILE))
    perror (TEMPFILE), uexit (1);

  return EXIT_SUCCESS;
}
