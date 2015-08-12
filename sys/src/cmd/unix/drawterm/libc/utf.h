/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#ifndef _UTFH_
#define _UTFH_ 1

typedef unsigned int Rune; /* 32 bits */

enum { UTFmax = 4,          /* maximum bytes per rune */
       Runesync = 0x80,     /* cannot represent part of a UTF sequence (<) */
       Runeself = 0x80,     /* rune and UTF sequences are the same (<) */
       Runeerror = 0xFFFD,  /* decoding error in UTF */
       Runemax = 0x10FFFF,  /* 21-bit rune */
       Runemask = 0x1FFFFF, /* bits used by runes (see grep) */
};

/*
 * rune routines
 */
extern int runetochar(char*, Rune*);
extern int chartorune(Rune*, char*);
extern int runelen(long);
extern int runenlen(Rune*, int);
extern int fullrune(char*, int);
extern int utflen(char*);
extern int utfnlen(char*, long);
extern char* utfrune(char*, long);
extern char* utfrrune(char*, long);
extern char* utfutf(char*, char*);
extern char* utfecpy(char*, char*, char*);

extern Rune* runestrcat(Rune*, Rune*);
extern Rune* runestrchr(Rune*, Rune);
extern int runestrcmp(Rune*, Rune*);
extern Rune* runestrcpy(Rune*, Rune*);
extern Rune* runestrncpy(Rune*, Rune*, long);
extern Rune* runestrecpy(Rune*, Rune*, Rune*);
extern Rune* runestrdup(Rune*);
extern Rune* runestrncat(Rune*, Rune*, long);
extern int runestrncmp(Rune*, Rune*, long);
extern Rune* runestrrchr(Rune*, Rune);
extern long runestrlen(Rune*);
extern Rune* runestrstr(Rune*, Rune*);

extern Rune tolowerrune(Rune);
extern Rune totitlerune(Rune);
extern Rune toupperrune(Rune);
extern int isalpharune(Rune);
extern int islowerrune(Rune);
extern int isspacerune(Rune);
extern int istitlerune(Rune);
extern int isupperrune(Rune);

#endif
