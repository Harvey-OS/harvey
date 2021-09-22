/* variables.c: this package provides symbols and dynamic strings

Copyright (C) 1997 Fabrice POPINEAU.

Time-stamp: <98/02/26 14:33:11 Popineau>

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
#include <kpathsea/c-proto.h>
#include <kpathsea/types.h>
#include <kpathsea/lib.h>
#include <kpathsea/hash.h>
#include <kpathsea/variable.h>
#include <kpathsea/c-pathch.h>
#include <kpathsea/c-ctype.h>
#include <kpathsea/cnf.h>
#include <kpathsea/fn.h>
#include "regex.h"
#include "stackenv.h"

string setval(string name, string value);
string setval_default(string name, string value);
string expand_var(const_string var);
string normalize(string);
string mktex_var_expand P1H(const_string src);

/*
  We will use kpathsea's hash tables
  */

/* The symbol table */
hash_table_type symtab;

/* Initialize a symbol table to store all the vars used by 
   the various shell scripts. */
void init_vars()
{
  symtab = hash_create(719);
}

void
hash_remove_all (hash_table_type *table, string key)
{
  string *val;
  while ((val = hash_lookup(*table, key)))
    hash_remove(table, key, *val);
}

string __cdecl getval(string name)
{
  string *lookup = NULL;
  string res = NULL;
  /*
    Either the name is already in the symbol table ...
    */
  if ((lookup = hash_lookup(symtab, name)) != NULL) {
    /* FIXME: we return the first value ... */
    res = lookup[0];
  }
  if (!res) {
#if 0
    /*
      ... Either it is known from kpathsea, in which case we put it 
      in the symbol table.
      */
    string dollar_name = concat("$", name);
    res = expand_var(dollar_name);

    if (test_file('n', res)) {
      setval(name, res);
    }
    else {
      /* res = "", so return NULL */
      res = NULL;
    }
    free (dollar_name);
#endif
    res = getenv(name);
  }

  if (KPSE_DEBUG_P(MKTEX_FINE_DEBUG)) {
    fprintf(stderr, "GetVal %s => %s\n", name, res);
  }

  /* In any case ... */
  return res;
}

string setval(string name, string value)
{
  /* Be safe ... */
  if (!value) value = "";
  hash_remove_all(&symtab, name);
  hash_insert(&symtab, name, value);
  if (KPSE_DEBUG_P(MKTEX_FINE_DEBUG)) {
    fprintf(stderr, "SetVal %s <= %s\n", name, value);
  }
  return value;
}

string setval_default(string name, string value)
{
  string old_value;
  /* Be safe ... */
  if (!value) value = "";

  if (KPSE_DEBUG_P(MKTEX_FINE_DEBUG)) {
    fprintf(stderr, "SetVal default %s ... ", name);
  }

  /* The mktex... scripts NEVER use ${foo:=bar}, always ${foo=bar}.
     So we only need to set NAME=VALUE if $NAME is unset.  */
  if ((old_value = getval(name)) == NULL) {
    hash_insert(&symtab, name, value);
    if (KPSE_DEBUG_P(MKTEX_FINE_DEBUG)) {
      fprintf(stderr, "SetVal %s <= %s\n", name, value);
    }
  }
  else
    value = old_value;
  return value;
}

string *grep(char *regexp, char *line, int num_vars) 
{
  struct re_pattern_buffer *rc;
  struct re_registers *p;
  const_string ok;
  string *vars = NULL;
  string *lookup;
  int i;

  if (KPSE_DEBUG_P(MKTEX_FINE_DEBUG)) {
    fprintf(stderr, "Grep\n\t%s\n\tin\n\t%s\n", regexp, line);
  }

  if (test_file('z', line))
    return NULL;

  /* This will retrieve the precompiled regexp or compile it and
     remember it. vars contains the strings matched, num_vars the number
     of these strings. */
  if ((lookup = hash_lookup(symtab, regexp)))
    rc = (struct re_pattern_buffer *)lookup[0];
  else
    rc = NULL;
  if (rc == NULL) {
    /* Compile the regexp and stores the result */

    if (KPSE_DEBUG_P(MKTEX_FINE_DEBUG)) {
      fprintf(stderr, "\tCompiling the regexp\n");
    }

    re_syntax_options = RE_SYNTAX_POSIX_EGREP;
    rc = (struct re_pattern_buffer *) calloc(1, sizeof(struct re_pattern_buffer));
    rc->regs_allocated = REGS_UNALLOCATED;
    if ((ok = re_compile_pattern(regexp, strlen(regexp), rc)) != 0)
      FATAL1("Can't compile regex %s\n", regexp);
    hash_remove_all(&symtab, regexp);
    hash_insert(&symtab, regexp, (char *)rc);
  }
  else   if (KPSE_DEBUG_P(MKTEX_FINE_DEBUG)) {
    fprintf(stderr, "\tAlready compiled\n");
  }

  p = (struct re_registers *) calloc(1, sizeof(struct re_registers));
  p->num_regs = num_vars;
  if ((re_match(rc, line, strlen(line), 0, p)) > 0) {
    vars = (char **) xmalloc ((num_vars+1) * sizeof(char *));
    for (i = 0; i <= num_vars; i++) {
      vars[i] = malloc((p->end[i] - p->start[i] + 1)*sizeof(char));
      strncpy(vars[i], line+p->start[i], p->end[i] - p->start[i]);
      vars[i][p->end[i] - p->start[i]] = '\0';
    }
  }
  free (p);
  if (KPSE_DEBUG_P(MKTEX_FINE_DEBUG)) {
    if (vars)
      for(i = 0; i <= num_vars; i++)
	fprintf(stderr, "String %d matches %s\n", i, vars[i]);
  }
  return vars;
}

/* read a line, trying to find 'variable = value' 
   Currently groks two constructs:
   VAR=value
   : {VAR=value}
   using regex to analyze them.
   The variable is added to the symbol table. In the first case, it
   overrides an old value, not in the second case.
 */
boolean parse_variable(string line)
{
  char **vars;

  static char *r1 = "^[[:space:]]*([[:alnum:]_]+)=[[:space:]]*([^[:space:]]*)[[:space:]]*.*$";
  static char *r2 = "^:[[:space:]]+\\$\\{([[:alnum:]_]+)=([^\\}]*)\\}[[:space:]]*.*$";

  if ((vars = grep(r1, line, 2))) {
    setval(vars[1], vars[2]);
    return true;
  }
  else if ((vars = grep(r2, line, 2))) {
    setval_default(vars[1], vars[2]);
    return true;
  }
  else
    WARNING1("The following line has not been parsed:\n%s\n", line);
  return false;
}

/*
  Kpathsea's basename does not strip suffixes
  */
string my_basename(string name, string suffix)
{
  string res = (string)basename(name);
  if (suffix) {
    int suffix_len = strlen(suffix);
    if (FILESTRCASEEQ(res+strlen(res)-suffix_len, suffix))
      *(res+strlen(res)-suffix_len) = '\0';
  }
  return xstrdup(res);
}

/* Return directory for NAME.  This is "." if NAME contains no directory
   separators (should never happen for selfdir), else whatever precedes
   the final directory separator, but with multiple separators stripped.
   For example, `my_dirname ("/foo//bar.baz")' returns "/foo".  Always
   return a new string.  */

string
my_dirname P1C(const_string, name)
{
  string ret;
  unsigned loc = strlen (name);
  
  for (loc = strlen (name); loc > 0 && !IS_DIR_SEP (name[loc-1]); loc--)
    ;

  if (loc) {
    /* If have ///a, must return /, so don't strip off everything.  */
    unsigned limit = NAME_BEGINS_WITH_DEVICE (name) ? 3 : 1;
    while (loc > limit && IS_DIR_SEP (name[loc-1])
           && !IS_DEVICE_SEP (name[loc-1])) {
      loc--;
    } 
    ret = xmalloc (loc + 1);
    strncpy (ret, name, loc);
    ret[loc] = 0;
  } else {
    /* No directory separators at all, so return the current directory.
       The way this is used in kpathsea, this should never happen.  */
    ret = xstrdup (".");
  }
  
  return ret;
}

/* Smart concatenation of 2 pathes: add the separator only 
 when needed. Allocates a new path. */
string concat_pathes(const_string p1, const_string p2)
{
  string res;
  if (KPSE_DEBUG_P(MKTEX_FINE_DEBUG)) {
    fprintf(stderr, "Concat pathes %s and %s\n", p1, p2);
  }

  assert (p1 && p2);

  res = normalize(concat3(p1, DIR_SEP_STRING, p2));

  if (KPSE_DEBUG_P(MKTEX_FINE_DEBUG)) {
    fprintf(stderr, "\t=> %s\n", res);
  }

  return res;
}

/* 
  Rewrite path with the same DIR_SEP and deleting multiple ones
  FIXME: does this eliminates the trailing ones ? W95 doesn't grok 
  stat() on pathes that end up in PATH_SEP. 
  */
char *normalize(string path)
{
  string p, q;

  /* Delete any multiple slashes */
  for (p = q = path; *p != '\0'; p++, q++) {
    if (IS_DIR_SEP(*p)) {
      *q = DIR_SEP;
      q++;
      while (*++p && IS_DIR_SEP(*p));
    }
    *q = *p;
  }
  *q = '\0';

  return path;
}

/* Substitute from by to in line */
string subst(string line, string from, string to)
{
  string before, after, p;

  if (KPSE_DEBUG_P(MKTEX_FINE_DEBUG)) {
    fprintf(stderr, "Substituting %s by %s in %s\n", from, to, line);
  }

  if ((p = strstr(line, from))) {
    before = xstrdup(line);
    before[p-line] = '\0';
    after = p+strlen(from);
    p = concat3(before, to , after);
    free (before);
  }
  else
    p = xstrdup(line);

  if (KPSE_DEBUG_P(MKTEX_FINE_DEBUG)) {
    fprintf(stderr, "=> gives %s\n", p);
  }

  return p;
}

/* kpse_expand_var does not remove leading !!, but we do not need
   them in mktex*
   (Parts of code shamelessly stolen from remove_dbonly on tex-file.c.)
   */
string expand_var(const_string s)
{
  string p, q;
  string res = NULL;

  if (s) {
    boolean new_elt = true;

    res = mktex_var_expand(s);

    for (p = q = res; *p; ) {
      if (new_elt && *p == '!' && p[1] == '!')
	p += 2;
      else {
	new_elt = *p == ENV_SEP;
	*q++ = *p++;
      }
    }
    *q = '\0';

    if (KPSE_DEBUG_P(MKTEX_FINE_DEBUG)) {
      fprintf(stderr, "Expanding variable %s into %s\n", s, res);
    }
  }

  return res;
}

/*
  This is shamelessly stolen from kpathsea/variable.c .
  We want to do expansion without having to put variables in the
  environment.
  */

/* We have to keep track of variables being expanded, otherwise
   constructs like TEXINPUTS = $TEXINPUTS result in an infinite loop.
   (Or indirectly recursive variables, etc.)  Our simple solution is to
   add to a list each time an expansion is started, and check the list
   before expanding.  */

typedef struct {
  const_string var;
  boolean expanding;
} expansion_type;
static expansion_type *expansions; /* The sole variable of this type.  */
static unsigned expansion_len = 0;

static void
expanding P2C(const_string, var,  boolean, xp)
{
  unsigned e;
  for (e = 0; e < expansion_len; e++) {
    if (STREQ (expansions[e].var, var)) {
      expansions[e].expanding = xp;
      return;
    }
  }

  /* New variable, add it to the list.  */
  expansion_len++;
  XRETALLOC (expansions, expansion_len, expansion_type);
  expansions[expansion_len - 1].var = xstrdup (var);
  expansions[expansion_len - 1].expanding = xp;
}


/* Return whether VAR is currently being expanding.  */

static boolean
expanding_p P1C(const_string, var)
{
  unsigned e;
  for (e = 0; e < expansion_len; e++) {
    if (STREQ (expansions[e].var, var))
      return expansions[e].expanding;
  }
  
  return false;
}

/* Append the result of value of `var' to EXPANSION, where `var' begins
   at START and ends at END.  If `var' is not set, do not complain.
   This is a subroutine for the more complicated expansion function.  */

static void
expand P3C(fn_type *, expansion,  const_string, start,  const_string, end)
{
  string value;
  unsigned len = end - start + 1;
  string var = xmalloc (len + 1);
  strncpy (var, start, len);
  var[len] = 0;

  if (expanding_p (var)) {
    WARNING1 ("kpathsea: variable `%s' references itself (eventually)", var);
  } else {
#if 1
    /* The mktex.c replacement for shell scripts needs a more sophisticated
       lookup for variables */
    value = getval(var);
#else
    /* Check for an environment variable.  */
    value = getenv (var);
#endif
    /* If no envvar, check the config files.  */
    if (!value)
      value = kpse_cnf_get (var);

    if (value) {
      expanding (var, true);
      value = mktex_var_expand (value);
      expanding (var, false);
      fn_grow (expansion, value, strlen (value));
      free (value);
    }

    free (var);
  }
}

/* Can't think of when it would be useful to change these (and the
   diagnostic messages assume them), but ... */
#ifndef IS_VAR_START /* starts all variable references */
#define IS_VAR_START(c) ((c) == '$')
#endif
#ifndef IS_VAR_CHAR  /* variable name constituent */
#define IS_VAR_CHAR(c) (ISALNUM (c) || (c) == '_')
#endif
#ifndef IS_VAR_BEGIN_DELIMITER /* start delimited variable name (after $) */
#define IS_VAR_BEGIN_DELIMITER(c) ((c) == '{')
#endif
#ifndef IS_VAR_END_DELIMITER
#define IS_VAR_END_DELIMITER(c) ((c) == '}')
#endif


/* Maybe we should support some or all of the various shell ${...}
   constructs, especially ${var-value}.  */

string
mktex_var_expand P1C(const_string, src)
{
  const_string s;
  string ret;
  fn_type expansion;
  expansion = fn_init ();
  
  /* Copy everything but variable constructs.  */
  for (s = src; *s; s++) {
    if (IS_VAR_START (*s)) {
      s++;

      /* Three cases: `$VAR', `${VAR}', `$<anything-else>'.  */
      if (IS_VAR_CHAR (*s)) {
        /* $V: collect name constituents, then expand.  */
        const_string var_end = s;

        do {
          var_end++;
        } while (IS_VAR_CHAR (*var_end));

        var_end--; /* had to go one past */
        expand (&expansion, s, var_end);
        s = var_end;

      } else if (IS_VAR_BEGIN_DELIMITER (*s)) {
        /* ${: scan ahead for matching delimiter, then expand.  */
        const_string var_end = ++s;

        while (*var_end && !IS_VAR_END_DELIMITER (*var_end))
          var_end++;

        if (! *var_end) {
          WARNING1 ("%s: No matching } for ${", src);
          s = var_end - 1; /* will incr to null at top of loop */
        } else {
          expand (&expansion, s, var_end - 1);
          s = var_end; /* will incr past } at top of loop*/
        }

      } else {
        /* $<something-else>: error.  */
        WARNING2 ("%s: Unrecognized variable construct `$%c'", src, *s);
        /* Just ignore those chars and keep going.  */
      }
    } else
     fn_1grow (&expansion, *s);
  }
  fn_1grow (&expansion, 0);
          
  ret = FN_STRING (expansion);
  return ret;
}

#ifdef TEST

main()
{
  char buf[256];
  while (gets(buf)) parse_variable(buf);
}
#endif
