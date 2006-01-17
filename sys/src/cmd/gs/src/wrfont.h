/* Copyright (C) 1989, 2000 Aladdin Enterprises.  All rights reserved.

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

/* $Id: wrfont.h,v 1.2 2002/12/04 07:45:47 igor Exp $ */
/*
Header for support functions to serialize fonts as PostScript code that can
then be passed to FreeType via the FAPI FreeType bridge.
Started by Graham Asher, 9th August 2002.
*/

#ifndef wrfont_INCLUDED
#define wrfont_INCLUDED

#include "stdpre.h"

typedef struct WRF_output_
	{
	unsigned char* m_pos;
	long m_limit;
	long m_count;
	bool m_encrypt;
	unsigned short m_key;
	} WRF_output;

void WRF_init(WRF_output* a_output,unsigned char* a_buffer,long a_buffer_size);
void WRF_wbyte(WRF_output* a_output,unsigned char a_byte);
void WRF_wtext(WRF_output* a_output,const unsigned char* a_string,long a_length);
void WRF_wstring(WRF_output* a_output,const char* a_string);
void WRF_wfloat(WRF_output* a_output,double a_float);
void WRF_wint(WRF_output* a_output,long a_int);

#endif
