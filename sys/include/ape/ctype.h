#ifndef __CTYPE
#define __CTYPE
#pragma lib "/$M/lib/ape/libap.a"

#ifdef __cplusplus
extern "C" {
#endif

extern int isalnum(int);
extern int isalpha(int);
extern int iscntrl(int);
extern int isdigit(int);
extern int isgraph(int);
extern int islower(int);
extern int isprint(int);
extern int ispunct(int);
extern int isspace(int);
extern int isupper(int);
extern int isxdigit(int);
extern int tolower(int);
extern int toupper(int);

#ifdef __cplusplus
}
#endif
enum
{
  _ISupper = 01,	/* UPPERCASE.  */
  _ISlower = 02,	/* lowercase.  */
  _ISdigit = 04,	/* Numeric.  */
  _ISspace = 010,	/* Whitespace.  */
  _ISpunct = 020,	/* Punctuation.  */
  _IScntrl = 040,	/* Control character.  */
  _ISblank = 0100,	/* Blank (usually SPC and TAB).  */
  _ISxdigit = 0200,	/* Hexadecimal numeric.  */
};

extern unsigned char _ctype[];
#define	isalnum(c)	(_ctype[(unsigned char)(c)]&(_ISupper|_ISlower|_ISdigit))
#define	isalpha(c)	(_ctype[(unsigned char)(c)]&(_ISupper|_ISlower))
#define	iscntrl(c)	(_ctype[(unsigned char)(c)]&_IScntrl)
#define	isdigit(c)	(_ctype[(unsigned char)(c)]&_ISdigit)
#define	isgraph(c)	(_ctype[(unsigned char)(c)]&(_ISpunct|_ISupper|_ISlower|_ISdigit))
#define	islower(c)	(_ctype[(unsigned char)(c)]&_ISlower)
#define	isprint(c)	(_ctype[(unsigned char)(c)]&(_ISpunct|_ISupper|_ISlower|_ISdigit|_ISblank))
#define	ispunct(c)	(_ctype[(unsigned char)(c)]&_ISpunct)
#define	isspace(c)	(_ctype[(unsigned char)(c)]&_ISspace)
#define	isupper(c)	(_ctype[(unsigned char)(c)]&_ISupper)
#define	isxdigit(c)	(_ctype[(unsigned char)(c)]&_ISxdigit)

#ifdef _BSD_EXTENSION
#define	isascii(c) (((unsigned int)(c))<0x80)
#endif

#endif /* __CTYPE */
