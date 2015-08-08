/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

/* Copyright (C) 1993, 1998 Aladdin Enterprises.  All rights reserved.

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

/* $Id: gdevemap.c,v 1.4 2002/02/21 22:24:51 giles Exp $ */
/* Mappings between StandardEncoding and ISOLatin1Encoding */
#include "std.h"

const byte gs_map_std_to_iso[256] = {
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   32,  33,  34,  35,  36,  37,  38,  39,  40,  41,  42,  43,  44,
    173, 46,  47,  48,  49,  50,  51,  52,  53,  54,  55,  56,  57,  58,  59,
    60,  61,  62,  63,  64,  65,  66,  67,  68,  69,  70,  71,  72,  73,  74,
    75,  76,  77,  78,  79,  80,  81,  82,  83,  84,  85,  86,  87,  88,  89,
    90,  91,  92,  93,  94,  95,  96,  97,  98,  99,  100, 101, 102, 103, 104,
    105, 106, 107, 108, 109, 110, 111, 112, 113, 114, 115, 116, 117, 118, 119,
    120, 121, 122, 123, 124, 125, 126, 0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   161, 162, 163, 0,
    165, 0,   167, 164, 0,   0,   171, 0,   0,   0,   0,   0,   0,   0,   0,
    183, 0,   182, 0,   0,   0,   0,   187, 0,   0,   0,   191, 0,   145, 180,
    147, 148, 175, 150, 151, 168, 0,   154, 184, 0,   157, 158, 159, 0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    198, 0,   170, 0,   0,   0,   0,   0,   216, 0,   186, 0,   0,   0,   0,
    0,   230, 0,   0,   0,   144, 0,   0,   0,   248, 0,   223, 0,   0,   0,
    0};

const byte gs_map_iso_to_std[256] = {
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   32,  33,  34,  35,  36,  37,  38,  39,  40,  41,  42,  43,  44,
    0,   46,  47,  48,  49,  50,  51,  52,  53,  54,  55,  56,  57,  58,  59,
    60,  61,  62,  63,  64,  65,  66,  67,  68,  69,  70,  71,  72,  73,  74,
    75,  76,  77,  78,  79,  80,  81,  82,  83,  84,  85,  86,  87,  88,  89,
    90,  91,  92,  93,  94,  95,  96,  97,  98,  99,  100, 101, 102, 103, 104,
    105, 106, 107, 108, 109, 110, 111, 112, 113, 114, 115, 116, 117, 118, 119,
    120, 121, 122, 123, 124, 125, 126, 0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   245, 193, 194, 195, 196, 197,
    198, 199, 200, 0,   202, 203, 0,   205, 206, 207, 32,  161, 162, 163, 168,
    165, 0,   167, 200, 0,   227, 171, 0,   45,  0,   197, 0,   0,   0,   0,
    194, 0,   182, 180, 203, 0,   235, 187, 0,   0,   0,   191, 0,   0,   0,
    0,   0,   0,   225, 0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   233, 0,   0,   0,   0,   0,   0,   251, 0,
    0,   0,   0,   0,   0,   241, 0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   249, 0,   0,   0,   0,   0,   0,
    0};
