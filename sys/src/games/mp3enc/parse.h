/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

int  print_license(const lame_global_flags* gfp, FILE* const fp, const char* ProgramName);
int  usage(const lame_global_flags* gfp, FILE* const fp, const char* ProgramName);
int  short_help(const lame_global_flags* gfp, FILE* const fp, const char* ProgramName);
int  long_help(const lame_global_flags* gfp, FILE* const fp, const char* ProgramName, int lessmode);
int  display_bitrates(FILE* const fp);

int  parse_args(lame_global_flags* gfp, int argc, char** argv);
void print_config(lame_global_flags* gfp);
