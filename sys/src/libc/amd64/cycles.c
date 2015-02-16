#include <u.h>
#include <libc.h>

void _cycles(uvlong *x)
{
        uint32_t a, d;

        asm __volatile__ ("rdtsc" : "=a" (a), "=d" (d));

}

