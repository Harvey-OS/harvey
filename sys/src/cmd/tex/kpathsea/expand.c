/* expand.c: general expansion.  Some of this file (the brace-expansion
   code from bash) is covered by the GPL; this is the only GPL-covered
   code in kpathsea.  The part of the file that I wrote (the first
   couple of functions) is covered by the LGPL.

Copyright (C) 1993, 94, 95, 96, 97 Karl Berry & O. Weber.

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Library General Public
License as published by the Free Software Foundation; either
version 2 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Library General Public License for more details.

You should have received a copy of the GNU Library General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */

#include <kpathsea/config.h>

#include <kpathsea/c-pathch.h>
#include <kpathsea/expand.h>
#include <kpathsea/pathsearch.h>
#include <kpathsea/tilde.h>
#include <kpathsea/variable.h>
#include <kpathsea/concatn.h>
#include <kpathsea/absolute.h>


/* Do variable expansion first so ~${USER} works.  (Besides, it's what the
   shells do.)  */

string
kpse_expand P1C(const_string, s)
{
  string var_expansion = kpse_var_expand (s);
  string tilde_expansion = kpse_tilde_expand (var_expansion);
  
  /* `kpse_var_expand' always gives us new memory; `kpse_tilde_expand'
     doesn't, necessarily.  So be careful that we don't free what we are
     about to return.  */
  if (tilde_expansion != var_expansion)
    free (var_expansion);
  
  return tilde_expansion;
}


/* Forward declarations of functions from the original expand.c  */
static char **brace_expand P1H(const_string);
static void free_array P1H(char **);

/* If $KPSE_DOT is defined in the environment, prepend it to any relative
   path components. */

static string
kpse_expand_kpse_dot P1C(string, path)
{
  string ret, elt;
  string kpse_dot = getenv("KPSE_DOT");
#ifdef MSDOS
  boolean malloced_kpse_dot = false;
#endif
  
  if (kpse_dot == NULL)
    return path;
  ret = xmalloc(1);
  *ret = 0;

#ifdef MSDOS
  /* Some setups of ported Bash force $KPSE_DOT to have the //d/foo/bar
     form (when `pwd' is used), which is not understood by libc and the OS.
     Convert them back to the usual d:/foo/bar form.  */
  if (kpse_dot[0] == '/' && kpse_dot[1] == '/'
      && kpse_dot[2] >= 'A' && kpse_dot[2] <= 'z' && kpse_dot[3] == '/') {
    kpse_dot++;
    kpse_dot = xstrdup (kpse_dot);
    kpse_dot[0] = kpse_dot[1];  /* drive letter */
    kpse_dot[1] = ':';
    malloced_kpse_dot = true;
  }
#endif

  for (elt = kpse_path_element (path); elt; elt = kpse_path_element (NULL)) {
    string save_ret = ret;
    /* We assume that the !! magic is only used on absolute components.
       Single "." get special treatment, as does "./" or its equivalent. */
    if (kpse_absolute_p (elt, false) || (elt[0] == '!' && elt[1] == '!')) {
      ret = concat3(ret, elt, ENV_SEP_STRING);
    } else if (elt[0] == '.' && elt[1] == 0) {
      ret = concat3 (ret, kpse_dot, ENV_SEP_STRING);
#ifndef VMS
    } else if (elt[0] == '.' && IS_DIR_SEP(elt[1])) {
      ret = concatn (ret, kpse_dot, elt + 1, ENV_SEP_STRING, NULL);
    } else {
      ret = concatn (ret, kpse_dot, DIR_SEP_STRING, elt, ENV_SEP_STRING, NULL);
#endif
    }
    free (save_ret);
  }

#ifdef MSDOS
  if (malloced_kpse_dot) free (kpse_dot);
#endif

  ret[strlen (ret) - 1] = 0;
  return ret;
}

/* Do brace expansion on ELT; then do variable and ~ expansion on each
   element of the result; then do brace expansion again, in case a
   variable definition contained braces (e.g., $TEXMF).  Return a
   string comprising all of the results separated by ENV_SEP_STRING.  */

static string
kpse_brace_expand_element P1C(const_string, elt)
{
  unsigned i;
  string *expansions = brace_expand (elt);
  string ret = xmalloc (1);
  *ret = 0;

  for (i = 0; expansions[i]; i++) {
    /* Do $ and ~ expansion on each element.  */
    string x = kpse_expand (expansions[i]);
    string save_ret = ret;
    if (!STREQ (x, expansions[i])) {
      /* If we did any expansions, do brace expansion again.  Since
         recursive variable definitions are not allowed, this recursion
         must terminate.  (In practice, it's unlikely there will ever be
         more than one level of recursion.)  */
      string save_x = x;
      x = kpse_brace_expand_element (x);
      free (save_x);
    }
    ret = concat3 (ret, x, ENV_SEP_STRING);
    free (save_ret);
    free (x);
  }

  free_array (expansions);
  ret[strlen (ret) - 1] = 0; /* waste the trailing null */
  return ret;
}

/* Be careful to not waste all the memory we allocate for each element.  */

string
kpse_brace_expand P1C(const_string, path)
{
  string kpse_dot_expansion;
  string elt;
  unsigned len;
  /* Must do variable expansion first because if we have
       foo = .:~
       TEXINPUTS = $foo
     we want to end up with TEXINPUTS = .:/home/karl.
     Since kpse_path_element is not reentrant, we must get all
     the path elements before we start the loop.  */
  string xpath = kpse_var_expand (path);
  string ret = xmalloc (1);
  *ret = 0;

  for (elt = kpse_path_element (xpath); elt; elt = kpse_path_element (NULL)) {
    string save_ret = ret;
    /* Do brace expansion first, so tilde expansion happens in {~ka,~kb}.  */
    string expansion = kpse_brace_expand_element (elt);
    ret = concat3 (ret, expansion, ENV_SEP_STRING);
    free (expansion);
    free (save_ret);
  }

  /* Waste the last byte by overwriting the trailing env_sep with a null.  */
  len = strlen (ret);
  if (len != 0)
    ret[len - 1] = 0;
  free (xpath);

  kpse_dot_expansion = kpse_expand_kpse_dot (ret);
  if (kpse_dot_expansion != ret)
    free (ret);

  return kpse_dot_expansion;
}

/* Expand all special constructs in a path, and include only the actually
   existing directories in the result. */
string
kpse_path_expand P1C(const_string, path)
{
  string ret;
  string xpath;
  string elt;
  unsigned len;

  /* Initialise ret to the empty string. */
  ret = xmalloc (1);
  *ret = 0;
  len = 0;
  
  /* Expand variables and braces first.  */
  xpath = kpse_brace_expand (path);

  /* Now expand each of the path elements, printing the results */
  for (elt = kpse_path_element (xpath); elt; elt = kpse_path_element (NULL)) {
    str_llist_type *dirs;

    /* Skip and ignore magic leading chars.  */
    if (*elt == '!' && *(elt + 1) == '!')
      elt += 2;

    /* Do not touch the device if present */
    if (NAME_BEGINS_WITH_DEVICE (elt)) {
      while (IS_DIR_SEP (*(elt + 2)) && IS_DIR_SEP (*(elt + 3))) {
        *(elt + 2) = *(elt + 1);
        *(elt + 1) = *elt;
        elt++;
      }
    } else {
      /* We never want to search the whole disk.  */
      while (IS_DIR_SEP (*elt) && IS_DIR_SEP (*(elt + 1)))
        elt++;
    }

    /* Search the disk for all dirs in the component specified.
       Be faster to check the database, but this is more reliable.  */
    dirs = kpse_element_dirs (elt); 
    if (dirs && *dirs) {
      str_llist_elt_type *dir;

      for (dir = *dirs; dir; dir = STR_LLIST_NEXT (*dir)) {
        string thedir = STR_LLIST (*dir);
        unsigned dirlen = strlen (thedir);
        string save_ret = ret;
        /* Retain trailing slash if that's the root directory.  */
        if (dirlen == 1 || (dirlen == 3 && NAME_BEGINS_WITH_DEVICE (thedir)
                            && IS_DIR_SEP (thedir[2]))) {
          ret = concat3 (ret, thedir, ENV_SEP_STRING);
          len += dirlen + 1;
          ret[len - 1] = ENV_SEP;
        } else {
          ret = concat (ret, thedir);
          len += dirlen;
          ret [len - 1] = ENV_SEP;
        }
        free (save_ret);
      }
    }
  }
  /* Get rid of trailing ':', if any. */
  if (len != 0)
    ret[len - 1] = 0;
  return ret;
}

/* braces.c -- code for doing word expansion in curly braces. Taken from
   bash 1.14.5.  [Ans subsequently modified for kpatshea.]

   Copyright (C) 1987,1991 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 1, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not, write to the
   Free Software Foundation, Inc., 59 Temple Place - Suite 330, Boston,
   MA 02111-1307, USA.  */


#define brace_whitespace(c) (!(c) || (c) == ' ' || (c) == '\t' || (c) == '\n')
#define savestring xstrdup

/* Basic idea:

   Segregate the text into 3 sections: preamble (stuff before an open brace),
   postamble (stuff after the matching close brace) and amble (stuff after
   preamble, and before postamble).  Expand amble, and then tack on the
   expansions to preamble.  Expand postamble, and tack on the expansions to
   the result so far.
 */

/* The character which is used to separate arguments. */
static int brace_arg_separator = ',';

static int brace_gobbler P3H(const_string, int *, int);
static char **expand_amble P1H(const_string),
            **array_concat P2H(string * , string *);

/* Return the length of ARRAY, a NULL terminated array of char *. */
static int
array_len P1C(char **, array)
{
  register int i;
  for (i = 0; array[i]; i++);
  return (i);
}

/* Free the contents of ARRAY, a NULL terminated array of char *. */
static void
free_array P1C(char **, array)
{
  register int i = 0;

  if (!array) return;

  while (array[i])
    free (array[i++]);
  free (array);
}

/* Allocate and return a new copy of ARRAY and its contents. */
static char **
copy_array P1C(char **, array)
{
  register int i;
  int len;
  char **new_array;

  len = array_len (array);

  new_array = (char **)xmalloc ((len + 1) * sizeof (char *));
  for (i = 0; array[i]; i++)
    new_array[i] = savestring (array[i]);
  new_array[i] = (char *)NULL;

  return (new_array);
}


/* Return an array of strings; the brace expansion of TEXT. */
static char **
brace_expand P1C(const_string, text)
{
  register int start;
  char *preamble, *amble;
  const_string postamble;
  char **tack, **result;
  int i, c;

  /* Find the text of the preamble. */
  i = 0;
  c = brace_gobbler (text, &i, '{');

  preamble = xmalloc (i + 1);
  strncpy (preamble, text, i);
  preamble[i] = 0;

  result = xmalloc (2 * sizeof (char *));
  result[0] = preamble;
  result[1] = NULL;
  
  /* Special case.  If we never found an exciting character, then
     the preamble is all of the text, so just return that. */
  if (c != '{')
    return (result);

  /* Find the amble.  This is the stuff inside this set of braces. */
  start = ++i;
  c = brace_gobbler (text, &i, '}');

  /* What if there isn't a matching close brace? */
  if (!c)
    {
      WARNING1 ("%s: Unmatched {", text);
      free (preamble);		/* Same as result[0]; see initialization. */
      result[0] = savestring (text);
      return (result);
    }

  amble = xmalloc (1 + (i - start));
  strncpy (amble, &text[start], (i - start));
  amble[i - start] = 0;

  postamble = &text[i + 1];

  tack = expand_amble (amble);
  result = array_concat (result, tack);
  free (amble);
  free_array (tack);

  tack = brace_expand (postamble);
  result = array_concat (result, tack);
  free_array (tack);

  return (result);
}


/* Expand the text found inside of braces.  We simply try to split the
   text at BRACE_ARG_SEPARATORs into separate strings.  We then brace
   expand each slot which needs it, until there are no more slots which
   need it. */
static char **
expand_amble P1C(const_string, text)
{
  char **result, **partial;
  char *tem;
  int start, i, c;

  result = NULL;

  for (start = 0, i = 0, c = 1; c; start = ++i)
    {
      int c0, c1;
      int i0, i1;
      i0 = i;
      c0 = brace_gobbler (text, &i0, brace_arg_separator);
      i1 = i;
      c1 = brace_gobbler (text, &i1, ENV_SEP);
      c = c0 | c1;
      i = (i0 < i1 ? i0 : i1);

      tem = xmalloc (1 + (i - start));
      strncpy (tem, &text[start], (i - start));
      tem[i- start] = 0;

      partial = brace_expand (tem);

      if (!result)
	result = partial;
      else
	{
	  register int lr = array_len (result);
	  register int lp = array_len (partial);
	  register int j;

	  result = xrealloc (result, (1 + lp + lr) * sizeof (char *));

	  for (j = 0; j < lp; j++)
	    result[lr + j] = partial[j];

	  result[lr + j] = NULL;
	  free (partial);
	}
      free (tem);
    }
  return (result);
}

/* Return a new array of strings which is the result of appending each
   string in ARR2 to each string in ARR1.  The resultant array is
   len (arr1) * len (arr2) long.  For convenience, ARR1 (and its contents)
   are free ()'ed.  ARR1 can be NULL, in that case, a new version of ARR2
   is returned. */
static char **
array_concat P2C(string *, arr1,  string *, arr2)
{
  register int i, j, len, len1, len2;
  register char **result;

  if (!arr1)
    return (copy_array (arr2));

  if (!arr2)
    return (copy_array (arr1));

  len1 = array_len (arr1);
  len2 = array_len (arr2);

  result = xmalloc ((1 + (len1 * len2)) * sizeof (char *));

  len = 0;
  for (i = 0; i < len2; i++)
    {
      int strlen_2 = strlen (arr2[i]);

      for (j = 0; j < len1; j++)
	{
	  int strlen_1 = strlen (arr1[j]);

	  result[len] =
	    xmalloc (1 + strlen_1 + strlen_2);
	  strcpy (result[len], arr1[j]);
	  strcpy (result[len] + strlen_1, arr2[i]);
	  len++;
	}
    }
  free_array (arr1);

  result[len] = NULL;
  return (result);
}

/* Start at INDEX, and skip characters in TEXT. Set INDEX to the
   index of the character matching SATISFY.  This understands about
   quoting.  Return the character that caused us to stop searching;
   this is either the same as SATISFY, or 0. */
static int
brace_gobbler P3C(const_string, text,  int *, indx,  int, satisfy)
{
  register int i, c, quoted, level, pass_next;

  level = quoted = pass_next = 0;

  for (i = *indx; (c = text[i]); i++)
    {
      if (pass_next)
	{
	  pass_next = 0;
	  continue;
	}

      /* A backslash escapes the next character.  This allows backslash to
	 escape the quote character in a double-quoted string. */
      if (c == '\\' && (quoted == 0 || quoted == '"' || quoted == '`'))
        {
          pass_next = 1;
          continue;
        }

      if (quoted)
	{
	  if (c == quoted)
	    quoted = 0;
	  continue;
	}

      if (c == '"' || c == '\'' || c == '`')
	{
	  quoted = c;
	  continue;
	}
      
      if (c == satisfy && !level && !quoted)
	{
	  /* We ignore an open brace surrounded by whitespace, and also
	     an open brace followed immediately by a close brace, that
	     was preceded with whitespace.  */
	  if (c == '{' &&
	      ((!i || brace_whitespace (text[i - 1])) &&
	       (brace_whitespace (text[i + 1]) || text[i + 1] == '}')))
	    continue;
	  /* If this is being compiled as part of bash, ignore the `{'
	     in a `${}' construct */
	  if ((c != '{') || !i || (text[i - 1] != '$'))
	    break;
	}

      if (c == '{')
	level++;
      else if (c == '}' && level)
	level--;
    }

  *indx = i;
  return (c);
}

#if defined (TEST)
#include <stdio.h>

fatal_error (format, arg1, arg2)
     char *format, *arg1, *arg2;
{
  report_error (format, arg1, arg2);
  exit (1);
}

report_error (format, arg1, arg2)
     char *format, *arg1, *arg2;
{
  fprintf (stderr, format, arg1, arg2);
  fprintf (stderr, "\n");
}

main ()
{
  char example[256];

  for (;;)
    {
      char **result;
      int i;

      fprintf (stderr, "brace_expand> ");

      if ((!fgets (example, 256, stdin)) ||
	  (strncmp (example, "quit", 4) == 0))
	break;

      if (strlen (example))
	example[strlen (example) - 1] = 0;

      result = brace_expand (example);

      for (i = 0; result[i]; i++)
	printf ("%s\n", result[i]);

      free_array (result);
    }
}

/*
 * Local variables:
 * test-compile-command: "gcc -g -DTEST -I.. -I. -o brace_expand braces.c -L. -lkpathsea"
 * end:
 */

#endif /* TEST */
