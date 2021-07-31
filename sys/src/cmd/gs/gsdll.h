/* Copyright (C) 1994, Russell Lang.  All rights reserved.
  
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


/* gsdll.h */

/* type of exported functions */
#ifdef _Windows
/* avoid including <windows.h> just to get CALLBACK */
#ifdef __WIN32__
#define DLL_EXPORT _pascal _export
#else
#define DLL_EXPORT _far _pascal _export
#endif
#else
#define DLL_EXPORT
#endif

/* global pointer to callback */
typedef int (*GSDLL_CALLBACK)(int, char *, unsigned long);
extern GSDLL_CALLBACK pgsdll_callback;

/* message values for callback */
#define GSDLL_STDIN 1   /* get count characters to str from stdin */
			/* return number of characters read */
#define GSDLL_STDOUT 2  /* put count characters from str to stdout*/
			/* return number of characters written */
#define GSDLL_DEVICE 3  /* device = str has been opened if count=1 */
			/*                    or closed if count=0 */
#define GSDLL_SYNC 4    /* sync_output for device str */ 
#define GSDLL_PAGE 5    /* output_page for device str */
#define GSDLL_SIZE 6    /* resize for device str */
			/* LOWORD(count) is new xsize */
			/* HIWORD(count) is new ysize */


/* DLL exported  functions */
int DLL_EXPORT gsdll_init(GSDLL_CALLBACK callback, char *str);
int DLL_EXPORT gsdll_execute(char *str);
int DLL_EXPORT gsdll_exit(void);
int DLL_EXPORT gsdll_revision(char **product, char **copyright, long *gs_revision, long *gs_revisiondate);

