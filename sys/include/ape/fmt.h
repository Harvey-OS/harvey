/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#ifndef _PLAN9_SOURCE
  This header file is an extension to ANSI/POSIX
#endif

#ifndef __FMT_H_
#define __FMT_H_
#pragma src "/sys/src/ape/lib/fmt"
#pragma lib "/$M/lib/ape/libfmt.a"

#include <u.h>

/*
 * The authors of this software are Rob Pike and Ken Thompson.
 *              Copyright (c) 2002 by Lucent Technologies.
 * Permission to use, copy, modify, and distribute this software for any
 * purpose without fee is hereby granted, provided that this entire notice
 * is included in all copies of any software which is or includes a copy
 * or modification of this software and in all copies of the supporting
 * documentation for such software.
 * THIS SOFTWARE IS BEING PROVIDED "AS IS", WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTY.  IN PARTICULAR, NEITHER THE AUTHORS NOR LUCENT TECHNOLOGIES MAKE ANY
 * REPRESENTATION OR WARRANTY OF ANY KIND CONCERNING THE MERCHANTABILITY
 * OF THIS SOFTWARE OR ITS FITNESS FOR ANY PARTICULAR PURPOSE.
 */

#include <stdarg.h>
#include <utf.h>

typedef struct Fmt	Fmt;
struct Fmt{
	unsigned char	runes;		/* output buffer is runes or chars? */
	void	*start;			/* of buffer */
	void	*to;			/* current place in the buffer */
	void	*stop;			/* end of the buffer; overwritten if flush fails */
	int	(*flush)(Fmt *);	/* called when to == stop */
	void	*farg;			/* to make flush a closure */
	int	nfmt;			/* num chars formatted so far */
	va_list	args;			/* args passed to dofmt */
	int	r;			/* % format Rune */
	int	width;
	int	prec;
	unsigned long	flags;
};

enum{
	FmtWidth	= 1,
	FmtLeft		= FmtWidth << 1,
	FmtPrec		= FmtLeft << 1,
	FmtSharp	= FmtPrec << 1,
	FmtSpace	= FmtSharp << 1,
	FmtSign		= FmtSpace << 1,
	FmtZero		= FmtSign << 1,
	FmtUnsigned	= FmtZero << 1,
	FmtShort	= FmtUnsigned << 1,
	FmtLong		= FmtShort << 1,
	FmtVLong	= FmtLong << 1,
	FmtComma	= FmtVLong << 1,
	FmtByte		= FmtComma << 1,
	FmtLDouble	= FmtByte << 1,

	FmtFlag		= FmtLDouble << 1
};

#ifdef	__cplusplus
extern "C" { 
#endif

extern	int	print(int8_t*, ...);
extern	int8_t*	seprint(int8_t*, int8_t*, int8_t*, ...);
extern	int8_t*	vseprint(int8_t*, int8_t*, int8_t*, va_list);
extern	int	snprint(int8_t*, int, int8_t*, ...);
extern	int	vsnprint(int8_t*, int, int8_t*, va_list);
extern	int8_t*	smprint(int8_t*, ...);
extern	int8_t*	vsmprint(int8_t*, va_list);
extern	int	sprint(int8_t*, int8_t*, ...);
extern	int	fprint(int, int8_t*, ...);
extern	int	vfprint(int, int8_t*, va_list);

extern	int	runesprint(Rune*, int8_t*, ...);
extern	int	runesnprint(Rune*, int, int8_t*, ...);
extern	int	runevsnprint(Rune*, int, int8_t*, va_list);
extern	Rune*	runeseprint(Rune*, Rune*, int8_t*, ...);
extern	Rune*	runevseprint(Rune*, Rune*, int8_t*, va_list);
extern	Rune*	runesmprint(int8_t*, ...);
extern	Rune*	runevsmprint(int8_t*, va_list);

extern	int	fmtfdinit(Fmt*, int, int8_t*, int);
extern	int	fmtfdflush(Fmt*);
extern	int	fmtstrinit(Fmt*);
extern	int8_t*	fmtstrflush(Fmt*);
extern	int	runefmtstrinit(Fmt*);

extern	int	quotestrfmt(Fmt *f);
extern	void	quotefmtinstall(void);
extern	int	(*fmtdoquote)(int);


extern	int	fmtinstall(int, int (*)(Fmt*));
extern	int	dofmt(Fmt*, int8_t*);
extern	int	fmtprint(Fmt*, int8_t*, ...);
extern	int	fmtvprint(Fmt*, int8_t*, va_list);
extern	int	fmtrune(Fmt*, int);
extern	int	fmtstrcpy(Fmt*, int8_t*);

extern	double	fmtstrtod(const int8_t *, int8_t **);
extern	double	fmtcharstod(int(*)(void*), void*);

extern	void	werrstr(const int8_t*, ...);

#ifdef	__cplusplus
}
#endif

#endif
