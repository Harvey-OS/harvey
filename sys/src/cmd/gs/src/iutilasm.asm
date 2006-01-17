;    Copyright (C) 1989, 1992, 1993 Aladdin Enterprises.  All rights reserved.
; 
; This software is provided AS-IS with no warranty, either express or
; implied.
; 
; This software is distributed under license and may not be copied,
; modified or distributed except as expressly authorized under the terms
; of the license contained in the file LICENSE in this distribution.
; 
; For more information about licensing, please refer to
; http://www.ghostscript.com/licensing/. For information on
; commercial licensing, go to http://www.artifex.com/licensing/ or
; contact Artifex Software, Inc., 101 Lucas Valley Road #110,
; San Rafael, CA  94903, U.S.A., +1(415)492-9861.

; $Id: iutilasm.asm,v 1.4 2002/02/21 22:24:53 giles Exp $
; iutilasm.asm
; Assembly code for Ghostscript interpreter on MS-DOS systems

	ifdef	FOR80386

	.286c

	endif

utilasm_TEXT	SEGMENT	WORD PUBLIC 'CODE'
	ASSUME	CS:utilasm_TEXT


	ifdef	FOR80386

; Macro for 32-bit operand prefix.
OP32	macro
	db	66h
	endm

	endif					; FOR80386

; Clear a register

clear	macro	reg
	xor	reg,reg
	endm


	ifdef	FOR80386

; Replace the multiply and divide routines in the Turbo C library
; if we are running on an 80386.

; Macro to swap the halves of a 32-bit register.
; Unfortunately, masm won't allow a shift instruction with a count of 16,
; so we have to code it in hex.
swap	macro	regno
	  OP32
	db	0c1h,0c0h+regno,16		; rol regno,16
	endm
regax	equ	0
regcx	equ	1
regdx	equ	2
regbx	equ	3


; Multiply (dx,ax) by (cx,bx) to (dx,ax).
	PUBLIC	LXMUL@
	PUBLIC	F_LXMUL@
F_LXMUL@ proc	far
LXMUL@	proc	far
	swap	regdx
	mov	dx,ax
	swap	regcx
	mov	cx,bx
	  OP32
	db	0fh,0afh,0d1h			; imul dx,cx
	  OP32
	mov	ax,dx
	swap	regdx
	ret
LXMUL@	endp
F_LXMUL@ endp


; Divide two stack operands, leave the result in (dx,ax).

	ifdef	DEBUG

setup32	macro
	mov	bx,sp
	push	bp
	mov	bp,sp
	  OP32
	mov	ax,ss:[bx+4]			; dividend
	endm

ret32	macro	n
	mov	sp,bp
	pop	bp
	ret	n
	endm

	else					; !DEBUG

setup32	macro
	mov	bx,sp
	  OP32
	mov	ax,ss:[bx+4]			; dividend
	endm

ret32	macro	n
	ret	n
	endm

	endif					; (!)DEBUG

	PUBLIC	LDIV@, LUDIV@, LMOD@, LUMOD@
	PUBLIC	F_LDIV@, F_LUDIV@, F_LMOD@, F_LUMOD@
F_LDIV@	proc	far
LDIV@	proc	far
	setup32
	  OP32
	cwd
	  OP32
	idiv	word ptr ss:[bx+8]		; divisor
	  OP32
	mov	dx,ax
	swap	regdx
	ret32	8
LDIV@	endp
F_LDIV@	endp
F_LUDIV@ proc	far
LUDIV@	proc	far
	setup32
	  OP32
	xor	dx,dx
	  OP32
	div	word ptr ss:[bx+8]		; divisor
	  OP32
	mov	dx,ax
	swap	regdx
	ret32	8
LUDIV@	endp
F_LUDIV@ endp
F_LMOD@	proc	far
LMOD@	proc	far
	setup32
	  OP32
	cwd
	  OP32
	idiv	word ptr ss:[bx+8]		; divisor
	  OP32
	mov	ax,dx
	swap	regdx
	ret32	8
LMOD@	endp
F_LMOD@	endp
F_LUMOD@ proc	far
LUMOD@	proc	far
	setup32
	  OP32
	xor	dx,dx
	  OP32
	div	word ptr ss:[bx+8]		; divisor
	  OP32
	mov	ax,dx
	swap	regdx
	ret32	8
LUMOD@	endp
F_LUMOD@ endp

	else					; !FOR80386

; Replace the divide routines in the Turbo C library,
; which do the division one bit at a time (!).

	PUBLIC	LDIV@, LMOD@, LUDIV@, LUMOD@
	PUBLIC	F_LDIV@, F_LMOD@, F_LUDIV@, F_LUMOD@

; Negate a long on the stack.
negbp	macro	offset
	neg	word ptr [bp+offset+2]		; high part
	neg	word ptr [bp+offset]		; low part
	sbb	word ptr [bp+offset+2],0
	endm

; Negate a long in (dx,ax).
negr	macro
	neg	dx
	neg	ax
	sbb	dx,0
	endm

; Divide two unsigned longs on the stack.
; Leave either the quotient or the remainder in (dx,ax).
; Operand offsets assume that bp (and only bp) has been pushed.
nlo	equ	6
nhi	equ	8
dlo	equ	10
dhi	equ	12

; We use an offset in bx to distinguish div from mod,
; and to indicate whether the result should be negated.
odiv	equ	0
omod	equ	2
odivneg	equ	4
omodneg	equ	6
F_LMOD@	proc	far
LMOD@	proc	far
	push	bp
	mov	bp,sp
	mov	bx,omod
			; Take abs of denominator
	cmp	byte ptr [bp+dhi+1],bh		; bh = 0
	jge	modpd
	negbp	dlo
modpd:			; Negate mod if numerator < 0
	cmp	byte ptr [bp+nhi+1],bh		; bh = 0
	jge	udiv
	mov	bx,omodneg
negnum:	negbp	nlo
	jmp	udiv
LMOD@	endp
F_LMOD@	endp
F_LUMOD@ proc	far
LUMOD@	proc	far
	mov	bx,omod
	jmp	udpush
LUMOD@	endp
F_LUMOD@ endp
F_LDIV@	proc	far
LDIV@	proc	far
	push	bp
	mov	bp,sp
	mov	bx,odiv
			; Negate quo if num^den < 0
	mov	ax,[bp+nhi]
	xor	ax,[bp+dhi]
	jge	divabs
	mov	bx,odivneg
divabs:			; Take abs of denominator
	cmp	byte ptr [bp+dhi+1],bh		; bh = 0
	jge	divpd
	negbp	dlo
divpd:			; Take abs of numerator
	cmp	byte ptr [bp+nhi+1],bh		; bh = 0
	jge	udiv
	jmp	negnum
LDIV@	endp
F_LDIV@	endp
F_LUDIV@ proc	far
LUDIV@	proc	far
	mov	bx,odiv
udpush:	push	bp
	mov	bp,sp
udiv:	push	bx				; odiv, omod, odivneg, omodneg
	mov	ax,[bp+nlo]
	mov	dx,[bp+nhi]
	mov	bx,[bp+dlo]
	mov	cx,[bp+dhi]
; Now we are dividing dx:ax by cx:bx.
; Check to see whether this is really a 32/16 division.
	or	cx,cx
	jnz	div2
; 32/16, check for 16- vs. 32-bit quotient
	cmp	dx,bx
	jae	div1
; 32/16 with 16-bit quotient, just do it.
	div	bx				; ax = quo, dx = rem
	pop	bx
	pop	bp
	jmp	cs:xx1[bx]
	even
xx1	dw	offset divx1
	dw	offset modx1
	dw	offset divx1neg
	dw	offset modx1neg
modx1:	mov	ax,dx
divx1:	xor	dx,dx
	ret	8
modx1neg: mov	ax,dx
divx1neg: xor	dx,dx
rneg:	negr
	ret	8
; 32/16 with 32-bit quotient, do in 2 parts.
div1:	mov	cx,ax				; save lo num
	mov	ax,dx
	xor	dx,dx
	div	bx				; ax = hi quo
	xchg	cx,ax				; save hi quo, get lo num
	div	bx				; ax = lo quo, dx = rem
	pop	bx
	pop	bp
	jmp	cs:xx1a[bx]
	even
xx1a	dw	offset divx1a
	dw	offset modx1
	dw	offset divx1aneg
	dw	offset modx1neg
divx1a:	mov	dx,cx				; hi quo
	ret	8
divx1aneg: mov	dx,cx
	jmp	rneg
; This is really a 32/32 bit division.
; (Note that the quotient cannot exceed 16 bits.)
; The following algorithm is taken from pp. 235-240 of Knuth, vol. 2
; (first edition).
; Start by normalizing the numerator and denominator.
div2:	or	ch,ch
	jz	div21				; ch == 0, but cl != 0
; Do 8 steps all at once.
	mov	bl,bh
	mov	bh,cl
	mov	cl,ch
	xor	ch,ch
	mov	al,ah
	mov	ah,dl
	mov	dl,dh
	xor	dh,dh
	rol	bx,1				; faster than jmp
div2a:	rcr	bx,1				; finish previous shift
div21:	shr	dx,1
	rcr	ax,1
	shr	cx,1
	jnz	div2a
	rcr	bx,1
; Now we can do a 32/16 divide.
div2x:	div	bx				; ax = quo, dx = rem
; Multiply by the denominator, and correct the result.
	mov	cx,ax				; save quotient
	mul	word ptr [bp+dhi]
	mov	bx,ax				; save lo part of hi product
	mov	ax,cx
	mul	word ptr [bp+dlo]
	add	dx,bx
; Now cx = trial quotient, (dx,ax) = cx * denominator.
	not	dx
	neg	ax
	cmc
	adc	dx,0				; double-precision neg
	jc	divz				; zero quotient
						; requires special handling
	add	ax,[bp+nlo]
	adc	dx,[bp+nhi]
	jc	divx
; Quotient is too large, adjust it.
div3:	dec	cx
	add	ax,[bp+dlo]
	adc	dx,[bp+dhi]
	jnc	div3
; All done.  (dx,ax) = remainder, cx = lo quotient.
divx:	pop	bx
	pop	bp
	jmp	cs:xx3[bx]
	even
xx3	dw	offset divx3
	dw	offset modx3
	dw	offset divx3neg
	dw	offset modx3neg
divx3:	mov	ax,cx
	xor	dx,dx
modx3:	ret	8
divx3neg: mov	ax,cx
	xor	dx,dx
modx3neg: jmp	rneg
; Handle zero quotient specially.
divz:	pop	bx
	jmp	cs:xxz[bx]
	even
xxz	dw	offset divxz
	dw	offset modxz
	dw	offset divxz
	dw	offset modxzneg
divxz:	pop	bp
	ret	8
modxzneg: negbp	nlo
modxz:	mov	ax,[bp+nlo]
	mov	dx,[bp+nhi]
	pop	bp
	ret	8
LUDIV@	endp
F_LUDIV@ endp

	endif					; FOR80386


	ifdef	NOFPU

; See gsmisc.c for the C version of this code.

; /*
;  * Floating multiply with fixed result, for avoiding floating point in
;  * common coordinate transformations.  Assumes IEEE representation,
;  * 16-bit short, 32-bit long.  Optimized for the case where the first
;  * operand has no more than 16 mantissa bits, e.g., where it is a user space
;  * coordinate (which are often integers).
;  *
;  * The assembly language version of this code is actually faster than
;  * the FPU, if the code is compiled with FPU_TYPE=0 (which requires taking
;  * a trap on every FPU operation).  If there is no FPU, the assembly
;  * language version of this code is over 10 times as fast as the
;  * emulated FPU.
;  */
; fixed
; fmul2fixed_(long /*float*/ a, long /*float*/ b)
; {

	PUBLIC	_fmul2fixed_
_fmul2fixed_ proc far
	push	bp
	mov	bp,sp
a	equ	6
alo	equ	a
ahi	equ	a+2
b	equ	10
blo	equ	b
bhi	equ	b+2
	push	si		; will hold ma
	push	di		; will hold mb

; 	int e = 260 + _fixed_shift - ((
; 		(((uint)(a >> 16)) & 0x7f80) + (((uint)(b >> 16)) & 0x7f80)
; 	  ) >> 7);

	mov	dx,[bp+ahi]
; dfmul2fixed enters here
fmf:	mov	cx,260+12
	mov	ax,[bp+bhi]
	and	ax,7f80h
	and	dx,7f80h
	add	ax,dx
	xchg	ah,al		; ror ax,7 without using cl
	rol	ax,1
	sub	cx,ax
	push	cx		; e

; 	ulong ma = (ushort)(a >> 8) | 0x8000;
; 	ulong mb = (ushort)(b >> 8) | 0x8000;

	mov	si,[bp+alo+1]	; unaligned
	clear	ax
	mov	di,[bp+blo+1]	; unaligned
	or	si,8000h
	or	di,8000h

; 	ulong p1 = ma * (b & 0xff);

	mov	al,[bp+blo]
	mul	si

;			(Do this later:)
; 	ulong p = ma * mb;

; 	if ( (byte)a )		/* >16 mantissa bits */

	cmp	byte ptr [bp+alo],0
	je	mshort

; 	{	ulong p2 = (a & 0xff) * mb;
; 		p += ((((uint)(byte)a * (uint)(byte)b) >> 8) + p1 + p2) >> 8;

	mov	cx,dx
	mov	bx,ax
	clear	ax
	mov	al,[bp+alo]
	clear	dx
	mov	dl,[bp+blo]
	mul	dx
	mov	dl,ah		; dx is zero
	add	bx,cx
	adc	cx,0
	clear	ax
	mov	al,[bp+alo]
	mul	di
	add	ax,bx
	adc	dx,cx

; 	}

mshort:

; 	else
; 		p += p1 >> 8;

	mov	bl,ah		; set (cx,bx) = (dx,ax) >> 8
	mov	bh,dl
	clear	cx
	mov	cl,dh
	mov	ax,si
	mul	di
	add	ax,bx
	adc	dx,cx

; 	if ( (uint)e < 32 )		/* e = -1 is possible */

	pop	cx		; e
	cmp	cx,16
	jb	shr1

; 	else if ( e >= 32 )		/* also detects a=0 or b=0 */

	cmp	cx,0
	jl	eneg
	sub	cx,16
	cmp	cx,16
	jge	shr0
	mov	ax,dx
	clear	dx
	shr	ax,cl
	jmp	ex

; 		return fixed_0;

shr0:	clear	ax
	clear	dx
	jmp	ex

; 	else
; 		p <<= -e;

	even
eneg:	neg	cx
	shl	dx,cl
	mov	bx,ax
	shl	ax,cl
	rol	bx,cl
	xor	bx,ax
	add	dx,bx
	jmp	ex

; 		p >>= e;

	even
shr1:	shr	ax,cl
	mov	bx,dx
	shr	dx,cl
	ror	bx,cl
	xor	bx,dx
	add	ax,bx

ex:

; 	return ((a ^ b) < 0 ? -p : p);

	mov	cx,[bp+ahi]
	xor	cx,[bp+bhi]
	jge	pos
	neg	dx
	neg	ax
	sbb	dx,0
pos:

; }

retu:	pop	di
	pop	si
	mov	sp,bp
	pop	bp
	ret

_fmul2fixed_ ENDP

; The same routine with the first argument a double rather than a float.
; The argument is split into two pieces to reduce data movement.

	PUBLIC	_dfmul2fixed_
_dfmul2fixed_ proc far
	push	bp
	mov	bp,sp
xalo	equ	6
;b	equ	10
xahi	equ	14
	push	si		; overlap this below
	push	di		; ditto

; Shuffle the arguments and then use fmul2fixed.

; Squeeze 3 exponent bits out of the top 35 bits of a.

	mov	dx,[bp+xahi+2]
	mov	bx,0c000h
	mov	ax,[bp+xahi]
	and	bx,dx
	mov	cx,[bp+xalo+2]
	and	dx,7ffh		; get rid of discarded bits
	add	cx,cx		; faster than shl!
	jz	cz		; detect common case
	adc	ax,ax		; faster than rcl!
	adc	dx,dx
	add	cx,cx
	adc	ax,ax
	adc	dx,dx
	add	cx,cx
	adc	ax,ax
	mov	[bp+alo],ax
	adc	dx,dx
	or	dx,bx
	mov	[bp+ahi],dx
	jmp	fmf
	even
cz:	adc	ax,ax
	adc	dx,dx
	add	ax,ax
	adc	dx,dx
	add	ax,ax
	mov	[bp+alo],ax
	adc	dx,dx
	or	dx,bx
	mov	[bp+ahi],dx
	jmp	fmf

_dfmul2fixed_ ENDP

	endif					; NOFPU


; Transpose an 8x8 bit matrix.  See gsmisc.c for the algorithm in C.
	PUBLIC	_memflip8x8
_memflip8x8 proc far
	push	ds
	push	si
	push	di
		; After pushing, the offsets of the parameters are:
		; byte *inp=10, int line_size=14, byte *outp=16, int dist=20.
	mov	si,sp
	mov	di,ss:[si+14]			; line_size
	lds	si,ss:[si+10]			; inp
		; We assign variables to registers as follows:
		; ax = AE, bx = BF, cx (or di) = CG, dx = DH.
		; Load the input data.  Initially we assign
		; ax = AB, bx = EF, cx (or di) = CD, dx = GH.
	mov	ah,[si]
iload	macro	reg
	add	si,di
	mov	reg,[si]
	endm
	iload	al
	iload	ch
	iload	cl
	iload	bh
	iload	bl
	iload	dh
	iload	dl
		; Transposition macro, see C code for explanation.
trans	macro	reg1,reg2,shift,mask
	mov	si,reg1
	shr	si,shift
	xor	si,reg2
	and	si,mask
	xor	reg2,si
	shl	si,shift
	xor	reg1,si
	endm
		; Do 4x4 transpositions
	mov	di,cx			; we need cl for the shift count
	mov	cl,4
	trans	bx,ax,cl,0f0fh
	trans	dx,di,cl,0f0fh
		; Swap B/E, D/G
	xchg	al,bh
	mov	cx,di
	xchg	cl,dh
		; Do 2x2 transpositions
	mov	di,cx				; need cl again
	mov	cl,2
	trans	di,ax,cl,3333h
	trans	dx,bx,cl,3333h
	mov	cx,di				; done shifting >1
		; Do 1x1 transpositions
	trans	bx,ax,1,5555h
	trans	dx,cx,1,5555h
		; Store result
	mov	si,sp
	mov	di,ss:[si+20]			; dist
	lds	si,ss:[si+16]			; outp
	mov	[si],ah
istore	macro	reg
	add	si,di
	mov	[si],reg
	endm
	istore	bh
	istore	ch
	istore	dh
	istore	al
	istore	bl
	istore	cl
	istore	dl
		; All done
	pop	di
	pop	si
	pop	ds
	ret
_memflip8x8 ENDP


utilasm_TEXT ENDS
	END
