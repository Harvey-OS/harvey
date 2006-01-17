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

/* $Id: wrfont.c,v 1.3 2003/05/05 12:57:13 igor Exp $ */
/*
Support functions to serialize fonts as PostScript code that can
then be passed to FreeType via the FAPI FreeType bridge.
Started by Graham Asher, 9th August 2002.
*/

#include "wrfont.h"
#include "stdio_.h"

#define EEXEC_KEY 55665
#define EEXEC_FACTOR 52845
#define EEXEC_OFFSET 22719

void WRF_init(WRF_output* a_output,unsigned char* a_buffer,long a_buffer_size)
	{
	a_output->m_pos = a_buffer;
	a_output->m_limit = a_buffer_size;
	a_output->m_count = 0;
	a_output->m_encrypt = false;
	a_output->m_key = EEXEC_KEY;
	}

void WRF_wbyte(WRF_output* a_output,unsigned char a_byte)
	{
	if (a_output->m_count < a_output->m_limit)
		{
		if (a_output->m_encrypt)
			{
			a_byte ^= (a_output->m_key >> 8);
			a_output->m_key = (unsigned short)((a_output->m_key + a_byte) * EEXEC_FACTOR + EEXEC_OFFSET);
			}
		*a_output->m_pos++ = a_byte;
		}
	a_output->m_count++;
	}

void WRF_wtext(WRF_output* a_output,const unsigned char* a_string,long a_length)
	{
	while (a_length > 0)
		{
		WRF_wbyte(a_output,*a_string++);
		a_length--;
		}
	}

void WRF_wstring(WRF_output* a_output,const char* a_string)
	{
	while (*a_string)
		WRF_wbyte(a_output,*a_string++);
	}

void WRF_wfloat(WRF_output* a_output,double a_float)
	{
	char buffer[32];
	sprintf(buffer,"%f",a_float);
	WRF_wstring(a_output,buffer);
	}

void WRF_wint(WRF_output* a_output,long a_int)
	{
	char buffer[32];
	sprintf(buffer,"%ld",a_int);
	WRF_wstring(a_output,buffer);
	}
