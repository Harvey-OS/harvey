/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

/* Copyright (C) 1999, Ghostgum Software Pty Ltd.  All rights reserved.
  
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


// $Id: dwsetup.h,v 1.6 2004/11/18 06:48:41 ghostgum Exp $

#ifndef dwsetup_INCLUDED
#  define dwsetup_INCLUDED

// Definitions for Ghostscript setup program

#ifndef IDC_STATIC
#define IDC_STATIC			-1
#endif

#define IDD_TEXTWIN                     101
#define IDD_DIRDLG                      102
#define IDR_MAIN                        200
#define IDD_MAIN                        201
#define IDC_TARGET_DIR                  202
#define IDC_TARGET_GROUP                203
#define IDC_BROWSE_DIR                  204
#define IDC_BROWSE_GROUP                205
#define IDC_README                      206
#define IDS_APPNAME                     501
#define IDS_TARGET_DIR                  502
#define IDS_TARGET_GROUP                503
#define IDC_PRODUCT_NAME                1000
#define IDC_INSTALL_FONTS               1001
#define IDC_TEXTWIN_MLE                 1002
#define IDC_TEXTWIN_COPY                1003
#define IDC_INSTALL                     1004
#define IDC_FILES                       1006
#define IDC_FOLDER                      1007
#define IDC_TARGET                      1008
#define IDC_ALLUSERS                    1009
#define IDC_COPYRIGHT                   1010
#define IDC_CJK_FONTS			1011


#endif /* dwsetup_INCLUDED */
