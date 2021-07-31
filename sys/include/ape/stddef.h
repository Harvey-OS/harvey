#ifndef __STDDEF_H
#define __STDDEF_H

#ifndef NULL
#define NULL 0
#endif
#define offsetof(ty,mem) ((size_t) &(((ty *)0)->mem))

typedef long ptrdiff_t;
#ifndef _SIZE_T
#define _SIZE_T
typedef unsigned long size_t;
#endif
#ifndef _WCHAR_T
#define _WCHAR_T
typedef unsigned short wchar_t;
#endif

#endif /* __STDDEF_H */
