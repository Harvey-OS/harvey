/* Reading input lines for MFT.  Taken from weavext.c.
*/

#include <stdio.h>
#include "site.h"

/*
 * lineread reads from the Pascal text file with iorec pointer filep
 * into buffer[0], buffer[1],..., buffer[limit-1] (and
 * setting "limit").
 * Characters are read until a newline is found (which isn't put in the
 * buffer) or until the next character would go into buffer[BUF_SIZE].
 * And trailing blanks are to be ignored, so limit is really set to
 * one past the last non-blank.
 * The characters need to be translated, so really xord[c] is put into
 * the buffer when c is read.
 * If end-of-file is encountered, the funit field of *filep is set
 * appropriately.
 */

#define BUF_SIZE 100		/* should agree with mft.web */
extern schar buffer[];		/* 0..BUF_SIZE.  Input goes here */
extern schar xord[];		/* character translation arrays */
extern integer limit;		/* index into buffer.  */

void lineread(iop)
FILE *iop;
{
	register c;
	register schar *cs; /* pointer into buffer where next char goes */
	register schar *cnb; /* last non-blank character input */
	register l; /* how many chars allowed before buffer overflow */
	
	cnb = cs = &(buffer[0]);
	l = BUF_SIZE;
	  /* overflow when next char would go into buffer[BUF_SIZE] */
	while (--l>=0 && (c = getc(iop)) != EOF && c!='\n')
	    if ((*cs++ = xord[c])!=' ') cnb = cs;
	limit = cnb - &(buffer[0]);
}
