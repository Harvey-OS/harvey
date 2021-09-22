#ifndef __SETJMP_H
#define __SETJMP_H
#pragma lib "/$M/lib/ape/libap.a"

#undef	_SUSV2_SOURCE
#define _SUSV2_SOURCE
#include <inttypes.h>

typedef uintptr_t jmp_buf[4];	/* need 4 pointers for sigsetjmp on riscv64 */
#ifdef _POSIX_SOURCE
typedef uintptr_t sigjmp_buf[4];
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
