/* strpascal.c: deal with C vs. Pascal strings.  */

#include "config.h"

/* Change the Pascal string P_STRING into a C string; i.e., make it
   start after the leading character Pascal puts in, and terminate it
   with a null.  */

void
make_c_string (p_string)
  char **p_string;
{
  (*p_string)++;
  null_terminate (*p_string);
}


/* Replace the first space we come to with a null.  */

void
null_terminate (s)
  char *s;
{
  while (*s != ' ')
    s++;

  *s = 0;
}


/* Change the C string C_STRING into a Pascal string; i.e., make it
   start one character before it does now (so C_STRING had better have
   been a Pascal string originally), and terminate with a space.  */

void
make_pascal_string (c_string)
  char **c_string;
{
  space_terminate (*c_string);
  (*c_string)--;
}


/* Replace the first null we come to with a space.  */

void
space_terminate (s)
  char *s;
{
  for ( ; *s != 0; s++)
    ;
  
  *s = ' ';
}



/* Change the suffix of BASE (a Pascal string) to be SUFFIX (another
   Pascal string).  We have to change them to C strings to do the work,
   then convert them back to Pascal strings.  */

void
makesuffixpas (base, suffix)
  char *base;
  char *suffix;
{
  char *last_dot, *last_slash;
  make_c_string (&base);
  
  last_dot = strrchr (base, '.');
  last_slash = strrchr (base, '/');
  
  if (last_dot == NULL || last_dot < last_slash) /* Add the suffix?  */
    {
      make_c_string (&suffix);
      strcat (base, suffix);
      make_pascal_string (&suffix);
    }
    
  make_pascal_string (&base);
}
   
   

/* Print S, a Pascal string, on the file F.  It starts at index 1 and is
   terminated by a space.  */

static void
fprint_pascal_string (s, f)
  char *s;
  FILE *f;
{
  s++;
  while (*s != ' ') putc (*s++, f);
}

/* Print S, a Pascal string, on stdout.  */

void
printpascalstring (s)
  char *s;
{
  fprint_pascal_string (s, stdout);
}

/* Ditto, for stderr.  */

void
errprintpascalstring (s)
  char *s;
{
  fprint_pascal_string (s, stderr);
}
