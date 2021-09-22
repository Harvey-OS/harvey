/****************************************************************
Copyright 1990 - 1995 by AT&T Bell Laboratories.

Permission to use, copy, modify, and distribute this software
and its documentation for any purpose and without fee is hereby
granted, provided that the above copyright notice appear in all
copies and that both that the copyright notice and this
permission notice and warranty disclaimer appear in supporting
documentation, and that the names of AT&T Bell Laboratories or
any of its entities not be used in advertising or publicity
pertaining to distribution of the software without specific,
written prior permission.

AT&T disclaims all warranties with regard to this software,
including all implied warranties of merchantability and fitness.
In no event shall AT&T be liable for any special, indirect or
consequential damages or any damages whatsoever resulting from
loss of use, data or profits, whether in an action of contract,
negligence or other tortious action, arising out of or in
connection with the use or performance of this software.
****************************************************************/

#include "c-auto.h"     /* In case we need, e.g., _POSIX_SOURCE */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>

#ifdef WIN32
#include <string.h>
#endif

/*
 *      newer x y
 *
 *      returns 0 if x exists and y does not, or if
 *      files x and y both exist and x was modified
 *      at least as recently as y, and nonzero otherwise.
 *      (i.e., it's really ``is x older than y'', not ``is y newer than x'')
 *      Perhaps this program should be replaced by ls -lt, but then two
 *      files with the same mtime couldn't be handled right ...
 */

int main (argc, argv)
        int argc;
        char *argv[];
{
        struct stat x, y;

        if (argc > 1 && strcmp (argv[1], "--help") == 0) {
          fputs ("Usage: newer [OPTION]... FILE1 FILE2\n\
  Exit successfully if FILE1 exists and is at least as old as FILE2.\n\
\n\
--help      display this help and exit\n\
--version   output version information and exit\n\n", stdout);
          fputs ("Email bug reports to tex-k@mail.tug.org.\n", stdout);
          exit(0);
        } else if (argc > 1 && strcmp (argv[1], "--version") == 0) {
          printf ("newer%s 0.63\n\
Copyright (C) 1996 AT&T Bell Laboratories.\n\
There is NO warranty.  You may redistribute this software\n\
under the terms of the GNU General Public License\n\
and the newer copyright.\n\
For more information about these matters, see the files\n\
named COPYING and the newer source.\n\
Primary author of newer: John Hobby.\n",
WEB2CVERSION);
          exit (0);
        }

        /* insist on exactly two args */
        if (argc != 3) {
          fputs ("newer: Need exactly two arguments.\n", stderr);
          fputs ("Try `newer --help' for more information.\n", stderr);
          exit(1);
        }
        
        /* does the first file exist? */
        if (stat (argv[1], &x) < 0)
                exit(1);
        
        /* does the second file exist? */
        if (stat (argv[2], &y) < 0)
                exit(0);
        
        /* fail if the first file is older than the second */
        if (x.st_mtime < y.st_mtime)
                exit(1);
        
        /* otherwise, succeed */
        exit(0);
}
