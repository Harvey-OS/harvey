/*
 *	Bitrate histogram include file
 *
 *	Copyright (c) 2000 Mark Taylor
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef LAME_BRHIST_H
#define LAME_BRHIST_H

#if defined(_WIN32)  &&  !defined(__CYGWIN__)
# include <windows.h>
#endif
#include "lame.h"

int   brhist_init       ( const lame_global_flags  *gf, const int bitrate_kbps_min, const int bitrate_kbps_max );
void  brhist_disp       ( const lame_global_flags  *gf );
void  brhist_disp_total ( const lame_global_flags  *gf );
void  brhist_jump_back  ( void );

typedef struct {
    FILE*   Console_fp;			/* filepointer to stream reporting information */
    FILE*   Error_fp;                   /* filepointer to stream fatal error reporting information */
    FILE*   Report_fp;                  /* filepointer to stream reports (normally a text file or /dev/null) */
#if defined(_WIN32)  &&  !defined(__CYGWIN__) 
    HANDLE  Console_Handle;
#endif
    int     disp_width;
    int     disp_height;
    char    str_up         [10];
    char    str_clreoln    [10];
    char    str_emph       [10];
    char    str_norm       [10];
    char    Console_buff [1024];
} Console_IO_t;

extern Console_IO_t  Console_IO;

#endif /* LAME_BRHIST_H */
