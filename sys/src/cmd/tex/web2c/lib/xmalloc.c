/* xmalloc.c: malloc with error checking.

Copyright (C) 1992 Free Software Foundation, Inc.

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

#include "config.h"


address
xmalloc P1C(unsigned, size)
{
  address new_mem = (address) malloc (size);

  if (new_mem == NULL)
    {
      fprintf (stderr, "xmalloc: request for %u bytes failed.\n", size);
      abort ();
    }

  return new_mem;
}
