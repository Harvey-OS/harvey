/*ident	"@(#)cls4:lib/stream/streamdefs.h	1.1" */
/*******************************************************************************
 
C++ source for the C++ Language System, Release 3.0.  This product
is a new release of the original cfront developed in the computer
science research center of AT&T Bell Laboratories.

Copyright (c) 1991 AT&T and UNIX System Laboratories, Inc.
Copyright (c) 1984, 1989, 1990 AT&T.  All Rights Reserved.

THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE of AT&T and UNIX System
Laboratories, Inc.  The copyright notice above does not evidence
any actual or intended publication of such source code.

*******************************************************************************/


// This file contains #defines for controlling conditional compilation 
// of the stream library


// The follwing functions adjust for machine dependencies

#define BREAKEVEN	0    
			/* The approximate length of a string
			 * for which it is faster to do a strncpy
			 * than a char by char loop. If BREAKEVEN is 0 
			 * then strncpy is always better.  If it is <0 then 
			 * loop is always better, (e.g. if strncopy does
			 * a char by char copy anyway.)
			 */

#define SEEK_ARITH_OK	1
			/* System supports arithmetic on stream positions.
			 * I.e. if file is at a position and we read or
			 * write n bytes we can find the new position
			 * by adding n to old position. (Providing
			 * O_APPEND isn't set in on open.)
			 */

static const int PTRBASE = ios::hex ;
			/* base for output of pointers (void*) */

// There is one important machine dependent feature of this implementation
// It assumes that it can always create a pointer to the byte after
// a char array used as a buffer, and that pointer will be greater than
// any pointer into the array. 
// My reading of the ANSI standard is that this assumption is permissible,
// but I can imagine segmented architectures where it fails.

#define VSPRINTF vsprintf

			/* If defined, the name of a "vsprintf" function.
			 * If not defined, 
			 * various non-portable kludges are used in
			 * oldformat.c
			 */

static const int STREAMBUFSIZE = 1024 ;
			// The default buffer size.

/*******
	#define O_CREAT 01000 
	#define O_TRUNC 02000 
	#define O_EXCL  04000
 *******/
			/* Used in filebuf.c.  Define if your system
			 * needs it to have a value different from
			 * that indicated here, but doesn't
			 * define it in standard system headers
			 */

/**** 
	#define STDIO_ONLY
 ****/
			/* If STDIO_ONLY is defined, then filebuf.c
			 * will be compiled to only use stdio functions.
			 * This does not provide complete functionality
			 * (e.g. some open modes are not supported), but
			 * is a way to port iostreams to non UNIX 
			 * environments.  
			 */ 
