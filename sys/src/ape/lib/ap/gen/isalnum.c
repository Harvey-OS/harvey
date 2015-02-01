/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include <ctype.h>

#undef isalnum
#undef isalpha
#undef iscntrl
#undef isdigit
#undef isgraph
#undef islower
#undef isprint
#undef ispunct
#undef isspace
#undef isupper
#undef isxdigit
int isalnum(int c){ return (_ctype+1)[c]&(_ISupper|_ISlower|_ISdigit); }
int isalpha(int c){ return (_ctype+1)[c]&(_ISupper|_ISlower); }
int iscntrl(int c){ return (_ctype+1)[c]&_IScntrl; }
int isdigit(int c){ return (_ctype+1)[c]&_ISdigit; }
int isgraph(int c){ return (_ctype+1)[c]&(_ISpunct|_ISupper|_ISlower|_ISdigit); }
int islower(int c){ return (_ctype+1)[c]&_ISlower; }
int isprint(int c){ return (_ctype+1)[c]&(_ISpunct|_ISupper|_ISlower|_ISdigit|_ISblank); }
int ispunct(int c){ return (_ctype+1)[c]&_ISpunct; }
int isspace(int c){ return (_ctype+1)[c]&_ISspace; }
int isupper(int c){ return (_ctype+1)[c]&_ISupper; }
int isxdigit(int c){ return (_ctype+1)[c]&_ISxdigit; }
