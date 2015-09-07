/****************************************************************************
*
*						Realmode X86 Emulator Library
*
*            	Copyright (C) 1996-1999 SciTech Software, Inc.
* 				     Copyright (C) David Mosberger-Tang
* 					   Copyright (C) 1999 Egbert Eich
*
*  ========================================================================
*
*  Permission to use, copy, modify, distribute, and sell this software and
*  its documentation for any purpose is hereby granted without fee,
*  provided that the above copyright notice appear in all copies and that
*  both that copyright notice and this permission notice appear in
*  supporting documentation, and that the name of the authors not be used
*  in advertising or publicity pertaining to distribution of the software
*  without specific, written prior permission.  The authors makes no
*  representations about the suitability of this software for any purpose.
*  It is provided "as is" without express or implied warranty.
*
*  THE AUTHORS DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
*  INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
*  EVENT SHALL THE AUTHORS BE LIABLE FOR ANY SPECIAL, INDIRECT OR
*  CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF
*  USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
*  OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
*  PERFORMANCE OF THIS SOFTWARE.
*
*  ========================================================================
*
* Language:		Watcom C++ 10.6 or later
* Environment:	Any
* Developer:    Kendall Bennett
*
* Description:  Inline assembler versions of the primitive operand
*				functions for faster performance. At the moment this is
*				x86 inline assembler, but these functions could be replaced
*				with native inline assembler for each supported processor
*				platform.
*
****************************************************************************/
/* $XFree86: xc/extras/x86emu/src/x86emu/x86emu/prim_asm.h,v 1.3 2000/04/19 15:48:15 tsi Exp $ */

#ifndef	__X86EMU_PRIM_ASM_H
#define	__X86EMU_PRIM_ASM_H

#ifdef	__WATCOMC__

#ifndef	VALIDATE
#define	__HAVE_INLINE_ASSEMBLER__
#endif

uint32_t		get_flags_asm(void);
#pragma aux get_flags_asm =			\
	"pushf"                         \
	"pop	eax"                  	\
	value [eax]                     \
	modify exact [eax];

uint16_t     aaa_word_asm(uint32_t *flags,uint16_t d);
#pragma aux aaa_word_asm =			\
	"push	[edi]"            		\
	"popf"                         	\
	"aaa"                  			\
	"pushf"                         \
	"pop	[edi]"            		\
	parm [edi] [ax] 				\
	value [ax]                      \
	modify exact [ax];

uint16_t     aas_word_asm(uint32_t *flags,uint16_t d);
#pragma aux aas_word_asm =			\
	"push	[edi]"            		\
	"popf"                         	\
	"aas"                  			\
	"pushf"                         \
	"pop	[edi]"            		\
	parm [edi] [ax] 				\
	value [ax]                      \
	modify exact [ax];

uint16_t     aad_word_asm(uint32_t *flags,uint16_t d);
#pragma aux aad_word_asm =			\
	"push	[edi]"            		\
	"popf"                         	\
	"aad"                  			\
	"pushf"                         \
	"pop	[edi]"            		\
	parm [edi] [ax] 				\
	value [ax]                      \
	modify exact [ax];

uint16_t     aam_word_asm(uint32_t *flags,uint8_t d);
#pragma aux aam_word_asm =			\
	"push	[edi]"            		\
	"popf"                         	\
	"aam"                  			\
	"pushf"                         \
	"pop	[edi]"            		\
	parm [edi] [al] 				\
	value [ax]                      \
	modify exact [ax];

uint8_t      adc_byte_asm(uint32_t *flags,uint8_t d, uint8_t s);
#pragma aux adc_byte_asm =			\
	"push	[edi]"            		\
	"popf"                         	\
	"adc	al,bl"                  \
	"pushf"                         \
	"pop	[edi]"            		\
	parm [edi] [al] [bl]            \
	value [al]                      \
	modify exact [al bl];

uint16_t     adc_word_asm(uint32_t *flags,uint16_t d, uint16_t s);
#pragma aux adc_word_asm =			\
	"push	[edi]"            		\
	"popf"                         	\
	"adc	ax,bx"                  \
	"pushf"                         \
	"pop	[edi]"            		\
	parm [edi] [ax] [bx]            \
	value [ax]                      \
	modify exact [ax bx];

uint32_t     adc_long_asm(uint32_t *flags,uint32_t d, uint32_t s);
#pragma aux adc_long_asm =			\
	"push	[edi]"            		\
	"popf"                         	\
	"adc	eax,ebx"                \
	"pushf"                         \
	"pop	[edi]"            		\
	parm [edi] [eax] [ebx]          \
	value [eax]                     \
	modify exact [eax ebx];

uint8_t      add_byte_asm(uint32_t *flags,uint8_t d, uint8_t s);
#pragma aux add_byte_asm =			\
	"push	[edi]"            		\
	"popf"                         	\
	"add	al,bl"                  \
	"pushf"                         \
	"pop	[edi]"            		\
	parm [edi] [al] [bl]            \
	value [al]                      \
	modify exact [al bl];

uint16_t     add_word_asm(uint32_t *flags,uint16_t d, uint16_t s);
#pragma aux add_word_asm =			\
	"push	[edi]"            		\
	"popf"                         	\
	"add	ax,bx"                  \
	"pushf"                         \
	"pop	[edi]"            		\
	parm [edi] [ax] [bx]            \
	value [ax]                      \
	modify exact [ax bx];

uint32_t     add_long_asm(uint32_t *flags,uint32_t d, uint32_t s);
#pragma aux add_long_asm =			\
	"push	[edi]"            		\
	"popf"                         	\
	"add	eax,ebx"                \
	"pushf"                         \
	"pop	[edi]"            		\
	parm [edi] [eax] [ebx]          \
	value [eax]                     \
	modify exact [eax ebx];

uint8_t      and_byte_asm(uint32_t *flags,uint8_t d, uint8_t s);
#pragma aux and_byte_asm =			\
	"push	[edi]"            		\
	"popf"                         	\
	"and	al,bl"                  \
	"pushf"                         \
	"pop	[edi]"            		\
	parm [edi] [al] [bl]            \
	value [al]                      \
	modify exact [al bl];

uint16_t     and_word_asm(uint32_t *flags,uint16_t d, uint16_t s);
#pragma aux and_word_asm =			\
	"push	[edi]"            		\
	"popf"                         	\
	"and	ax,bx"                  \
	"pushf"                         \
	"pop	[edi]"            		\
	parm [edi] [ax] [bx]            \
	value [ax]                      \
	modify exact [ax bx];

uint32_t     and_long_asm(uint32_t *flags,uint32_t d, uint32_t s);
#pragma aux and_long_asm =			\
	"push	[edi]"            		\
	"popf"                         	\
	"and	eax,ebx"                \
	"pushf"                         \
	"pop	[edi]"            		\
	parm [edi] [eax] [ebx]          \
	value [eax]                     \
	modify exact [eax ebx];

uint8_t      cmp_byte_asm(uint32_t *flags,uint8_t d, uint8_t s);
#pragma aux cmp_byte_asm =			\
	"push	[edi]"            		\
	"popf"                         	\
	"cmp	al,bl"                  \
	"pushf"                         \
	"pop	[edi]"            		\
	parm [edi] [al] [bl]            \
	value [al]                      \
	modify exact [al bl];

uint16_t     cmp_word_asm(uint32_t *flags,uint16_t d, uint16_t s);
#pragma aux cmp_word_asm =			\
	"push	[edi]"            		\
	"popf"                         	\
	"cmp	ax,bx"                  \
	"pushf"                         \
	"pop	[edi]"            		\
	parm [edi] [ax] [bx]            \
	value [ax]                      \
	modify exact [ax bx];

uint32_t     cmp_long_asm(uint32_t *flags,uint32_t d, uint32_t s);
#pragma aux cmp_long_asm =			\
	"push	[edi]"            		\
	"popf"                         	\
	"cmp	eax,ebx"                \
	"pushf"                         \
	"pop	[edi]"            		\
	parm [edi] [eax] [ebx]          \
	value [eax]                     \
	modify exact [eax ebx];

uint8_t      daa_byte_asm(uint32_t *flags,uint8_t d);
#pragma aux daa_byte_asm =			\
	"push	[edi]"            		\
	"popf"                         	\
	"daa"                  			\
	"pushf"                         \
	"pop	[edi]"            		\
	parm [edi] [al]            		\
	value [al]                      \
	modify exact [al];

uint8_t      das_byte_asm(uint32_t *flags,uint8_t d);
#pragma aux das_byte_asm =			\
	"push	[edi]"            		\
	"popf"                         	\
	"das"                  			\
	"pushf"                         \
	"pop	[edi]"            		\
	parm [edi] [al]            		\
	value [al]                      \
	modify exact [al];

uint8_t      dec_byte_asm(uint32_t *flags,uint8_t d);
#pragma aux dec_byte_asm =			\
	"push	[edi]"            		\
	"popf"                         	\
	"dec	al"                  	\
	"pushf"                         \
	"pop	[edi]"            		\
	parm [edi] [al]            		\
	value [al]                      \
	modify exact [al];

uint16_t     dec_word_asm(uint32_t *flags,uint16_t d);
#pragma aux dec_word_asm =			\
	"push	[edi]"            		\
	"popf"                         	\
	"dec	ax"                  	\
	"pushf"                         \
	"pop	[edi]"            		\
	parm [edi] [ax]            		\
	value [ax]                      \
	modify exact [ax];

uint32_t     dec_long_asm(uint32_t *flags,uint32_t d);
#pragma aux dec_long_asm =			\
	"push	[edi]"            		\
	"popf"                         	\
	"dec	eax"                	\
	"pushf"                         \
	"pop	[edi]"            		\
	parm [edi] [eax]          		\
	value [eax]                     \
	modify exact [eax];

uint8_t      inc_byte_asm(uint32_t *flags,uint8_t d);
#pragma aux inc_byte_asm =			\
	"push	[edi]"            		\
	"popf"                         	\
	"inc	al"                  	\
	"pushf"                         \
	"pop	[edi]"            		\
	parm [edi] [al]            		\
	value [al]                      \
	modify exact [al];

uint16_t     inc_word_asm(uint32_t *flags,uint16_t d);
#pragma aux inc_word_asm =			\
	"push	[edi]"            		\
	"popf"                         	\
	"inc	ax"                  	\
	"pushf"                         \
	"pop	[edi]"            		\
	parm [edi] [ax]            		\
	value [ax]                      \
	modify exact [ax];

uint32_t     inc_long_asm(uint32_t *flags,uint32_t d);
#pragma aux inc_long_asm =			\
	"push	[edi]"            		\
	"popf"                         	\
	"inc	eax"                	\
	"pushf"                         \
	"pop	[edi]"            		\
	parm [edi] [eax]          		\
	value [eax]                     \
	modify exact [eax];

uint8_t      or_byte_asm(uint32_t *flags,uint8_t d, uint8_t s);
#pragma aux or_byte_asm =			\
	"push	[edi]"            		\
	"popf"                         	\
	"or	al,bl"                  	\
	"pushf"                         \
	"pop	[edi]"            		\
	parm [edi] [al] [bl]            \
	value [al]                      \
	modify exact [al bl];

uint16_t     or_word_asm(uint32_t *flags,uint16_t d, uint16_t s);
#pragma aux or_word_asm =			\
	"push	[edi]"            		\
	"popf"                         	\
	"or	ax,bx"                  	\
	"pushf"                         \
	"pop	[edi]"            		\
	parm [edi] [ax] [bx]            \
	value [ax]                      \
	modify exact [ax bx];

uint32_t     or_long_asm(uint32_t *flags,uint32_t d, uint32_t s);
#pragma aux or_long_asm =			\
	"push	[edi]"            		\
	"popf"                         	\
	"or	eax,ebx"                	\
	"pushf"                         \
	"pop	[edi]"            		\
	parm [edi] [eax] [ebx]          \
	value [eax]                     \
	modify exact [eax ebx];

uint8_t      neg_byte_asm(uint32_t *flags,uint8_t d);
#pragma aux neg_byte_asm =			\
	"push	[edi]"            		\
	"popf"                         	\
	"neg	al"                  	\
	"pushf"                         \
	"pop	[edi]"            		\
	parm [edi] [al]            		\
	value [al]                      \
	modify exact [al];

uint16_t     neg_word_asm(uint32_t *flags,uint16_t d);
#pragma aux neg_word_asm =			\
	"push	[edi]"            		\
	"popf"                         	\
	"neg	ax"                  	\
	"pushf"                         \
	"pop	[edi]"            		\
	parm [edi] [ax]            		\
	value [ax]                      \
	modify exact [ax];

uint32_t     neg_long_asm(uint32_t *flags,uint32_t d);
#pragma aux neg_long_asm =			\
	"push	[edi]"            		\
	"popf"                         	\
	"neg	eax"                	\
	"pushf"                         \
	"pop	[edi]"            		\
	parm [edi] [eax]          		\
	value [eax]                     \
	modify exact [eax];

uint8_t      not_byte_asm(uint32_t *flags,uint8_t d);
#pragma aux not_byte_asm =			\
	"push	[edi]"            		\
	"popf"                         	\
	"not	al"                  	\
	"pushf"                         \
	"pop	[edi]"            		\
	parm [edi] [al]            		\
	value [al]                      \
	modify exact [al];

uint16_t     not_word_asm(uint32_t *flags,uint16_t d);
#pragma aux not_word_asm =			\
	"push	[edi]"            		\
	"popf"                         	\
	"not	ax"                  	\
	"pushf"                         \
	"pop	[edi]"            		\
	parm [edi] [ax]            		\
	value [ax]                      \
	modify exact [ax];

uint32_t     not_long_asm(uint32_t *flags,uint32_t d);
#pragma aux not_long_asm =			\
	"push	[edi]"            		\
	"popf"                         	\
	"not	eax"                	\
	"pushf"                         \
	"pop	[edi]"            		\
	parm [edi] [eax]          		\
	value [eax]                     \
	modify exact [eax];

uint8_t      rcl_byte_asm(uint32_t *flags,uint8_t d, uint8_t s);
#pragma aux rcl_byte_asm =			\
	"push	[edi]"            		\
	"popf"                         	\
	"rcl	al,cl"                  \
	"pushf"                         \
	"pop	[edi]"            		\
	parm [edi] [al] [cl]            \
	value [al]                      \
	modify exact [al cl];

uint16_t     rcl_word_asm(uint32_t *flags,uint16_t d, uint8_t s);
#pragma aux rcl_word_asm =			\
	"push	[edi]"            		\
	"popf"                         	\
	"rcl	ax,cl"                  \
	"pushf"                         \
	"pop	[edi]"            		\
	parm [edi] [ax] [cl]            \
	value [ax]                      \
	modify exact [ax cl];

uint32_t     rcl_long_asm(uint32_t *flags,uint32_t d, uint8_t s);
#pragma aux rcl_long_asm =			\
	"push	[edi]"            		\
	"popf"                         	\
	"rcl	eax,cl"                	\
	"pushf"                         \
	"pop	[edi]"            		\
	parm [edi] [eax] [cl]          	\
	value [eax]                     \
	modify exact [eax cl];

uint8_t      rcr_byte_asm(uint32_t *flags,uint8_t d, uint8_t s);
#pragma aux rcr_byte_asm =			\
	"push	[edi]"            		\
	"popf"                         	\
	"rcr	al,cl"                  \
	"pushf"                         \
	"pop	[edi]"            		\
	parm [edi] [al] [cl]            \
	value [al]                      \
	modify exact [al cl];

uint16_t     rcr_word_asm(uint32_t *flags,uint16_t d, uint8_t s);
#pragma aux rcr_word_asm =			\
	"push	[edi]"            		\
	"popf"                         	\
	"rcr	ax,cl"                  \
	"pushf"                         \
	"pop	[edi]"            		\
	parm [edi] [ax] [cl]            \
	value [ax]                      \
	modify exact [ax cl];

uint32_t     rcr_long_asm(uint32_t *flags,uint32_t d, uint8_t s);
#pragma aux rcr_long_asm =			\
	"push	[edi]"            		\
	"popf"                         	\
	"rcr	eax,cl"                	\
	"pushf"                         \
	"pop	[edi]"            		\
	parm [edi] [eax] [cl]          	\
	value [eax]                     \
	modify exact [eax cl];

uint8_t      rol_byte_asm(uint32_t *flags,uint8_t d, uint8_t s);
#pragma aux rol_byte_asm =			\
	"push	[edi]"            		\
	"popf"                         	\
	"rol	al,cl"                  \
	"pushf"                         \
	"pop	[edi]"            		\
	parm [edi] [al] [cl]            \
	value [al]                      \
	modify exact [al cl];

uint16_t     rol_word_asm(uint32_t *flags,uint16_t d, uint8_t s);
#pragma aux rol_word_asm =			\
	"push	[edi]"            		\
	"popf"                         	\
	"rol	ax,cl"                  \
	"pushf"                         \
	"pop	[edi]"            		\
	parm [edi] [ax] [cl]            \
	value [ax]                      \
	modify exact [ax cl];

uint32_t     rol_long_asm(uint32_t *flags,uint32_t d, uint8_t s);
#pragma aux rol_long_asm =			\
	"push	[edi]"            		\
	"popf"                         	\
	"rol	eax,cl"                	\
	"pushf"                         \
	"pop	[edi]"            		\
	parm [edi] [eax] [cl]          	\
	value [eax]                     \
	modify exact [eax cl];

uint8_t      ror_byte_asm(uint32_t *flags,uint8_t d, uint8_t s);
#pragma aux ror_byte_asm =			\
	"push	[edi]"            		\
	"popf"                         	\
	"ror	al,cl"                  \
	"pushf"                         \
	"pop	[edi]"            		\
	parm [edi] [al] [cl]            \
	value [al]                      \
	modify exact [al cl];

uint16_t     ror_word_asm(uint32_t *flags,uint16_t d, uint8_t s);
#pragma aux ror_word_asm =			\
	"push	[edi]"            		\
	"popf"                         	\
	"ror	ax,cl"                  \
	"pushf"                         \
	"pop	[edi]"            		\
	parm [edi] [ax] [cl]            \
	value [ax]                      \
	modify exact [ax cl];

uint32_t     ror_long_asm(uint32_t *flags,uint32_t d, uint8_t s);
#pragma aux ror_long_asm =			\
	"push	[edi]"            		\
	"popf"                         	\
	"ror	eax,cl"                	\
	"pushf"                         \
	"pop	[edi]"            		\
	parm [edi] [eax] [cl]          	\
	value [eax]                     \
	modify exact [eax cl];

uint8_t      shl_byte_asm(uint32_t *flags,uint8_t d, uint8_t s);
#pragma aux shl_byte_asm =			\
	"push	[edi]"            		\
	"popf"                         	\
	"shl	al,cl"                  \
	"pushf"                         \
	"pop	[edi]"            		\
	parm [edi] [al] [cl]            \
	value [al]                      \
	modify exact [al cl];

uint16_t     shl_word_asm(uint32_t *flags,uint16_t d, uint8_t s);
#pragma aux shl_word_asm =			\
	"push	[edi]"            		\
	"popf"                         	\
	"shl	ax,cl"                  \
	"pushf"                         \
	"pop	[edi]"            		\
	parm [edi] [ax] [cl]            \
	value [ax]                      \
	modify exact [ax cl];

uint32_t     shl_long_asm(uint32_t *flags,uint32_t d, uint8_t s);
#pragma aux shl_long_asm =			\
	"push	[edi]"            		\
	"popf"                         	\
	"shl	eax,cl"                	\
	"pushf"                         \
	"pop	[edi]"            		\
	parm [edi] [eax] [cl]          	\
	value [eax]                     \
	modify exact [eax cl];

uint8_t      shr_byte_asm(uint32_t *flags,uint8_t d, uint8_t s);
#pragma aux shr_byte_asm =			\
	"push	[edi]"            		\
	"popf"                         	\
	"shr	al,cl"                  \
	"pushf"                         \
	"pop	[edi]"            		\
	parm [edi] [al] [cl]            \
	value [al]                      \
	modify exact [al cl];

uint16_t     shr_word_asm(uint32_t *flags,uint16_t d, uint8_t s);
#pragma aux shr_word_asm =			\
	"push	[edi]"            		\
	"popf"                         	\
	"shr	ax,cl"                  \
	"pushf"                         \
	"pop	[edi]"            		\
	parm [edi] [ax] [cl]            \
	value [ax]                      \
	modify exact [ax cl];

uint32_t     shr_long_asm(uint32_t *flags,uint32_t d, uint8_t s);
#pragma aux shr_long_asm =			\
	"push	[edi]"            		\
	"popf"                         	\
	"shr	eax,cl"                	\
	"pushf"                         \
	"pop	[edi]"            		\
	parm [edi] [eax] [cl]          	\
	value [eax]                     \
	modify exact [eax cl];

uint8_t      sar_byte_asm(uint32_t *flags,uint8_t d, uint8_t s);
#pragma aux sar_byte_asm =			\
	"push	[edi]"            		\
	"popf"                         	\
	"sar	al,cl"                  \
	"pushf"                         \
	"pop	[edi]"            		\
	parm [edi] [al] [cl]            \
	value [al]                      \
	modify exact [al cl];

uint16_t     sar_word_asm(uint32_t *flags,uint16_t d, uint8_t s);
#pragma aux sar_word_asm =			\
	"push	[edi]"            		\
	"popf"                         	\
	"sar	ax,cl"                  \
	"pushf"                         \
	"pop	[edi]"            		\
	parm [edi] [ax] [cl]            \
	value [ax]                      \
	modify exact [ax cl];

uint32_t     sar_long_asm(uint32_t *flags,uint32_t d, uint8_t s);
#pragma aux sar_long_asm =			\
	"push	[edi]"            		\
	"popf"                         	\
	"sar	eax,cl"                	\
	"pushf"                         \
	"pop	[edi]"            		\
	parm [edi] [eax] [cl]          	\
	value [eax]                     \
	modify exact [eax cl];

uint16_t		shld_word_asm(uint32_t *flags,uint16_t d, uint16_t fill, uint8_t s);
#pragma aux shld_word_asm =			\
	"push	[edi]"            		\
	"popf"                         	\
	"shld	ax,dx,cl"               \
	"pushf"                         \
	"pop	[edi]"            		\
	parm [edi] [ax] [dx] [cl]       \
	value [ax]                      \
	modify exact [ax dx cl];

uint32_t     shld_long_asm(uint32_t *flags,uint32_t d, uint32_t fill, uint8_t s);
#pragma aux shld_long_asm =			\
	"push	[edi]"            		\
	"popf"                         	\
	"shld	eax,edx,cl"             \
	"pushf"                         \
	"pop	[edi]"            		\
	parm [edi] [eax] [edx] [cl]     \
	value [eax]                     \
	modify exact [eax edx cl];

uint16_t		shrd_word_asm(uint32_t *flags,uint16_t d, uint16_t fill, uint8_t s);
#pragma aux shrd_word_asm =			\
	"push	[edi]"            		\
	"popf"                         	\
	"shrd	ax,dx,cl"               \
	"pushf"                         \
	"pop	[edi]"            		\
	parm [edi] [ax] [dx] [cl]       \
	value [ax]                      \
	modify exact [ax dx cl];

uint32_t     shrd_long_asm(uint32_t *flags,uint32_t d, uint32_t fill, uint8_t s);
#pragma aux shrd_long_asm =			\
	"push	[edi]"            		\
	"popf"                         	\
	"shrd	eax,edx,cl"             \
	"pushf"                         \
	"pop	[edi]"            		\
	parm [edi] [eax] [edx] [cl]     \
	value [eax]                     \
	modify exact [eax edx cl];

uint8_t      sbb_byte_asm(uint32_t *flags,uint8_t d, uint8_t s);
#pragma aux sbb_byte_asm =			\
	"push	[edi]"            		\
	"popf"                         	\
	"sbb	al,bl"                  \
	"pushf"                         \
	"pop	[edi]"            		\
	parm [edi] [al] [bl]            \
	value [al]                      \
	modify exact [al bl];

uint16_t     sbb_word_asm(uint32_t *flags,uint16_t d, uint16_t s);
#pragma aux sbb_word_asm =			\
	"push	[edi]"            		\
	"popf"                         	\
	"sbb	ax,bx"                  \
	"pushf"                         \
	"pop	[edi]"            		\
	parm [edi] [ax] [bx]            \
	value [ax]                      \
	modify exact [ax bx];

uint32_t     sbb_long_asm(uint32_t *flags,uint32_t d, uint32_t s);
#pragma aux sbb_long_asm =			\
	"push	[edi]"            		\
	"popf"                         	\
	"sbb	eax,ebx"                \
	"pushf"                         \
	"pop	[edi]"            		\
	parm [edi] [eax] [ebx]          \
	value [eax]                     \
	modify exact [eax ebx];

uint8_t      sub_byte_asm(uint32_t *flags,uint8_t d, uint8_t s);
#pragma aux sub_byte_asm =			\
	"push	[edi]"            		\
	"popf"                         	\
	"sub	al,bl"                  \
	"pushf"                         \
	"pop	[edi]"            		\
	parm [edi] [al] [bl]            \
	value [al]                      \
	modify exact [al bl];

uint16_t     sub_word_asm(uint32_t *flags,uint16_t d, uint16_t s);
#pragma aux sub_word_asm =			\
	"push	[edi]"            		\
	"popf"                         	\
	"sub	ax,bx"                  \
	"pushf"                         \
	"pop	[edi]"            		\
	parm [edi] [ax] [bx]            \
	value [ax]                      \
	modify exact [ax bx];

uint32_t     sub_long_asm(uint32_t *flags,uint32_t d, uint32_t s);
#pragma aux sub_long_asm =			\
	"push	[edi]"            		\
	"popf"                         	\
	"sub	eax,ebx"                \
	"pushf"                         \
	"pop	[edi]"            		\
	parm [edi] [eax] [ebx]          \
	value [eax]                     \
	modify exact [eax ebx];

void	test_byte_asm(uint32_t *flags,uint8_t d, uint8_t s);
#pragma aux test_byte_asm =			\
	"push	[edi]"            		\
	"popf"                         	\
	"test	al,bl"                  \
	"pushf"                         \
	"pop	[edi]"            		\
	parm [edi] [al] [bl]            \
	modify exact [al bl];

void	test_word_asm(uint32_t *flags,uint16_t d, uint16_t s);
#pragma aux test_word_asm =			\
	"push	[edi]"            		\
	"popf"                         	\
	"test	ax,bx"                  \
	"pushf"                         \
	"pop	[edi]"            		\
	parm [edi] [ax] [bx]            \
	modify exact [ax bx];

void	test_long_asm(uint32_t *flags,uint32_t d, uint32_t s);
#pragma aux test_long_asm =			\
	"push	[edi]"            		\
	"popf"                         	\
	"test	eax,ebx"                \
	"pushf"                         \
	"pop	[edi]"            		\
	parm [edi] [eax] [ebx]          \
	modify exact [eax ebx];

uint8_t      xor_byte_asm(uint32_t *flags,uint8_t d, uint8_t s);
#pragma aux xor_byte_asm =			\
	"push	[edi]"            		\
	"popf"                         	\
	"xor	al,bl"                  \
	"pushf"                         \
	"pop	[edi]"            		\
	parm [edi] [al] [bl]            \
	value [al]                      \
	modify exact [al bl];

uint16_t     xor_word_asm(uint32_t *flags,uint16_t d, uint16_t s);
#pragma aux xor_word_asm =			\
	"push	[edi]"            		\
	"popf"                         	\
	"xor	ax,bx"                  \
	"pushf"                         \
	"pop	[edi]"            		\
	parm [edi] [ax] [bx]            \
	value [ax]                      \
	modify exact [ax bx];

uint32_t     xor_long_asm(uint32_t *flags,uint32_t d, uint32_t s);
#pragma aux xor_long_asm =			\
	"push	[edi]"            		\
	"popf"                         	\
	"xor	eax,ebx"                \
	"pushf"                         \
	"pop	[edi]"            		\
	parm [edi] [eax] [ebx]          \
	value [eax]                     \
	modify exact [eax ebx];

void    imul_byte_asm(uint32_t *flags,uint16_t *ax,uint8_t d,uint8_t s);
#pragma aux imul_byte_asm =			\
	"push	[edi]"            		\
	"popf"                         	\
	"imul	bl"                  	\
	"pushf"                         \
	"pop	[edi]"            		\
	"mov	[esi],ax"				\
	parm [edi] [esi] [al] [bl]      \
	modify exact [esi ax bl];

void    imul_word_asm(uint32_t *flags,uint16_t *ax,uint16_t *dx,uint16_t d,uint16_t s);
#pragma aux imul_word_asm =			\
	"push	[edi]"            		\
	"popf"                         	\
	"imul	bx"                  	\
	"pushf"                         \
	"pop	[edi]"            		\
	"mov	[esi],ax"				\
	"mov	[ecx],dx"				\
	parm [edi] [esi] [ecx] [ax] [bx]\
	modify exact [esi edi ax bx dx];

void    imul_long_asm(uint32_t *flags,uint32_t *eax,uint32_t *edx,uint32_t d,uint32_t s);
#pragma aux imul_long_asm =			\
	"push	[edi]"            		\
	"popf"                         	\
	"imul	ebx"                  	\
	"pushf"                         \
	"pop	[edi]"            		\
	"mov	[esi],eax"				\
	"mov	[ecx],edx"				\
	parm [edi] [esi] [ecx] [eax] [ebx] \
	modify exact [esi edi eax ebx edx];

void    mul_byte_asm(uint32_t *flags,uint16_t *ax,uint8_t d,uint8_t s);
#pragma aux mul_byte_asm =			\
	"push	[edi]"            		\
	"popf"                         	\
	"mul	bl"                  	\
	"pushf"                         \
	"pop	[edi]"            		\
	"mov	[esi],ax"				\
	parm [edi] [esi] [al] [bl]      \
	modify exact [esi ax bl];

void    mul_word_asm(uint32_t *flags,uint16_t *ax,uint16_t *dx,uint16_t d,uint16_t s);
#pragma aux mul_word_asm =			\
	"push	[edi]"            		\
	"popf"                         	\
	"mul	bx"                  	\
	"pushf"                         \
	"pop	[edi]"            		\
	"mov	[esi],ax"				\
	"mov	[ecx],dx"				\
	parm [edi] [esi] [ecx] [ax] [bx]\
	modify exact [esi edi ax bx dx];

void    mul_long_asm(uint32_t *flags,uint32_t *eax,uint32_t *edx,uint32_t d,uint32_t s);
#pragma aux mul_long_asm =			\
	"push	[edi]"            		\
	"popf"                         	\
	"mul	ebx"                  	\
	"pushf"                         \
	"pop	[edi]"            		\
	"mov	[esi],eax"				\
	"mov	[ecx],edx"				\
	parm [edi] [esi] [ecx] [eax] [ebx] \
	modify exact [esi edi eax ebx edx];

void	idiv_byte_asm(uint32_t *flags,uint8_t *al,uint8_t *ah,uint16_t d,uint8_t s);
#pragma aux idiv_byte_asm =			\
	"push	[edi]"            		\
	"popf"                         	\
	"idiv	bl"                  	\
	"pushf"                         \
	"pop	[edi]"            		\
	"mov	[esi],al"				\
	"mov	[ecx],ah"				\
	parm [edi] [esi] [ecx] [ax] [bl]\
	modify exact [esi edi ax bl];

void	idiv_word_asm(uint32_t *flags,uint16_t *ax,uint16_t *dx,uint16_t dlo,uint16_t dhi,uint16_t s);
#pragma aux idiv_word_asm =			\
	"push	[edi]"            		\
	"popf"                         	\
	"idiv	bx"                  	\
	"pushf"                         \
	"pop	[edi]"            		\
	"mov	[esi],ax"				\
	"mov	[ecx],dx"				\
	parm [edi] [esi] [ecx] [ax] [dx] [bx]\
	modify exact [esi edi ax dx bx];

void	idiv_long_asm(uint32_t *flags,uint32_t *eax,uint32_t *edx,uint32_t dlo,uint32_t dhi,uint32_t s);
#pragma aux idiv_long_asm =			\
	"push	[edi]"            		\
	"popf"                         	\
	"idiv	ebx"                  	\
	"pushf"                         \
	"pop	[edi]"            		\
	"mov	[esi],eax"				\
	"mov	[ecx],edx"				\
	parm [edi] [esi] [ecx] [eax] [edx] [ebx]\
	modify exact [esi edi eax edx ebx];

void	div_byte_asm(uint32_t *flags,uint8_t *al,uint8_t *ah,uint16_t d,uint8_t s);
#pragma aux div_byte_asm =			\
	"push	[edi]"            		\
	"popf"                         	\
	"div	bl"                  	\
	"pushf"                         \
	"pop	[edi]"            		\
	"mov	[esi],al"				\
	"mov	[ecx],ah"				\
	parm [edi] [esi] [ecx] [ax] [bl]\
	modify exact [esi edi ax bl];

void	div_word_asm(uint32_t *flags,uint16_t *ax,uint16_t *dx,uint16_t dlo,uint16_t dhi,uint16_t s);
#pragma aux div_word_asm =			\
	"push	[edi]"            		\
	"popf"                         	\
	"div	bx"                  	\
	"pushf"                         \
	"pop	[edi]"            		\
	"mov	[esi],ax"				\
	"mov	[ecx],dx"				\
	parm [edi] [esi] [ecx] [ax] [dx] [bx]\
	modify exact [esi edi ax dx bx];

void	div_long_asm(uint32_t *flags,uint32_t *eax,uint32_t *edx,uint32_t dlo,uint32_t dhi,uint32_t s);
#pragma aux div_long_asm =			\
	"push	[edi]"            		\
	"popf"                         	\
	"div	ebx"                  	\
	"pushf"                         \
	"pop	[edi]"            		\
	"mov	[esi],eax"				\
	"mov	[ecx],edx"				\
	parm [edi] [esi] [ecx] [eax] [edx] [ebx]\
	modify exact [esi edi eax edx ebx];

#endif

#endif /* __X86EMU_PRIM_ASM_H */
