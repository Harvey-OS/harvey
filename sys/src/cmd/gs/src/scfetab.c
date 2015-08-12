/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

/* Copyright (C) 1992, 1998, 1999 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: scfetab.c,v 1.5 2002/06/16 03:58:14 lpd Exp $ */
/* Tables for CCITTFaxEncode filter */
#include "std.h"
#include "scommon.h" /* for scf.h */
#include "scf.h"

/* We make this a separate file so that it can be used by */
/* the program that generates the tables for the CCITTFaxDecode filter. */

/* ------ Run encoding tables ------ */

/* Abbreviate hce_entry to make the file smaller. */
#define RUN(c, len) hce_entry(c, len)

/* Define the end-of-line code. */
/* Code in scfd.c and scfdgen.c knows that the run value is 1. */
const cfe_run cf_run_eol =
    RUN(run_eol_code_value, run_eol_code_length);

/* Define the 1-D code that signals uncompressed data. */
const cfe_run cf1_run_uncompressed =
    RUN(0xf, 12);

/* Define the 2-D run codes. */
const cfe_run cf2_run_pass =
    RUN(cf2_run_pass_value, cf2_run_pass_length);
const cfe_run cf2_run_vertical[7] =
    {
     RUN(0x3, 7),
     RUN(0x3, 6),
     RUN(0x3, 3),
     RUN(0x1, 1),
     RUN(0x2, 3),
     RUN(0x2, 6),
     RUN(0x2, 7)};
const cfe_run cf2_run_horizontal =
    RUN(cf2_run_horizontal_value, cf2_run_horizontal_length);
const cfe_run cf2_run_uncompressed =
    RUN(0xf, 10);

/* EOL codes for Group 3 2-D. */
/* Code in scfd.c knows that these are 0...01x. */
const cfe_run cf2_run_eol_1d =
    RUN((run_eol_code_value << 1) + 1, run_eol_code_length + 1);
const cfe_run cf2_run_eol_2d =
    RUN((run_eol_code_value << 1) + 0, run_eol_code_length + 1);

/* White run codes. */
const cf_runs cf_white_runs =
    {
     {/* Termination codes */
      RUN(0x35, 8), RUN(0x7, 6), RUN(0x7, 4), RUN(0x8, 4),
      RUN(0xb, 4), RUN(0xc, 4), RUN(0xe, 4), RUN(0xf, 4),
      RUN(0x13, 5), RUN(0x14, 5), RUN(0x7, 5), RUN(0x8, 5),
      RUN(0x8, 6), RUN(0x3, 6), RUN(0x34, 6), RUN(0x35, 6),
      RUN(0x2a, 6), RUN(0x2b, 6), RUN(0x27, 7), RUN(0xc, 7),
      RUN(0x8, 7), RUN(0x17, 7), RUN(0x3, 7), RUN(0x4, 7),
      RUN(0x28, 7), RUN(0x2b, 7), RUN(0x13, 7), RUN(0x24, 7),
      RUN(0x18, 7), RUN(0x2, 8), RUN(0x3, 8), RUN(0x1a, 8),
      RUN(0x1b, 8), RUN(0x12, 8), RUN(0x13, 8), RUN(0x14, 8),
      RUN(0x15, 8), RUN(0x16, 8), RUN(0x17, 8), RUN(0x28, 8),
      RUN(0x29, 8), RUN(0x2a, 8), RUN(0x2b, 8), RUN(0x2c, 8),
      RUN(0x2d, 8), RUN(0x4, 8), RUN(0x5, 8), RUN(0xa, 8),
      RUN(0xb, 8), RUN(0x52, 8), RUN(0x53, 8), RUN(0x54, 8),
      RUN(0x55, 8), RUN(0x24, 8), RUN(0x25, 8), RUN(0x58, 8),
      RUN(0x59, 8), RUN(0x5a, 8), RUN(0x5b, 8), RUN(0x4a, 8),
      RUN(0x4b, 8), RUN(0x32, 8), RUN(0x33, 8), RUN(0x34, 8)},
     {/* Make-up codes */
      RUN(0, 0) /* dummy */, RUN(0x1b, 5), RUN(0x12, 5), RUN(0x17, 6),
      RUN(0x37, 7), RUN(0x36, 8), RUN(0x37, 8), RUN(0x64, 8),
      RUN(0x65, 8), RUN(0x68, 8), RUN(0x67, 8), RUN(0xcc, 9),
      RUN(0xcd, 9), RUN(0xd2, 9), RUN(0xd3, 9), RUN(0xd4, 9),
      RUN(0xd5, 9), RUN(0xd6, 9), RUN(0xd7, 9), RUN(0xd8, 9),
      RUN(0xd9, 9), RUN(0xda, 9), RUN(0xdb, 9), RUN(0x98, 9),
      RUN(0x99, 9), RUN(0x9a, 9), RUN(0x18, 6), RUN(0x9b, 9),
      RUN(0x8, 11), RUN(0xc, 11), RUN(0xd, 11), RUN(0x12, 12),
      RUN(0x13, 12), RUN(0x14, 12), RUN(0x15, 12), RUN(0x16, 12),
      RUN(0x17, 12), RUN(0x1c, 12), RUN(0x1d, 12), RUN(0x1e, 12),
      RUN(0x1f, 12)}};

/* Black run codes. */
const cf_runs cf_black_runs =
    {
     {/* Termination codes */
      RUN(0x37, 10), RUN(0x2, 3), RUN(0x3, 2), RUN(0x2, 2),
      RUN(0x3, 3), RUN(0x3, 4), RUN(0x2, 4), RUN(0x3, 5),
      RUN(0x5, 6), RUN(0x4, 6), RUN(0x4, 7), RUN(0x5, 7),
      RUN(0x7, 7), RUN(0x4, 8), RUN(0x7, 8), RUN(0x18, 9),
      RUN(0x17, 10), RUN(0x18, 10), RUN(0x8, 10), RUN(0x67, 11),
      RUN(0x68, 11), RUN(0x6c, 11), RUN(0x37, 11), RUN(0x28, 11),
      RUN(0x17, 11), RUN(0x18, 11), RUN(0xca, 12), RUN(0xcb, 12),
      RUN(0xcc, 12), RUN(0xcd, 12), RUN(0x68, 12), RUN(0x69, 12),
      RUN(0x6a, 12), RUN(0x6b, 12), RUN(0xd2, 12), RUN(0xd3, 12),
      RUN(0xd4, 12), RUN(0xd5, 12), RUN(0xd6, 12), RUN(0xd7, 12),
      RUN(0x6c, 12), RUN(0x6d, 12), RUN(0xda, 12), RUN(0xdb, 12),
      RUN(0x54, 12), RUN(0x55, 12), RUN(0x56, 12), RUN(0x57, 12),
      RUN(0x64, 12), RUN(0x65, 12), RUN(0x52, 12), RUN(0x53, 12),
      RUN(0x24, 12), RUN(0x37, 12), RUN(0x38, 12), RUN(0x27, 12),
      RUN(0x28, 12), RUN(0x58, 12), RUN(0x59, 12), RUN(0x2b, 12),
      RUN(0x2c, 12), RUN(0x5a, 12), RUN(0x66, 12), RUN(0x67, 12)},
     {/* Make-up codes. */
      RUN(0, 0) /* dummy */, RUN(0xf, 10), RUN(0xc8, 12), RUN(0xc9, 12),
      RUN(0x5b, 12), RUN(0x33, 12), RUN(0x34, 12), RUN(0x35, 12),
      RUN(0x6c, 13), RUN(0x6d, 13), RUN(0x4a, 13), RUN(0x4b, 13),
      RUN(0x4c, 13), RUN(0x4d, 13), RUN(0x72, 13), RUN(0x73, 13),
      RUN(0x74, 13), RUN(0x75, 13), RUN(0x76, 13), RUN(0x77, 13),
      RUN(0x52, 13), RUN(0x53, 13), RUN(0x54, 13), RUN(0x55, 13),
      RUN(0x5a, 13), RUN(0x5b, 13), RUN(0x64, 13), RUN(0x65, 13),
      RUN(0x8, 11), RUN(0xc, 11), RUN(0xd, 11), RUN(0x12, 12),
      RUN(0x13, 12), RUN(0x14, 12), RUN(0x15, 12), RUN(0x16, 12),
      RUN(0x17, 12), RUN(0x1c, 12), RUN(0x1d, 12), RUN(0x1e, 12),
      RUN(0x1f, 12)}};

/* Uncompressed codes. */
const cfe_run cf_uncompressed[6] =
    {
     RUN(1, 1),
     RUN(1, 2),
     RUN(1, 3),
     RUN(1, 4),
     RUN(1, 5),
     RUN(1, 6)};

/* Uncompressed exit codes. */
const cfe_run cf_uncompressed_exit[10] =
    {
     RUN(2, 8), RUN(3, 8),
     RUN(2, 9), RUN(3, 9),
     RUN(2, 10), RUN(3, 10),
     RUN(2, 11), RUN(3, 11),
     RUN(2, 12), RUN(3, 12)};

/* Some C compilers insist on having executable code in every file.... */
void scfetab_dummy(void); /* for picky compilers */
void
scfetab_dummy(void)
{
}
