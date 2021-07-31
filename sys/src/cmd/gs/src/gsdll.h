/* Copyright (C) 1994-2001, Ghostgum Software Pty Ltd.  All rights reserved.
  Portions Copyright (C) 1999 Aladdin Enterprises.  All rights reserved.
  
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


/*$Id: gsdll.h,v 1.3 2001/03/13 07:09:28 ghostgum Exp $ */

/* This interface is deprecated and will be removed in future
 * ghostscript releases.  Use the interface described in
 * API.htm and iapi.h.
 */

#include "iapi.h"

#ifndef gsdll_INCLUDED
#  define gsdll_INCLUDED
  

#ifdef __MACINTOSH__
#define HWND char *
#include <QDOffscreen.h>
#pragma export on
#endif

#ifdef __WINDOWS__
#define _Windows
#endif

#ifdef __IBMC__
#define GSDLLCALLLINK _System
#else
#define GSDLLCALLLINK
#endif

/* global pointer to callback */
typedef int (* GSDLLCALLLINK GSDLL_CALLBACK) (int, char *, unsigned long);
extern GSDLL_CALLBACK pgsdll_callback;

/* message values for callback */
#define GSDLL_STDIN 1		/* get count characters to str from stdin */
			/* return number of characters read */
#define GSDLL_STDOUT 2		/* put count characters from str to stdout */
			/* return number of characters written */
#define GSDLL_DEVICE 3		/* device = str has been opened if count=1 */
			/*                    or closed if count=0 */
#define GSDLL_SYNC 4		/* sync_output for device str */
#define GSDLL_PAGE 5		/* output_page for device str */
#define GSDLL_SIZE 6		/* resize for device str */
			/* LOWORD(count) is new xsize */
			/* HIWORD(count) is new ysize */
#define GSDLL_POLL 7		/* Called from gp_check_interrupt */
			/* Can be used by caller to poll the message queue */
			/* Normally returns 0 */
			/* To abort gsdll_execute_cont(), return a */
			/* non zero error code until gsdll_execute_cont() */
			/* returns */

/* return values from gsdll_init() */
#define GSDLL_INIT_IN_USE  100	/* DLL is in use */
#define GSDLL_INIT_QUIT    101	/* quit or EOF during init */
				  /* This is not an error. */
				  /* gsdll_exit() must not be called */


/* DLL exported  functions */
/* for load time dynamic linking */
GSDLLEXPORT int GSDLLAPI gsdll_revision(const char * * product, const char * * copyright, long * gs_revision, long * gs_revisiondate);
GSDLLEXPORT int GSDLLAPI gsdll_init(GSDLL_CALLBACK callback, HWND hwnd, int argc, char * * argv);
GSDLLEXPORT int GSDLLAPI gsdll_execute_begin(void);
GSDLLEXPORT int GSDLLAPI gsdll_execute_cont(const char * str, int len);
GSDLLEXPORT int GSDLLAPI gsdll_execute_end(void);
GSDLLEXPORT int GSDLLAPI gsdll_exit(void);
GSDLLEXPORT int GSDLLAPI gsdll_lock_device(unsigned char *device, int flag);

/* Function pointer typedefs */
/* for run time dynamic linking */
typedef int (GSDLLAPIPTR PFN_gsdll_revision)(const char ** product, 
    const char ** copyright, long * revision, long * revisiondate);
typedef int (GSDLLAPIPTR PFN_gsdll_init) (GSDLL_CALLBACK, HWND, int argc, char * * argv);
typedef int (GSDLLAPIPTR PFN_gsdll_execute_begin) (void);
typedef int (GSDLLAPIPTR PFN_gsdll_execute_cont) (const char * str, int len);
typedef int (GSDLLAPIPTR PFN_gsdll_execute_end) (void);
typedef int (GSDLLAPIPTR PFN_gsdll_exit) (void);
typedef int (GSDLLAPIPTR PFN_gsdll_lock_device) (unsigned char *, int);

#ifdef __MACINTOSH__
#pragma export off
#endif

#endif /* gsdll_INCLUDED */
