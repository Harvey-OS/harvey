/* $Id: ps2pdf12.cmd,v 1.1 2000/07/05 16:21:13 lpd Exp $ */
/*
 * This file is maintained by a user: if you have any questions about it,
 * please contact Mark Hale (mark.hale@physics.org).
 */

/* Convert PostScript to PDF 1.2 (Acrobat 3-and-later compatible). */

parse arg params

call 'ps2pdf' '-dCompatibilityLevel=1.2' params
