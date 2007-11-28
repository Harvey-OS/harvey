/*
 *	Lame time routines source file
 *
 *	Copyright (c) 2000 Mark Taylor
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

/* $Id: lametime.c,v 1.10 2001/02/26 18:57:20 markt Exp $ */

/*
 * name:        GetCPUTime ( void )
 *
 * description: returns CPU time used by the process
 * input:       none
 * output:      time in seconds
 * known bugs:  may not work in SMP and RPC
 * conforming:  ANSI C
 *
 * There is some old difficult to read code at the end of this file.
 * Can someone integrate this into this function (if useful)?
 */

#define _BSD_EXTENSION

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>

#include "lametime.h"

double GetCPUTime ( void )
{
    return clock () / (double) CLOCKS_PER_SEC;
}

/*
 * name:        GetRealTime ( void )
 *
 * description: returns real (human) time elapsed relative to a fixed time (mostly 1970-01-01 00:00:00)
 * input:       none
 * output:      time in seconds
 * known bugs:  bad precision with time()
 */

double GetRealTime ( void )			/* conforming:  SVr4, BSD 4.3 */
{
    struct timeval  t;

    if ( 0 != gettimeofday (&t, NULL) )
        assert (0);
    return t.tv_sec + 1.e-6 * t.tv_usec;
} 

int  lame_set_stream_binary_mode ( FILE* const fp )
{
    return 0;
}

off_t  lame_get_file_size ( const char* const filename )
{
    struct stat       sb;

    if ( 0 == stat ( filename, &sb ) )
        return sb.st_size;
    return (off_t) -1;
}
