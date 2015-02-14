#include <u.h>
#include <libc.h>

void _cycles(uvlong *x)
{
        ulong a, d;

        asm __volatile__ ("rdtsc" : "=a" (a), "=d" (d));

}

