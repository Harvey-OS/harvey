#ifndef __SETJMP_H
#define __SETJMP_H
#pragma lib "/$M/lib/ape/libap.a"

typedef int jmp_buf[10];
#ifdef _POSIX_SOURCE
typedef int sigjmp_buf[10];
#endif

#ifdef __cplusplus
extern "C" {
#endif

extern int setjmp(jmp_buf);
extern void longjmp(jmp_buf, int);

#ifdef _POSIX_SOURCE
extern int sigsetjmp(sigjmp_buf, int);
extern void siglongjmp(sigjmp_buf, int);
#endif

#ifdef __cplusplus
}
#endif

#endif /* __SETJMP_H */
