/* main.c -- the usual main program.  */

#include "config.h"


/* ridderbusch.pad@nixdorf.com says this is necessary.  */
#ifdef ATARI_ST
int _stksize = -1L;
#endif


/* The command line is stored under `gargv', since Pascal has usurped `argv' 
   for a procedure.  These variables are referenced from the Pascal, so
   we can't make them static.  */
char **gargv;
int argc;


/* The entry point for all the programs except TeX and Metafont, which
   have more to do.  We just have to set up the command line.  Pascal's
   main block is transformed into the procedure `main_body'.  */

int
main (ac, av)
  int ac;
  char **av;
{
  argc = ac;
  gargv = av;
  main_body ();
  uexit (0);
}


/* Read the Nth argument from the command line and return it in BUF as a
   Pascal string, i.e., starting at index 1 and ending with a space.  If
   N is beyond the end of the command line, abort.  */

void
argv P2C(int, n,  string, buf)
{
  if (n >= argc)
    {
      fprintf (stderr, "%s: Not enough arguments.\n", gargv[0]);
      uexit (1);
    }
  (void) strcpy (buf + 1, gargv[n]);
  (void) strcat (buf + 1, " ");
}
