/* xrealloc.c: realloc with error checking.

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
xrealloc P2C(address, old_ptr, unsigned, size)
{
  address new_mem;

  if (old_ptr == NULL)
    new_mem = xmalloc (size);
  else
    {
      new_mem = (address) realloc (old_ptr, size);
      if (new_mem == NULL)
        {
          fprintf (stderr, "xmalloc: request for %x to be %u bytes failed.\n",
                   (unsigned) old_ptr, size);
          abort ();
        }
    }

  return new_mem;
}
