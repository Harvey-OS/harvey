/* file-p.c: file predicates.

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

#include "xstat.h"

/* Test whether FILENAME1 and FILENAME2 are actually the same file.  If
   stat(2) fails on either of the names, we return false, without giving
   an error message.  */

boolean
same_file_p P2C(string, filename1, string, filename2)
{
    struct stat sb1, sb2;
    /* These are put in variables only so the results can be inspected
       under gdb.  */
    int r1 = stat (filename1, &sb1);
    int r2 = stat (filename2, &sb2);

    if (r1 == 0 && r2 == 0)
      return sb1.st_ino == sb2.st_ino && sb2.st_dev == sb2.st_dev;
    else
      return false;
}
