/* Copyright (C) 1991, 1992 Aladdin Enterprises.  All rights reserved.
  
  This file is part of Aladdin Ghostscript.
  
  Aladdin Ghostscript is distributed with NO WARRANTY OF ANY KIND.  No author
  or distributor accepts any responsibility for the consequences of using it,
  or for whether it serves any particular purpose or works at all, unless he
  or she says so in writing.  Refer to the Aladdin Ghostscript Free Public
  License (the "License") for full details.
  
  Every copy of Aladdin Ghostscript must include a copy of the License,
  normally in a plain ASCII text file named PUBLIC.  The License grants you
  the right to copy, modify and redistribute Aladdin Ghostscript, but only
  under certain conditions described in the License.  Among other things, the
  License requires that the copyright notice and this notice be preserved on
  all copies.
*/

/* dos_.h */
/* Generic MS-DOS interface */

/* This file is needed because the various DOS compilers */
/* provide slightly different procedures for interfacing to DOS and */
/* the I/O hardware, and because the Watcom compiler is 32-bit. */
#include <dos.h>
#if defined(__WATCOMC__) || defined(_MSC_VER)

/* ---------------- Microsoft C/C++, all models; */
/* ---------------- Watcom compiler, 32-bit flat model. */
/* ---------------- inp/outp prototypes are in conio.h, not dos.h. */

#  include <conio.h>
#  define inport(px) inpw(px)
#  define inportb(px) inp(px)
#  define outport(px,w) outpw(px,w)
#  define outportb(px,b) outp(px,b)
#  define enable() _enable()
#  define disable() _disable()
#  define PTR_OFF(ptr) ((ushort)(uint)(ptr))
/* Define the structure and procedures for file enumeration. */
#define ff_name name
#define dos_findfirst(n,b) _dos_findfirst(n, _A_NORMAL | _A_RDONLY, b)
#define dos_findnext(b) _dos_findnext(b)

/* Define things that differ between Watcom and Microsoft. */
#  ifdef __WATCOMC__
#    define MK_PTR(seg,off) (((seg) << 4) + (off))
#    define int86 int386
#    define int86x int386x
#    define rshort w
#    define ff_struct_t struct find_t
#  else
#    define MK_PTR(seg,off) (((ulong)(seg) << 16) + (off))
#    define cputs _cputs
#    define fdopen _fdopen
#    define O_BINARY _O_BINARY
#    define REGS _REGS
#    define rshort x
#    define ff_struct_t struct _find_t
#    define stdprn _stdprn
#  endif

#else			/* not Watcom or Microsoft */

/* ---------------- Borland compiler, 16:16 pseudo-segmented model. */
/* ---------------- ffblk is in dir.h, not dos.h. */
#include <dir.h>
#  define MK_PTR(seg,off) MK_FP(seg,off)
#  define PTR_OFF(ptr) FP_OFF(ptr)
/* Define the regs union tag for short registers. */
#  define rshort x
/* Define the structure and procedures for file enumeration. */
#define ff_struct_t struct ffblk
#define dos_findfirst(n,b) findfirst(n, b, 0)
#define dos_findnext(b) findnext(b)

#endif
