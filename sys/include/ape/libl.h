#ifndef __LIBL_H
#define __LIBL_H
#ifndef _RESEARCH_SOURCE
   This header file is not defined in ANSI or POSIX
#endif
#pragma lib "/$M/lib/ape/libl.a"

extern int printable(int);
extern void allprint(char);
extern int yyracc(int);
extern int yyreject(void);
extern void yyless(int);
extern int yywrap(void);

#endif /* __LIBV_L */
