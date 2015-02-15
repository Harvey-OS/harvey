/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

/* basename.c -- return the last element in a path */

#if HAVE_CONFIG_H
# include <config.h>
#endif

#include <backupfile.h>

#ifndef FILESYSTEM_PREFIX_LEN
#define FILESYSTEM_PREFIX_LEN(f) 0
#endif

#ifndef ISSLASH
#define ISSLASH(c) ((c) == '/')
#endif

/* In general, we can't use the builtin `basename' function if available,
   since it has different meanings in different environments.
   In some environments the builtin `basename' modifies its argument.  */

char *
base_name (name)
     char const *name;
{
  char const *base = name += FILESYSTEM_PREFIX_LEN (name);

  for (; *name; name++)
    if (ISSLASH (*name))
      base = name + 1;

  return (char *) base;
}
