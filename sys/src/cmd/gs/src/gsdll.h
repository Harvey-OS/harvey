/* Copyright (C) 1994-1996, Russell Lang.  All rights reserved.
   Portions Copyright (C) 1999 Aladdin Enterprises.  All rights reserved.

   This file is part of Aladdin Ghostscript.

   Aladdin Ghostscript is distributed with NO WARRANTY OF ANY KIND.  No author
   or distributor accepts any responsibility for the consequences of using it,
   or for whether it serves any particular purpose or works at all, unless he
   or she says so in writing.  Refer to the Aladdin Ghostscript Free Public
   License (the "License") for full details.

   Every copy of Aladdin Ghostscript must include a copy of the License,
   normally in a plain ASCII text file named PUBLIC.  The License grants you
   the right to copy, modify and redistribute Aladdin Ghostscript, but only
   under certain conditions described in the License.  Among other things, the
   License requires that the copyright notice and this notice be preserved on
   all copies.
 */


/*$Id: gsdll.h,v 1.1 2000/03/09 08:40:42 lpd Exp $ */

#ifndef gsdll_INCLUDED
#  define gsdll_INCLUDED
  
#ifdef __MACINTOSH__

#define HWND char *
#define GSFAR 
#include <QDOffscreen.h>
#pragma export on

#endif

#ifndef GSDLLEXPORT
#define GSDLLEXPORT
#endif

#ifdef __WINDOWS__
#define _Windows
#endif

/* type of exported functions */
#ifdef _Windows
#ifdef _WATCOM_
#define GSDLLAPI GSDLLEXPORT
#else
#define GSDLLAPI CALLBACK GSDLLEXPORT
#endif
#else
#ifdef __IBMC__
#define GSDLLAPI _System
#else
#define GSDLLAPI
#endif
#endif

#ifdef _Windows
#define GSDLLCALLLINK
#define GSFAR FAR
#else
#ifdef __IBMC__
#define GSDLLCALLLINK _System
#else
#define GSDLLCALLLINK
#endif
#define GSFAR
#endif

/* global pointer to callback */
typedef int (GSFAR * GSDLLCALLLINK GSDLL_CALLBACK) (int, char GSFAR *, unsigned long);
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
int GSDLLAPI gsdll_revision(char GSFAR * GSFAR * product, char GSFAR * GSFAR * copyright, long GSFAR * gs_revision, long GSFAR * gs_revisiondate);
int GSDLLAPI gsdll_init(GSDLL_CALLBACK callback, HWND hwnd, int argc, char GSFAR * GSFAR * argv);
int GSDLLAPI gsdll_execute_begin(void);
int GSDLLAPI gsdll_execute_cont(const char GSFAR * str, int len);
int GSDLLAPI gsdll_execute_end(void);
int GSDLLAPI gsdll_exit(void);
int GSDLLAPI gsdll_lock_device(unsigned char *device, int flag);

/* Function pointer typedefs */
/* for run time dynamic linking */
typedef int (GSDLLAPI * PFN_gsdll_revision) (char GSFAR * GSFAR *, char GSFAR * GSFAR *, long GSFAR *, long GSFAR *);
typedef int (GSDLLAPI * PFN_gsdll_init) (GSDLL_CALLBACK, HWND, int argc, char GSFAR * GSFAR * argv);
typedef int (GSDLLAPI * PFN_gsdll_execute_begin) (void);
typedef int (GSDLLAPI * PFN_gsdll_execute_cont) (const char GSFAR * str, int len);
typedef int (GSDLLAPI * PFN_gsdll_execute_end) (void);
typedef int (GSDLLAPI * PFN_gsdll_exit) (void);
typedef int (GSDLLAPI * PFN_gsdll_lock_device) (unsigned char GSFAR *, int);

#ifdef __MACINTOSH__
#pragma export off
#endif

#endif /* gsdll_INCLUDED */
