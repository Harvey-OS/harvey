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
* Description:  Header file for instruction decoding logic.
*
****************************************************************************/

#ifndef __X86EMU_DECODE_H
#define __X86EMU_DECODE_H

/*---------------------- Macros and type definitions ----------------------*/

/* Instruction Decoding Stuff */

#define FETCH_DECODE_MODRM(mod,rh,rl) 	fetch_decode_modrm(&mod,&rh,&rl)
#define DECODE_RM_BYTE_REGISTER(r)    	decode_rm_byte_register(r)
#define DECODE_RM_WORD_REGISTER(r)    	decode_rm_word_register(r)
#define DECODE_RM_LONG_REGISTER(r)    	decode_rm_long_register(r)
#define DECODE_CLEAR_SEGOVR()         	M.x86.mode &= ~SYSMODE_CLRMASK

/*-------------------------- Function Prototypes --------------------------*/

#ifdef  __cplusplus
extern "C" {            			/* Use "C" linkage when in C++ mode */
#endif

void 	x86emu_intr_raise (uint8_t type);
void    fetch_decode_modrm (int *mod,int *regh,int *regl);
uint8_t      fetch_byte_imm (void);
uint16_t     fetch_word_imm (void);
uint32_t     fetch_long_imm (void);
uint8_t      fetch_data_byte (uint offset);
uint8_t      fetch_data_byte_abs (uint segment, uint offset);
uint16_t     fetch_data_word (uint offset);
uint16_t     fetch_data_word_abs (uint segment, uint offset);
uint32_t     fetch_data_long (uint offset);
uint32_t     fetch_data_long_abs (uint segment, uint offset);
void    store_data_byte (uint offset, uint8_t val);
void    store_data_byte_abs (uint segment, uint offset, uint8_t val);
void    store_data_word (uint offset, uint16_t val);
void    store_data_word_abs (uint segment, uint offset, uint16_t val);
void    store_data_long (uint offset, uint32_t val);
void    store_data_long_abs (uint segment, uint offset, uint32_t val);
uint8_t* 	decode_rm_byte_register(int reg);
uint16_t* 	decode_rm_word_register(int reg);
uint32_t* 	decode_rm_long_register(int reg);
uint16_t* 	decode_rm_seg_register(int reg);
unsigned decode_rm00_address(int rm);
unsigned decode_rm01_address(int rm);
unsigned decode_rm10_address(int rm);
unsigned decode_rmXX_address(int mod, int rm);

#ifdef  __cplusplus
}                       			/* End of "C" linkage for C++   	*/
#endif

#endif /* __X86EMU_DECODE_H */
