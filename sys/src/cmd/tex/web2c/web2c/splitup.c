/* splitup -- take TeX or MF in C as a single stream on stdin,
   and it produces several .c and .h files in the current directory
   as its output.

  Tim Morgan  September 19, 1987.  */

#include "config.h"

int filenumber = 0, ifdef_nesting = 0, lines_in_file = 0;
char *output_name = "tex";
int has_ini;

#define	TEMPFILE	"temp.c"
#define	MAXLINES	2000

char buffer[1024], filename[PATH_MAX];

FILE *out, *ini, *temp;
FILE *in = stdin;


int
main (argc, argv)
  int argc;
  char *argv[];
{
    if (argc > 1)
	output_name = argv[1];

    (void) sprintf (filename, "%sd.h", output_name);
    if (!(out = fopen (filename, "w")))
	FATAL_PERROR (filename);

    (void) fprintf(out,
		"#undef\tTRIP\n#undef\tTRAP\n#define\tSTAT\n#undef\tDEBUG\n");
    for ((void) fgets(buffer, sizeof(buffer), in);
          strncmp(&buffer[10], "coerce.h", 8);
         (void) fgets(buffer, sizeof(buffer), in))
      {
	if (buffer[0] == '#' || buffer[0] == '\n' || buffer[0] == '}'
            || buffer[0] == '/'
	    || buffer[0] == ' ' || strncmp(buffer, "typedef", 7) == 0)
          /*nothing*/;
	else
          (void) fputs("EXTERN ", out);

	(void) fputs(buffer, out);
      }

    (void) fputs(buffer, out);
    fclose (out);

    (void) sprintf(filename, "i%s.c", output_name);
    ini = fopen(filename, "w");
    if (!ini)
	FATAL_PERROR (filename);

    (void) fputs("#define EXTERN extern\n", ini);
    (void) fprintf(ini, "#include \"%sd.h\"\n\n", output_name);
    (void) sprintf(filename, "%s0.c", output_name);

    if (!(out = fopen(filename, "w")))
	FATAL_PERROR (filename);
    (void) fputs("#define EXTERN extern\n", out);
    (void) fprintf(out, "#include \"%sd.h\"\n\n", output_name);

    do {
	/* Read one routine into a temp file */
	has_ini = false;
	if (!(temp = fopen(TEMPFILE, "w+")))
	    FATAL_PERROR (TEMPFILE);
            
	while (read_line()) {
	    (void) fputs(buffer, temp);
	    if (buffer[0] == '}') break; /* End of procedure */
	}
	while (ifdef_nesting > 0 && read_line())
	    (void) fputs(buffer, temp);
	rewind(temp);

	if (has_ini) {	/* Contained "#ifdef INI..." */
	    while (fgets(buffer, sizeof(buffer), temp))
		(void) fputs(buffer, ini);
	}
	else {			/* Doesn't contain "#ifdef INI..." */
	    while (fgets(buffer, sizeof(buffer), temp)) {
		(void) fputs(buffer, out);
		lines_in_file++;
	    }
	}
	if (fclose (temp))
	    FATAL_PERROR ("fclose");
            
	if (lines_in_file > MAXLINES) {
	    if (fclose(out))
		perror("fclose"), uexit (1);
	    (void) sprintf(filename, "%s%d.c", output_name, ++filenumber);
	    if ( !(out = fopen(filename, "w")))
		perror(filename), uexit (1);
	    (void) fputs("#define EXTERN extern\n", out);
	    (void) fprintf(out, "#include \"%sd.h\"\n\n", output_name);
	    lines_in_file = 0;
	}
    } while (!feof(in));

    if (fclose(out))
	perror("fclose"), uexit (1);
    if (lines_in_file == 0)
	(void) unlink(filename);
    if (fclose(ini))
	perror("fclose"), uexit (1);
    if (unlink(TEMPFILE))
	perror(TEMPFILE), uexit (1);

    uexit (0);
}

/*
 * Read a line of input into the buffer, returning `false' on EOF.
 * If the line is of the form "#ifdef INI...", we set "has_ini"
 * `true' else `false'.  We also keep up with the #ifdef/#endif nesting
 * so we know when it's safe to finish writing the current file.
 */
int
read_line()
{
    if (fgets(buffer, sizeof(buffer), in) == NULL) return false;
    if (strncmp(buffer, "#ifdef", 6) == 0) {
	++ifdef_nesting;
	if (strncmp(&buffer[7], "INI", 3) == 0) has_ini = true;
    }
    else if (strncmp(buffer, "#endif", 6) == 0) --ifdef_nesting;
    return true;
}
