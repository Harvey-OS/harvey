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
int isalnum(int c){ return (_ctype+1)[c]&(__U|__L|__N); }
int isalpha(int c){ return (_ctype+1)[c]&(__U|__L); }
int iscntrl(int c){ return (_ctype+1)[c]&__C; }
int isdigit(int c){ return (_ctype+1)[c]&__N; }
int isgraph(int c){ return (_ctype+1)[c]&(__P|__U|__L|__N); }
int islower(int c){ return (_ctype+1)[c]&__L; }
int isprint(int c){ return (_ctype+1)[c]&(__P|__U|__L|__N|__B); }
int ispunct(int c){ return (_ctype+1)[c]&__P; }
int isspace(int c){ return (_ctype+1)[c]&__S; }
int isupper(int c){ return (_ctype+1)[c]&__U; }
int isxdigit(int c){ return (_ctype+1)[c]&__X; }
