#ifndef __U_H
#define __U_H
#ifndef _PLAN9_SOURCE
   This header file is an extension to ANSI/POSIX
#endif

#define nil		((void*)0)
typedef	unsigned short	ushort;
typedef	unsigned char	uchar;
typedef unsigned long	ulong;
typedef unsigned int	uint;
typedef   signed char	schar;
typedef	long long	vlong;
typedef	unsigned long long uvlong;
typedef	ushort		Rune;
typedef 	union FPdbleword FPdbleword;
typedef	char*	p9va_list;

#endif
