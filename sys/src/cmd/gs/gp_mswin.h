/* Copyright (C) 1992, 1993 Aladdin Enterprises.  All rights reserved.
  
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

/* gp_mswin.h */
/* Definitions common to MS Windows implementation */
/* (used by both C code and Windows 'resource') */
  
#define TEXT_ICON	50
#define IMAGE_ICON	51
#define SPOOL_PORT	100
#define CANCEL_PCDONE	101

#ifndef RC_INVOKED		/* NOTA BENE */

/* system menu constants for image window */
#define M_COPY_CLIP 1

/* externals from gp_mswin.c */

/* Patch 26.10.94 :for Microsoft C/C++ 8.0 32-Bit       */
/* "_export" is Microsoft 16-Bit specific.              */
/* With MS-C/C++ 8.00 32 Bit "_export" causes an error. */
#if defined(_WIN32) & defined(_MSC_VER)
#define _export
#endif

extern HWND hwndtext;
extern HINSTANCE phInstance;
extern const LPSTR szAppName;
extern const LPSTR szImgName;
extern BOOL is_win31;
extern BOOL is_winnt;
extern BOOL CALLBACK _export AbortProc(HDC, int);
/* imitation pipes */
extern HGLOBAL pipe_hglobal;
extern LPBYTE pipe_lpbyte;
extern UINT pipe_count;
#ifdef __WIN32__
extern LPBYTE pipe_mapptr;
extern BOOL pipe_eof;
#endif

/* for gsview.exe */
extern BOOL gsview;
extern HWND gsview_hwnd;
extern BOOL gsview_next;
extern LPSTR gsview_option;
/* messages used between gsview and gswin */
#define WM_GSVIEW WM_USER+0
/* from gswin to gsview */
#define HWND_TEXT	0
#define HWND_IMGCHILD	1
#define GSWIN_CLOSE	2
#define SYNC_OUTPUT	3
#define OUTPUT_PAGE	4
#define SCROLL_POSITION 5
#define PIPE_REQUEST	6
#define BEGIN		7
#define END		8
/* from gsview to gswin image window */
#define NEXT_PAGE	10
#define COPY_CLIPBOARD	11
/* from gsview to gswin text window */
#define PIPE_DATA	12

#endif				/* !defined(RC_INVOKED) */
