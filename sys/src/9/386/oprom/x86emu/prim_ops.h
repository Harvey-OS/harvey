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
* Language:		ANSI C
* Environment:	Any
* Developer:    Kendall Bennett
*
* Description:  Header file for primitive operation functions.
*
****************************************************************************/

#ifndef __X86EMU_PRIM_OPS_H
#define __X86EMU_PRIM_OPS_H

#include "prim_asm.h"

#ifdef  __cplusplus
extern "C" {            			/* Use "C" linkage when in C++ mode */
#endif

uint16_t     aaa_word (uint16_t d);
uint16_t     aas_word (uint16_t d);
uint16_t     aad_word (uint16_t d);
uint16_t     aam_word (uint8_t d);
uint8_t      adc_byte (uint8_t d, uint8_t s);
uint16_t     adc_word (uint16_t d, uint16_t s);
uint32_t     adc_long (uint32_t d, uint32_t s);
uint8_t      add_byte (uint8_t d, uint8_t s);
uint16_t     add_word (uint16_t d, uint16_t s);
uint32_t     add_long (uint32_t d, uint32_t s);
uint8_t      and_byte (uint8_t d, uint8_t s);
uint16_t     and_word (uint16_t d, uint16_t s);
uint32_t     and_long (uint32_t d, uint32_t s);
uint8_t      cmp_byte (uint8_t d, uint8_t s);
uint16_t     cmp_word (uint16_t d, uint16_t s);
uint32_t     cmp_long (uint32_t d, uint32_t s);
uint8_t      daa_byte (uint8_t d);
uint8_t      das_byte (uint8_t d);
uint8_t      dec_byte (uint8_t d);
uint16_t     dec_word (uint16_t d);
uint32_t     dec_long (uint32_t d);
uint8_t      inc_byte (uint8_t d);
uint16_t     inc_word (uint16_t d);
uint32_t     inc_long (uint32_t d);
uint8_t      or_byte (uint8_t d, uint8_t s);
uint16_t     or_word (uint16_t d, uint16_t s);
uint32_t     or_long (uint32_t d, uint32_t s);
uint8_t      neg_byte (uint8_t s);
uint16_t     neg_word (uint16_t s);
uint32_t     neg_long (uint32_t s);
uint8_t      not_byte (uint8_t s);
uint16_t     not_word (uint16_t s);
uint32_t     not_long (uint32_t s);
uint8_t      rcl_byte (uint8_t d, uint8_t s);
uint16_t     rcl_word (uint16_t d, uint8_t s);
uint32_t     rcl_long (uint32_t d, uint8_t s);
uint8_t      rcr_byte (uint8_t d, uint8_t s);
uint16_t     rcr_word (uint16_t d, uint8_t s);
uint32_t     rcr_long (uint32_t d, uint8_t s);
uint8_t      rol_byte (uint8_t d, uint8_t s);
uint16_t     rol_word (uint16_t d, uint8_t s);
uint32_t     rol_long (uint32_t d, uint8_t s);
uint8_t      ror_byte (uint8_t d, uint8_t s);
uint16_t     ror_word (uint16_t d, uint8_t s);
uint32_t     ror_long (uint32_t d, uint8_t s);
uint8_t      shl_byte (uint8_t d, uint8_t s);
uint16_t     shl_word (uint16_t d, uint8_t s);
uint32_t     shl_long (uint32_t d, uint8_t s);
uint8_t      shr_byte (uint8_t d, uint8_t s);
uint16_t     shr_word (uint16_t d, uint8_t s);
uint32_t     shr_long (uint32_t d, uint8_t s);
uint8_t      sar_byte (uint8_t d, uint8_t s);
uint16_t     sar_word (uint16_t d, uint8_t s);
uint32_t     sar_long (uint32_t d, uint8_t s);
uint16_t     shld_word (uint16_t d, uint16_t fill, uint8_t s);
uint32_t     shld_long (uint32_t d, uint32_t fill, uint8_t s);
uint16_t     shrd_word (uint16_t d, uint16_t fill, uint8_t s);
uint32_t     shrd_long (uint32_t d, uint32_t fill, uint8_t s);
uint8_t      sbb_byte (uint8_t d, uint8_t s);
uint16_t     sbb_word (uint16_t d, uint16_t s);
uint32_t     sbb_long (uint32_t d, uint32_t s);
uint8_t      sub_byte (uint8_t d, uint8_t s);
uint16_t     sub_word (uint16_t d, uint16_t s);
uint32_t     sub_long (uint32_t d, uint32_t s);
void    test_byte (uint8_t d, uint8_t s);
void    test_word (uint16_t d, uint16_t s);
void    test_long (uint32_t d, uint32_t s);
uint8_t      xor_byte (uint8_t d, uint8_t s);
uint16_t     xor_word (uint16_t d, uint16_t s);
uint32_t     xor_long (uint32_t d, uint32_t s);
void    imul_byte (uint8_t s);
void    imul_word (uint16_t s);
void    imul_long (uint32_t s);
void 	imul_long_direct(uint32_t *res_lo, uint32_t* res_hi,uint32_t d, uint32_t s);
void    mul_byte (uint8_t s);
void    mul_word (uint16_t s);
void    mul_long (uint32_t s);
void    idiv_byte (uint8_t s);
void    idiv_word (uint16_t s);
void    idiv_long (uint32_t s);
void    div_byte (uint8_t s);
void    div_word (uint16_t s);
void    div_long (uint32_t s);
void    ins (int size);
void    outs (int size);
uint16_t     mem_access_word (int addr);
void    push_word (uint16_t w);
void    push_long (uint32_t w);
uint16_t     pop_word (void);
uint32_t	pop_long (void);
void	x86emu_cpuid (void);

#if  defined(__HAVE_INLINE_ASSEMBLER__) && !defined(PRIM_OPS_NO_REDEFINE_ASM)

#define	aaa_word(d)		aaa_word_asm(&M.x86.R_EFLG,d)
#define aas_word(d)		aas_word_asm(&M.x86.R_EFLG,d)
#define aad_word(d)		aad_word_asm(&M.x86.R_EFLG,d)
#define aam_word(d)		aam_word_asm(&M.x86.R_EFLG,d)
#define adc_byte(d,s)	adc_byte_asm(&M.x86.R_EFLG,d,s)
#define adc_word(d,s)	adc_word_asm(&M.x86.R_EFLG,d,s)
#define adc_long(d,s)	adc_long_asm(&M.x86.R_EFLG,d,s)
#define add_byte(d,s) 	add_byte_asm(&M.x86.R_EFLG,d,s)
#define add_word(d,s)	add_word_asm(&M.x86.R_EFLG,d,s)
#define add_long(d,s)	add_long_asm(&M.x86.R_EFLG,d,s)
#define and_byte(d,s)	and_byte_asm(&M.x86.R_EFLG,d,s)
#define and_word(d,s)	and_word_asm(&M.x86.R_EFLG,d,s)
#define and_long(d,s)	and_long_asm(&M.x86.R_EFLG,d,s)
#define cmp_byte(d,s)	cmp_byte_asm(&M.x86.R_EFLG,d,s)
#define cmp_word(d,s)	cmp_word_asm(&M.x86.R_EFLG,d,s)
#define cmp_long(d,s)	cmp_long_asm(&M.x86.R_EFLG,d,s)
#define daa_byte(d)		daa_byte_asm(&M.x86.R_EFLG,d)
#define das_byte(d)		das_byte_asm(&M.x86.R_EFLG,d)
#define dec_byte(d)		dec_byte_asm(&M.x86.R_EFLG,d)
#define dec_word(d)		dec_word_asm(&M.x86.R_EFLG,d)
#define dec_long(d)		dec_long_asm(&M.x86.R_EFLG,d)
#define inc_byte(d)		inc_byte_asm(&M.x86.R_EFLG,d)
#define inc_word(d)		inc_word_asm(&M.x86.R_EFLG,d)
#define inc_long(d)		inc_long_asm(&M.x86.R_EFLG,d)
#define or_byte(d,s)	or_byte_asm(&M.x86.R_EFLG,d,s)
#define or_word(d,s)	or_word_asm(&M.x86.R_EFLG,d,s)
#define or_long(d,s)	or_long_asm(&M.x86.R_EFLG,d,s)
#define neg_byte(s)		neg_byte_asm(&M.x86.R_EFLG,s)
#define neg_word(s)		neg_word_asm(&M.x86.R_EFLG,s)
#define neg_long(s)		neg_long_asm(&M.x86.R_EFLG,s)
#define not_byte(s)		not_byte_asm(&M.x86.R_EFLG,s)
#define not_word(s)		not_word_asm(&M.x86.R_EFLG,s)
#define not_long(s)		not_long_asm(&M.x86.R_EFLG,s)
#define rcl_byte(d,s)	rcl_byte_asm(&M.x86.R_EFLG,d,s)
#define rcl_word(d,s)	rcl_word_asm(&M.x86.R_EFLG,d,s)
#define rcl_long(d,s)	rcl_long_asm(&M.x86.R_EFLG,d,s)
#define rcr_byte(d,s)	rcr_byte_asm(&M.x86.R_EFLG,d,s)
#define rcr_word(d,s)	rcr_word_asm(&M.x86.R_EFLG,d,s)
#define rcr_long(d,s)	rcr_long_asm(&M.x86.R_EFLG,d,s)
#define rol_byte(d,s)	rol_byte_asm(&M.x86.R_EFLG,d,s)
#define rol_word(d,s)	rol_word_asm(&M.x86.R_EFLG,d,s)
#define rol_long(d,s)	rol_long_asm(&M.x86.R_EFLG,d,s)
#define ror_byte(d,s)	ror_byte_asm(&M.x86.R_EFLG,d,s)
#define ror_word(d,s)	ror_word_asm(&M.x86.R_EFLG,d,s)
#define ror_long(d,s)	ror_long_asm(&M.x86.R_EFLG,d,s)
#define shl_byte(d,s)	shl_byte_asm(&M.x86.R_EFLG,d,s)
#define shl_word(d,s)	shl_word_asm(&M.x86.R_EFLG,d,s)
#define shl_long(d,s)	shl_long_asm(&M.x86.R_EFLG,d,s)
#define shr_byte(d,s)	shr_byte_asm(&M.x86.R_EFLG,d,s)
#define shr_word(d,s)	shr_word_asm(&M.x86.R_EFLG,d,s)
#define shr_long(d,s)	shr_long_asm(&M.x86.R_EFLG,d,s)
#define sar_byte(d,s)	sar_byte_asm(&M.x86.R_EFLG,d,s)
#define sar_word(d,s)	sar_word_asm(&M.x86.R_EFLG,d,s)
#define sar_long(d,s)	sar_long_asm(&M.x86.R_EFLG,d,s)
#define shld_word(d,fill,s)	shld_word_asm(&M.x86.R_EFLG,d,fill,s)
#define shld_long(d,fill,s)	shld_long_asm(&M.x86.R_EFLG,d,fill,s)
#define shrd_word(d,fill,s)	shrd_word_asm(&M.x86.R_EFLG,d,fill,s)
#define shrd_long(d,fill,s)	shrd_long_asm(&M.x86.R_EFLG,d,fill,s)
#define sbb_byte(d,s)	sbb_byte_asm(&M.x86.R_EFLG,d,s)
#define sbb_word(d,s)	sbb_word_asm(&M.x86.R_EFLG,d,s)
#define sbb_long(d,s)	sbb_long_asm(&M.x86.R_EFLG,d,s)
#define sub_byte(d,s)	sub_byte_asm(&M.x86.R_EFLG,d,s)
#define sub_word(d,s)	sub_word_asm(&M.x86.R_EFLG,d,s)
#define sub_long(d,s)	sub_long_asm(&M.x86.R_EFLG,d,s)
#define test_byte(d,s)	test_byte_asm(&M.x86.R_EFLG,d,s)
#define test_word(d,s)	test_word_asm(&M.x86.R_EFLG,d,s)
#define test_long(d,s)	test_long_asm(&M.x86.R_EFLG,d,s)
#define xor_byte(d,s)	xor_byte_asm(&M.x86.R_EFLG,d,s)
#define xor_word(d,s)	xor_word_asm(&M.x86.R_EFLG,d,s)
#define xor_long(d,s)	xor_long_asm(&M.x86.R_EFLG,d,s)
#define imul_byte(s)	imul_byte_asm(&M.x86.R_EFLG,&M.x86.R_AX,M.x86.R_AL,s)
#define imul_word(s)	imul_word_asm(&M.x86.R_EFLG,&M.x86.R_AX,&M.x86.R_DX,M.x86.R_AX,s)
#define imul_long(s)	imul_long_asm(&M.x86.R_EFLG,&M.x86.R_EAX,&M.x86.R_EDX,M.x86.R_EAX,s)
#define imul_long_direct(res_lo,res_hi,d,s)	imul_long_asm(&M.x86.R_EFLG,res_lo,res_hi,d,s)
#define mul_byte(s)		mul_byte_asm(&M.x86.R_EFLG,&M.x86.R_AX,M.x86.R_AL,s)
#define mul_word(s)		mul_word_asm(&M.x86.R_EFLG,&M.x86.R_AX,&M.x86.R_DX,M.x86.R_AX,s)
#define mul_long(s)		mul_long_asm(&M.x86.R_EFLG,&M.x86.R_EAX,&M.x86.R_EDX,M.x86.R_EAX,s)
#define idiv_byte(s)	idiv_byte_asm(&M.x86.R_EFLG,&M.x86.R_AL,&M.x86.R_AH,M.x86.R_AX,s)
#define idiv_word(s)	idiv_word_asm(&M.x86.R_EFLG,&M.x86.R_AX,&M.x86.R_DX,M.x86.R_AX,M.x86.R_DX,s)
#define idiv_long(s)	idiv_long_asm(&M.x86.R_EFLG,&M.x86.R_EAX,&M.x86.R_EDX,M.x86.R_EAX,M.x86.R_EDX,s)
#define div_byte(s)		div_byte_asm(&M.x86.R_EFLG,&M.x86.R_AL,&M.x86.R_AH,M.x86.R_AX,s)
#define div_word(s)		div_word_asm(&M.x86.R_EFLG,&M.x86.R_AX,&M.x86.R_DX,M.x86.R_AX,M.x86.R_DX,s)
#define div_long(s)		div_long_asm(&M.x86.R_EFLG,&M.x86.R_EAX,&M.x86.R_EDX,M.x86.R_EAX,M.x86.R_EDX,s)

#endif

#ifdef  __cplusplus
}                       			/* End of "C" linkage for C++   	*/
#endif

#endif /* __X86EMU_PRIM_OPS_H */
