/* Copyright (C) 2001 Ghostgum Software Pty Ltd.  All rights reserved.
  
  This file is part of AFPL Ghostscript.
  
  AFPL Ghostscript is distributed with NO WARRANTY OF ANY KIND.  No author or
  distributor accepts any responsibility for the consequences of using it, or
  for whether it serves any particular purpose or works at all, unless he or
  she says so in writing.  Refer to the Aladdin Free Public License (the
  "License") for full details.
  
  Every copy of AFPL Ghostscript must include a copy of the License, normally
  in a plain ASCII text file named PUBLIC.  The License grants you the right
  to copy, modify and redistribute AFPL Ghostscript, but only under certain
  conditions described in the License.  Among other things, the License
  requires that the copyright notice and this notice be preserved on all
  copies.
*/

/* $Id: dxmainc.c,v 1.4.2.1 2002/02/01 06:38:54 giles Exp $ */

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
#include "errors.h"
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
    int code = 1;
    gs_main_instance *instance;
    int exit_code;

    /* run Ghostscript */
    if ((code = gsapi_new_instance(&instance, NULL)) == 0) {
        gsapi_set_stdio(instance, gsdll_stdin, gsdll_stdout, gsdll_stderr);
	code = gsapi_init_with_args(instance, argc, argv);

	if (code == 0)
	    code = gsapi_run_string(instance, start_string, 0, &exit_code);
        gsapi_exit(instance);
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

