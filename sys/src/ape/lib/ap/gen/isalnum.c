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
