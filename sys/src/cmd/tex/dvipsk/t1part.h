/*t1part.h from t1part.c version 1.59 beta (c)1994, 1996
by Sergey Lesenko
lesenko@desert.ihep.su
 *
 *   It is distributed with no warranty of any kind.
 *   You may modify and use this program. It can be included
 *   in any distribution, commercial or otherwise, so long as
 *   copyright notice be preserved on all copies.
 */
#ifdef KPATHSEA
#include <kpathsea/c-ctype.h>
#else /* not KPATHSEA */
#include <assert.h>

#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#endif /* not KPATHSEA */

#define DVIPS

/*
#define DOS
#define BORLANDC
*/

#ifdef DEBUG

extern int debug_flag;

#define D_VIEW_VECTOR    (1<<8)
#define D_CALL_SUBR      (1<<9)

#endif

#ifdef DVIPS
extern FILE *search();
#ifndef KPATHSEA
extern char *headerpath ;
#endif /* not KPATHSEA */
#define psfopen(A,B) search(headerpath,A,B)
#else
#define psfopen(A,B)  fopen(A,B)
#endif

#ifdef KPATHSEA
#define OPEN_READ_BINARY FOPEN_RBIN_MODE
#else
#if defined(DOS) || defined(WIN32)
#define OPEN_READ_BINARY "rb"
#else
#define OPEN_READ_BINARY "r"
#endif
#endif /* not KPATHSEA */

#ifdef DOS
typedef unsigned char  ub1;
typedef unsigned long  ub4;
#else
typedef unsigned char  ub1;
typedef unsigned long int   ub4;
#endif

#ifdef BORLANDC
typedef unsigned char typetemp;
#define _HUGE huge
#else
typedef unsigned char typetemp;
#define _HUGE
#endif

#ifdef BORLANDC
#include <alloc.h>
#define UniRealloc farrealloc
#define UniFree farfree
#else
#ifdef KPATHSEA
#define UniRealloc  xrealloc
#define UniFree free
#else
#define UniRealloc  realloc
#define UniFree free
#endif /* not KPATHSEA */
#endif

#include "protos.h"
#if 0
struct Char * AddChar ();
void AddStr();
void BinEDeCrypt ();
int  DeCodeStr();
int  DefTypeFont();
unsigned
char CDeCrypt();
void CorrectGrid();
int  CharEncoding ();
void CheckChoosing();
int  ChooseChar();
int  ChooseVect();
int  EndOfEncoding();
void ErrorOfScan ();
int FindCharW();
void FindEncoding ();
int  FindKeyWord ();
int  FontPart ();
void HexEDeCrypt ();
void *getmem ();
int  GetNum ();
int  GetToken ();
int  GetWord ();
int  GetZeroLine ();
unsigned
char *itoasp ();
void LastLook ();
ub4  little4 ();
void OutASCII ();
void OutHEX ();
void OutStr();
void NameOfProgram ();
int  PartialPFA ();
int  PartialPFB ();
void PrintChar ();
int  PassString ();
int  PassToken ();
void Reverse();
int  ScanBinary ();
void ScanChars ();
void ScanSubrs ();
void SubstNum ();
void ViewReturnCall();
struct Char * UnDefineChars ();
void UnDefineCharsW();
void UnDefineStr();
int WorkVect();
#endif

#define NUM_LABEL     1024
#define BASE_MEM     16384
#define ADD_MEM      16384

#define FLG_LOAD_BASE   (1)

extern unsigned char grid[];
extern unsigned char *line, *tmpline ;
extern int loadbase ;
extern struct Char *FirstCharB;
