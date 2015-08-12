/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#ifndef _UTF_H_
#define _UTF_H_ 1
#pragma lib "/$M/lib/ape/libutf.a"
#pragma src "/sys/src/ape/lib/utf"

#if defined(__cplusplus)
extern "C" {
#endif

typedef unsigned int Rune; /* 32 bits */

enum {
	UTFmax = 4,	 /* maximum bytes per rune */
	Runesync = 0x80,    /* cannot represent part of a UTF sequence (<) */
	Runeself = 0x80,    /* rune and UTF sequences are the same (<) */
	Runeerror = 0xFFFD, /* decoding error in UTF */
	Runemax = 0x10FFFF, /* 21-bit rune */
};

/*
 * rune routines
 */
extern int runetochar(char *, Rune *);
extern int chartorune(Rune *, char *);
extern int runelen(int32_t);
extern int runenlen(Rune *, int);
extern int fullrune(char *, int);
extern int utflen(char *);
extern int utfnlen(char *, int32_t);
extern char *utfrune(char *, int32_t);
extern char *utfrrune(char *, int32_t);
extern char *utfutf(char *, char *);
extern char *utfecpy(char *, char *, char *);

extern Rune *runestrcat(Rune *, Rune *);
extern Rune *runestrchr(Rune *, Rune);
extern int runestrcmp(Rune *, Rune *);
extern Rune *runestrcpy(Rune *, Rune *);
extern Rune *runestrncpy(Rune *, Rune *, int32_t);
extern Rune *runestrecpy(Rune *, Rune *, Rune *);
extern Rune *runestrdup(Rune *);
extern Rune *runestrncat(Rune *, Rune *, int32_t);
extern int runestrncmp(Rune *, Rune *, int32_t);
extern Rune *runestrrchr(Rune *, Rune);
extern int32_t runestrlen(Rune *);
extern Rune *runestrstr(Rune *, Rune *);

extern Rune tolowerrune(Rune);
extern Rune totitlerune(Rune);
extern Rune toupperrune(Rune);
extern int isalpharune(Rune);
extern int islowerrune(Rune);
extern int isspacerune(Rune);
extern int istitlerune(Rune);
extern int isupperrune(Rune);

#if defined(__cplusplus)
}
#endif
#endif
