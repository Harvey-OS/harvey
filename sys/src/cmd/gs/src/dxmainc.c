/* Copyright (C) 2001 Ghostgum Software Pty Ltd.  All rights reserved.
  
  This software is provided AS-IS with no warranty, either express or
  implied.
  
  This software is distributed under license and may not be copied,
  modified or distributed except as expressly authorized under the terms
  of the license contained in the file LICENSE in this distribution.
  
  For more information about licensing, please refer to
  http://www.ghostscript.com/licensing/. For information on
  commercial licensing, go to http://www.artifex.com/licensing/ or
  contact Artifex Software, Inc., 101 Lucas Valley Road #110,
  San Rafael, CA  94903, U.S.A., +1(415)492-9861.
*/

/* $Id: dxmainc.c,v 1.10 2004/08/19 21:52:20 ghostgum Exp $ */

/* dxmainc.c */
/* 
 * Ghostscript frontend which provides a console to the Ghostscript
 * shared library.  Load time linking to libgs.so 
 * This does not support the display device.  Use dxmain.c/gsx for that,
 * or modify this program to use bare Xlib calls.
 * Compile using
 *    gcc -o gsc dxmainc.c -lgs
 *
 * The ghostscript library needs to be compiled with
 *  gcc -fPIC -g -c -Wall file.c
 *  gcc -shared -Wl,-soname,libgs.so.7 -o libgs.so.7.00 file.o -lc
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#define __PROTOTYPES__
#include "ierrors.h"
#include "iapi.h"

const char start_string[] = "systemdict /start get exec\n";

static int gsdll_stdin(void *instance, char *buf, int len);
static int gsdll_stdout(void *instance, const char *str, int len);
static int gsdll_stdout(void *instance, const char *str, int len);

/*********************************************************************/
/* stdio functions */

/* callback for reading stdin */
/* Use async input */
static int 
gsdll_stdin(void *instance, char *buf, int len)
{
    return read(fileno(stdin), buf, len);
}

static int 
gsdll_stdout(void *instance, const char *str, int len)
{
    fwrite(str, 1, len, stdout);
    fflush(stdout);
    return len;
}

static int 
gsdll_stderr(void *instance, const char *str, int len)
{
    fwrite(str, 1, len, stderr);
    fflush(stderr);
    return len;
}

/*********************************************************************/

int main(int argc, char *argv[])
{
    int exit_status;
    int code = 1, code1;
    void *instance;
    int exit_code;

    /* run Ghostscript */
    if ((code = gsapi_new_instance(&instance, NULL)) == 0) {
        gsapi_set_stdio(instance, gsdll_stdin, gsdll_stdout, gsdll_stderr);
	code = gsapi_init_with_args(instance, argc, argv);

	if (code == 0)
	    code = gsapi_run_string(instance, start_string, 0, &exit_code);
        code1 = gsapi_exit(instance);
	if (code == 0 || code == e_Quit)
	    code = code1;
	if (code == e_Quit)
	    code = 0;	/* user executed 'quit' */

	gsapi_delete_instance(instance);
    }

    exit_status = 0;
    switch (code) {
	case 0:
	case e_Info:
	case e_Quit:
	    break;
	case e_Fatal:
	    exit_status = 1;
	    break;
	default:
	    exit_status = 255;
    }

    return exit_status;
}

