#include <u.h>
#include <libc.h>

void _cycles(u64 *x)
{
        u32 a, d;

        __asm__ __volatile__ ("rdtsc" : "=a" (a), "=d" (d));

}
