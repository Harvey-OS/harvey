/*ident	"@(#)cls4:incl-master/const-headers/stream.h	1.1" */
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

#ifndef STREAMH
#define STREAMH

#include <iostream.h>
#include <iomanip.h>
#include <stdiostream.h>
#include <fstream.h>
	/* for filebuf */

#ifndef NULL
#define NULL	0
#endif

extern char*  oct(long, int =0);
extern char*  dec(long, int =0);
extern char*  hex(long, int =0);

extern char*  chr(int, int =0);	/* chr(0) is the empty string "" */
extern char*  str(const char*, int =0);
extern char*  form(const char* ...);
			/* printf format
                         * Things may go terribly wrong (maybe even core
                         * dumps, if form tries to create a string with
                         * more than "max_field_width" characters. */

/* WS used to be a special in streams. The WS manipulator
 * is implemented differently but may be extracted from an istream
 * with the same effect as the old form.
 */

extern istream& WS(istream&) ;
extern void eatwhite(istream&) ;

static const int input = (ios::in) ;
static const int output = (ios::out) ;
static const int append = (ios::app) ;
static const int atend = (ios::ate) ;
static const int _good = (ios::goodbit) ;
static const int _bad = (ios::badbit) ;
static const int _fail = (ios::failbit) ;
static const int _eof = (ios::eofbit) ;

typedef ios::io_state state_value ;

#endif
