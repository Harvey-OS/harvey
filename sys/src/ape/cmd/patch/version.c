/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

/* Print the version number.  */

/* $Id: version.c,v 1.5 1997/05/21 18:29:20 eggert Exp $ */

#define XTERN extern
#include <common.h>
#undef XTERN
#define XTERN
#include <patchlevel.h>
#include <version.h>

static char const copyright_string[] = "\
Copyright 1988 Larry Wall\n\
Copyright 1997 Free Software Foundation, Inc.";

static char const free_software_msgid[] = "\
This program comes with NO WARRANTY, to the extent permitted by law.\n\
You may redistribute copies of this program\n\
under the terms of the GNU General Public License.\n\
For more information about these matters, see the file named COPYING.";

static char const authorship_msgid[] = "\
written by Larry Wall with lots o' patches by Paul Eggert";

void
version()
{
	printf("%s %s\n%s\n\n%s\n\n%s\n", program_name, PATCH_VERSION,
	       copyright_string, free_software_msgid, authorship_msgid);
}
