/* Copyright (C) 2002 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: stdpn.h,v 1.2 2002/06/16 08:59:16 lpd Exp $ */
/* Pn macros for pre-ANSI compiler compatibility */

#ifndef stdpn_INCLUDED
#  define stdpn_INCLUDED

/*
 * We formerly supported "traditional" (pre-ANSI) C compilers, by using
 * these macros for formal parameter lists and defining them as empty
 * for pre-ANSI compilers, with the syntax
 *      resulttype func(Pn(arg1, ..., argn));
 * However, we no longer support pre-ANSI compilers; these macros are
 * deprecated (should not be used in new code), and eventually will be
 * removed.
 */

#define P0() void
#define P1(t1) t1
#define P2(t1,t2) t1,t2
#define P3(t1,t2,t3) t1,t2,t3
#define P4(t1,t2,t3,t4) t1,t2,t3,t4
#define P5(t1,t2,t3,t4,t5) t1,t2,t3,t4,t5
#define P6(t1,t2,t3,t4,t5,t6) t1,t2,t3,t4,t5,t6
#define P7(t1,t2,t3,t4,t5,t6,t7) t1,t2,t3,t4,t5,t6,t7
#define P8(t1,t2,t3,t4,t5,t6,t7,t8) t1,t2,t3,t4,t5,t6,t7,t8
#define P9(t1,t2,t3,t4,t5,t6,t7,t8,t9) t1,t2,t3,t4,t5,t6,t7,t8,t9
#define P10(t1,t2,t3,t4,t5,t6,t7,t8,t9,t10) t1,t2,t3,t4,t5,t6,t7,t8,t9,t10
#define P11(t1,t2,t3,t4,t5,t6,t7,t8,t9,t10,t11) t1,t2,t3,t4,t5,t6,t7,t8,t9,t10,t11
#define P12(t1,t2,t3,t4,t5,t6,t7,t8,t9,t10,t11,t12) t1,t2,t3,t4,t5,t6,t7,t8,t9,t10,t11,t12
#define P13(t1,t2,t3,t4,t5,t6,t7,t8,t9,t10,t11,t12,t13) t1,t2,t3,t4,t5,t6,t7,t8,t9,t10,t11,t12,t13
#define P14(t1,t2,t3,t4,t5,t6,t7,t8,t9,t10,t11,t12,t13,t14) t1,t2,t3,t4,t5,t6,t7,t8,t9,t10,t11,t12,t13,t14
#define P15(t1,t2,t3,t4,t5,t6,t7,t8,t9,t10,t11,t12,t13,t14,t15) t1,t2,t3,t4,t5,t6,t7,t8,t9,t10,t11,t12,t13,t14,t15
#define P16(t1,t2,t3,t4,t5,t6,t7,t8,t9,t10,t11,t12,t13,t14,t15,t16) t1,t2,t3,t4,t5,t6,t7,t8,t9,t10,t11,t12,t13,t14,t15,t16

#endif /* stdpn_INCLUDED */
/* Copyright (C) 1993-2003 artofcode LLC.  All rights reserved.
  
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

/* $Id: stdpre.h,v 1.22 2003/12/09 21:17:59 giles Exp $ */
/* Standard definitions for Ghostscript code not needing arch.h */

#ifndef stdpre_INCLUDED
#  define stdpre_INCLUDED

/*
 * Here we deal with the vagaries of various C compilers.  We assume that:
 *      ANSI-standard Unix compilers define __STDC__.
 *      gcc defines __GNUC__.
 *      Borland Turbo C and Turbo C++ define __MSDOS__ and __TURBOC__.
 *      Borland C++ defines __BORLANDC__, __MSDOS__, and __TURBOC__.
 *      Microsoft C/C++ defines _MSC_VER and _MSDOS.
 *      Watcom C defines __WATCOMC__ and MSDOS.
 *      MetroWerks C defines __MWERKS__.
 *
 * We arrange to define __MSDOS__ on all the MS-DOS platforms.
 */
#if (defined(MSDOS) || defined(_MSDOS)) && !defined(__MSDOS__)
#  define __MSDOS__
#endif
/*
 * Also, not used much here, but used in other header files, we assume:
 *      Unix System V environments define SYSV.
 *      The SCO ODT compiler defines M_SYSV and M_SYS3.
 *      VMS systems define VMS.
 *      OSF/1 compilers define __osf__ or __OSF__.
 *        (The VMS and OSF/1 C compilers handle prototypes and const,
 *        but do not define __STDC__.)
 *      bsd 4.2 or 4.3 systems define BSD4_2.
 *      POSIX-compliant environments define _POSIX_SOURCE.
 *      Motorola 88K BCS/OCS systems defined m88k.
 *
 * We make fairly heroic efforts to confine all uses of these flags to
 * header files, and never to use them in code.
 */
#if defined(__osf__) && !defined(__OSF__)
#  define __OSF__		/* */
#endif
#if defined(M_SYSV) && !defined(SYSV)
#  define SYSV			/* */
#endif
#if defined(M_SYS3) && !defined(__SVR3)
#  define __SVR3		/* */
#endif

#if defined(__STDC__) || defined(__MSDOS__) || defined(__convex__) || defined(VMS) || defined(__OSF__) || defined(__WIN32__) || defined(__IBMC__) || defined(M_UNIX) || defined(__GNUC__) || defined(__BORLANDC__)
# if !(defined(M_XENIX) && !defined(__GNUC__))	/* SCO Xenix cc is broken */
#  define __PROTOTYPES__	/* */
# endif
#endif

/* Define dummy values for __FILE__ and __LINE__ if the compiler */
/* doesn't provide these.  Note that places that use __FILE__ */
/* must check explicitly for a null pointer. */
#ifndef __FILE__
#  define __FILE__ NULL
#endif
#ifndef __LINE__
#  define __LINE__ 0
#endif

/* Disable 'const' and 'volatile' if the compiler can't handle them. */
#ifndef __PROTOTYPES__
#  undef const
#  define const			/* */
#  undef volatile
#  define volatile		/* */
#endif

/* Disable 'inline' if the compiler can't handle it. */
#ifdef __DECC
#  undef inline
#  define inline __inline
#else
#  ifdef __GNUC__
/* Define inline as __inline__ so -pedantic won't produce a warning. */
#    undef inline
#    define inline __inline__
#  else
#    if !(defined(__MWERKS__) || defined(inline))
#      define inline		/* */
#    endif
#  endif
#endif

/*
 * Provide a way to include inline procedures in header files, regardless of
 * whether the compiler (A) doesn't support inline at all, (B) supports it
 * but also always compiles a closed copy, (C) supports it but somehow only
 * includes a single closed copy in the executable, or (D) supports it and
 * also supports a different syntax if no closed copy is desired.
 *
 * The code that appears just after this comment indicates which compilers
 * are of which kind.  (Eventually this might be determined automatically.)
 *	(A) and (B) require nothing here.
 *	(C) requires
 *		#define extern_inline inline
 *	(D) requires
 *		#define extern_inline extern inline  // or whatever
 * Note that for case (B), the procedure will only be declared inline in
 * the .c file where its closed copy is compiled.
 */
#ifdef __GNUC__
#  define extern_inline extern inline
#endif

/*
 * To include an inline procedure xyz in a header file abc.h, use the
 * following template in the header file:

extern_inline int xyz(<<parameters>>)
#if HAVE_EXTERN_INLINE || defined(INLINE_INCLUDE_xyz)
{
    <<body>>
}
#else
;
#endif

 * And use the following in whichever .c file takes responsibility for
 * including the closed copy of xyz:

#define EXTERN_INCLUDE_xyz	// must precede all #includes
#include "abc.h"

 * The definitions of the EXTERN_INCLUDE_ macros must precede *all* includes
 * because there is no way to know whether some other .h file #includes abc.h
 * indirectly, and because of the protection against double #includes, the
 * EXTERN_INCLUDE_s must be defined before the first inclusion of abc.h.
 */

/*
 * The following is generic code that does not need per-compiler
 * customization.
 */
#ifdef extern_inline
#  define HAVE_EXTERN_INLINE 1
#else
#  define extern_inline /* */
#  define HAVE_EXTERN_INLINE 0
#endif

/*
 * Some compilers give a warning if a function call that returns a value
 * is used as a statement; a few compilers give an error for the construct
 * (void)0, which is contrary to the ANSI standard.  Since we don't know of
 * any compilers that do both, we define a macro here for discarding
 * the value of an expression statement, which can be defined as either
 * including or not including the cast.  (We don't conditionalize this here,
 * because no commercial compiler gives the error on (void)0, although
 * some give warnings.)  */
#define DISCARD(expr) ((void)(expr))
/* Backward compatibility */
#define discard(expr) DISCARD(expr)

/*
 * Some versions of the Watcom compiler give a "Comparison result always
 * 0/1" message that we want to suppress because it gets in the way of
 * meaningful warnings.
 */
#ifdef __WATCOMC__
#  pragma disable_message(124);
#endif

/*
 * Some versions of gcc have a bug such that after
	byte *p;
	...
	x = *(long *)p;
 * the compiler then thinks that p always points to long-aligned data.
 * Detect this here so it can be handled appropriately in the few places
 * that (we think) matter.
 */
#ifdef __GNUC__
# if __GNUC__ == 2 & (7 < __GNUC_MINOR__ <= 95)
#  define ALIGNMENT_ALIASING_BUG
# endif
#endif

/*
 * The SVR4.2 C compiler incorrectly considers the result of << and >>
 * to be unsigned if the left operand is signed and the right operand is
 * unsigned.  We believe this only causes trouble in Ghostscript code when
 * the right operand is a sizeof(...), which is unsigned for this compiler.
 * Therefore, we replace the relevant uses of sizeof with size_of:
 */
#define size_of(x) ((int)(sizeof(x)))

/*
 * far_data was formerly used for static data that had to be assigned its
 * own segment on PCs with 64K segments.  This was supported in Borland C++,
 * but none of the other compilers.  Since we no longer support
 * small-segment systems, far_data is vacuous.
 */
#undef far_data
#define far_data /* */

/*
 * Get the number of elements of a statically dimensioned array.
 * Note that this also works on array members of structures.
 */
#define countof(a) (sizeof(a) / sizeof((a)[0]))
#define count_of(a) (size_of(a) / size_of((a)[0]))

/*
 * Get the offset of a structure member.  Amazingly enough, the simpler
 * definition works on all compilers except for one broken MIPS compiler
 * and the IBM RS/6000.  Unfortunately, because of these two compilers,
 * we have to use the more complex definition.  Even more unfortunately,
 * the more complex definition doesn't work on the MetroWerks
 * CodeWarrior compiler (Macintosh and BeOS).
 */
#ifdef __MWERKS__
#define offset_of(type, memb)\
 ((int) &((type *) 0)->memb)
#else
#define offset_of(type, memb)\
 ((int) ( (char *)&((type *)0)->memb - (char *)((type *)0) ))
#endif

/*
 * Get the alignment of a pointer modulo a given power of 2.
 * There is no portable way to do this, but the following definition
 * works on all reasonable systems.
 */
#define ALIGNMENT_MOD(ptr, modu)\
  ((uint)( ((const char *)(ptr) - (const char *)0) & ((modu) - 1) ))

/* Define short names for the unsigned types. */
typedef unsigned char byte;
typedef unsigned char uchar;
typedef unsigned short ushort;
typedef unsigned int uint;
typedef unsigned long ulong;

/* Since sys/types.h may define one or more of these (depending on
 * the platform), we have to take steps to prevent name clashes.
 * Unfortunately this can clobber valid definitions for the size-
 * specific types, but there's no simple solution.
 *
 * NOTE: This requires that you include std.h *before* any other
 * header file that includes sys/types.h.
 *
 */
#define bool bool_		/* (maybe not needed) */
#define uchar uchar_
#define uint uint_
#define ushort ushort_
#define ulong ulong_
#include <sys/types.h>
#undef bool
#undef uchar
#undef uint
#undef ushort
#undef ulong

/*
 * Define a Boolean type.  Even though we would like it to be
 * unsigned char, it pretty well has to be int, because
 * that's what all the relational operators and && and || produce.
 * We can't make it an enumerated type, because ints don't coerce
 * freely to enums (although the opposite is true).
 * Unfortunately, at least some C++ compilers have a built-in bool type,
 * and the MetroWerks C++ compiler insists that bool be equivalent to
 * unsigned char.
 */
#ifndef __cplusplus
#ifdef __BEOS__
typedef unsigned char bool;
#else
typedef int bool;
#endif
#endif
/*
 * Older versions of MetroWerks CodeWarrior defined true and false, but they're now
 * an enum in the (MacOS) Universal Interfaces. The only way around this is to escape
 * our own definitions wherever MacTypes.h is included.
 */
#ifndef __MACOS__
#undef false
#define false ((bool)0)
#undef true
#define true ((bool)1)
#endif /* __MACOS__ */

/*
 * Compilers disagree as to whether macros used in macro arguments
 * should be expanded at the time of the call, or at the time of
 * final expansion.  Even authoritative documents disagree: the ANSI
 * standard says the former, but Harbison and Steele's book says the latter.
 * In order to work around this discrepancy, we have to do some very
 * ugly things in a couple of places.  We mention it here because
 * it might well trip up future developers.
 */

/*
 * Define the type to be used for ordering pointers (<, >=, etc.).
 * The Borland and Microsoft large models only compare the offset part
 * of segmented pointers.  Semantically, the right type to use for the
 * comparison is char huge *, but we have no idea how expensive comparing
 * such pointers is, and any type that compares all the bits of the pointer,
 * gives the right result for pointers in the same segment, and keeps
 * different segments disjoint will do.
 */
#if defined(__TURBOC__) || defined(_MSC_VER)
typedef unsigned long ptr_ord_t;
#else
typedef const char *ptr_ord_t;
#endif
/* Define all the pointer comparison operations. */
#define _PTR_CMP(p1, rel, p2)  ((ptr_ord_t)(p1) rel (ptr_ord_t)(p2))
#define PTR_LE(p1, p2) _PTR_CMP(p1, <=, p2)
#define PTR_LT(p1, p2) _PTR_CMP(p1, <, p2)
#define PTR_GE(p1, p2) _PTR_CMP(p1, >=, p2)
#define PTR_GT(p1, p2) _PTR_CMP(p1, >, p2)
#define PTR_BETWEEN(ptr, lo, hi)\
  (PTR_GE(ptr, lo) && PTR_LT(ptr, hi))

/* Define  min and max, but make sure to use the identical definition */
/* to the one that all the compilers seem to have.... */
#ifndef min
#  define min(a, b) (((a) < (b)) ? (a) : (b))
#endif
#ifndef max
#  define max(a, b) (((a) > (b)) ? (a) : (b))
#endif

/* Define a standard way to round values to a (constant) modulus. */
#define ROUND_DOWN(value, modulus)\
  ( (modulus) & ((modulus) - 1) ?	/* not a power of 2 */\
    (value) - (value) % (modulus) :\
    (value) & -(modulus) )
#define ROUND_UP(value, modulus)\
  ( (modulus) & ((modulus) - 1) ?	/* not a power of 2 */\
    ((value) + ((modulus) - 1)) / (modulus) * (modulus) :\
    ((value) + ((modulus) - 1)) & -(modulus) )
/* Backward compatibility */
#define round_up(v, m) ROUND_UP(v, m)
#define round_down(v, m) ROUND_DOWN(v, m)

/*
 * In pre-ANSI C, float parameters get converted to double.
 * However, if we pass a float to a function that has been declared
 * with a prototype, and the parameter has been declared as float,
 * the ANSI standard specifies that the parameter is left as float.
 * To avoid problems caused by missing prototypes,
 * we declare almost all float parameters as double.
 */
typedef double floatp;

/*
 * Because of C's strange insistence that ; is a terminator and not a
 * separator, compound statements {...} are not syntactically equivalent to
 * single statements.  Therefore, we define here a compound-statement
 * construct that *is* syntactically equivalent to a single statement.
 * Usage is
 *      BEGIN
 *        ...statements...
 *      END
 */
#define BEGIN	do {
#define END	} while (0)

/*
 * Define a handy macro for a statement that does nothing.
 * We can't just use an empty statement, since this upsets some compilers.
 */
#ifndef DO_NOTHING
#  define DO_NOTHING BEGIN END
#endif

/*
 * For accountability, debugging, and error messages, we pass a client
 * identification string to alloc and free, and possibly other places as
 * well.  Define the type for these strings.
 */
typedef const char *client_name_t;
/****** WHAT TO DO ABOUT client_name_string ? ******/
#define client_name_string(cname) (cname)

/*
 * If we are debugging, make all static variables and procedures public
 * so they get passed through the linker.
 */
#define public			/* */
/*
 * We separate out the definition of private this way so that
 * we can temporarily #undef it to handle the X Windows headers,
 * which define a member named private.
 */
#ifdef NOPRIVATE
# define private_		/* */
#else
# define private_ static
#endif
#define private private_

/*
 * Define the now-deprecated Pn macros for pre-ANSI compiler compatibility.
 * The double-inclusion check is replicated here because of the way that
 * jconfig.h is constructed.
 */
#ifndef stdpn_INCLUDED
#  define stdpn_INCLUDED
#include "stdpn.h"
#endif /* stdpn_INCLUDED */

/*
 * Define success and failure codes for 'exit'.  The only system on which
 * they are different is VMS with older DEC C versions.  We aren't sure
 * in what version DEC C started being compatible with the rest of the
 * world, and we don't know what the story is with VAX C.  If you have
 * problems, uncomment the following line or add -DOLD_VMS_C to the C
 * command line.
 */
/*#define OLD_VMS_C*/
#if defined(VMS)
#  define exit_FAILED 18
#  if (defined(OLD_VMS_C) || !defined(__DECC))
#    define exit_OK 1
#  else
#    define exit_OK 0
#  endif
#else
#  define exit_OK 0
#  define exit_FAILED 1
#endif

#endif /* stdpre_INCLUDED */
/* Copyright (C) 1994, 1998 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: gsjconf.h,v 1.4 2002/02/21 22:24:52 giles Exp $ */
/* jconfig.h file for Independent JPEG Group code */

#ifndef gsjconf_INCLUDED
#  define gsjconf_INCLUDED

/*
 * We should have the following here:

#include "stdpre.h"

 * But because of the directory structure used to build the IJG library, we
 * actually concatenate stdpre.h on the front of this file instead to
 * construct the jconfig.h file used for the compilation.
 */

#include "arch.h"

/* See IJG's jconfig.doc for the contents of this file. */

#ifdef __PROTOTYPES__
#  define HAVE_PROTOTYPES
#endif

#define HAVE_UNSIGNED_CHAR
#define HAVE_UNSIGNED_SHORT
#undef CHAR_IS_UNSIGNED

#ifdef __STDC__			/* is this right? */
#  define HAVE_STDDEF_H
#  define HAVE_STDLIB_H
#endif

#undef NEED_BSD_STRINGS		/* WRONG */
#undef NEED_SYS_TYPES_H		/* WRONG */
#undef NEED_FAR_POINTERS
#undef NEED_SHORT_EXTERNAL_NAMES

#undef INCOMPLETE_TYPES_BROKEN

/* The following is documented in jmemsys.h, not jconfig.doc. */
#if ARCH_SIZEOF_INT <= 2
#  undef MAX_ALLOC_CHUNK
#  define MAX_ALLOC_CHUNK 0xfff0
#endif

#ifdef JPEG_INTERNALS

#if ARCH_ARITH_RSHIFT == 0
#  define RIGHT_SHIFT_IS_UNSIGNED
#else
#  undef RIGHT_SHIFT_IS_UNSIGNED
#endif

#endif /* JPEG_INTERNALS */

#endif /* gsjconf_INCLUDED */

