/* $Source: /u/mark/src/pax/RCS/append.c,v $
 *
 * $Revision: 1.2 $
 *
 * append.c - append to a tape archive. 
 *
 * DESCRIPTION
 *
 *	Routines to allow appending of archives
 *
 * AUTHORS
 *
 *     	Mark H. Colburn, NAPS International (mark@jhereg.mn.org)
 *
 *
 * Sponsored by The USENIX Association for public distribution. 
 *
 * Copyright (c) 1989 Mark H. Colburn.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that the above copyright notice is duplicated in all such 
 * forms and that any documentation, advertising materials, and other 
 * materials related to such distribution and use acknowledge that the 
 * software was developed * by Mark H. Colburn and sponsored by The 
 * USENIX Association. 
 *
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 * $Log:	append.c,v $
 * Revision 1.2  89/02/12  10:03:58  mark
 * 1.2 release fixes
 * 
 * Revision 1.1  88/12/23  18:02:00  mark
 * Initial revision
 * 
 */

#ifndef lint
static char *ident = "$Id: append.c,v 1.2 89/02/12 10:03:58 mark Exp $";
static char *copyright = "Copyright (c) 1989 Mark H. Colburn.\nAll rights reserved.\n";
#endif /* ! lint */


/* Headers */

#include "pax.h"


/* append_archive - main loop for appending to a tar archive
 *
 * DESCRIPTION
 *
 *	Append_archive reads an archive until the end of the archive is
 *	reached once the archive is reached, the buffers are reset and the
 *	create_archive function is called to handle the actual writing of
 *	the appended archive data.  This is quite similar to the
 *	read_archive function, however, it does not do all the processing.
 */

#ifdef __STDC__

void append_archive(void)

#else

void append_archive()

#endif
{
    Stat            sb;
    char            name[PATH_MAX + 1];

    name[0] = '\0';
    while (get_header(name, &sb) == 0) {
	if (((ar_format == TAR)
	     ? buf_skip(ROUNDUP((OFFSET) sb.sb_size, BLOCKSIZE))
	     : buf_skip((OFFSET) sb.sb_size)) < 0) {
	    warn(name, "File data is corrupt");
	}
    }
    /* we have now gotten to the end of the archive... */

    /* reset the buffer now that we have read the entire archive */
    bufend = bufidx = bufstart;
    create_archive();
}
