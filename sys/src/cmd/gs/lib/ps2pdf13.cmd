/* $Id: ps2pdf13.cmd,v 1.2 2000/09/10 19:32:44 lpd Exp $ */
/*
 * This file is maintained by a user: if you have any questions about it,
 * please contact Mark Hale (mark.hale@physics.org).
 */

/* Convert PostScript to PDF 1.3 (Acrobat 4-and-later compatible). */

parse arg params

call 'ps2pdf' '-dCompatibilityLevel=1.3' params
