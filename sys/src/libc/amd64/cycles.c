#include <u.h>
#include <libc.h>

void
_cycles(uint64_t* x)
{
	uint32_t a, d;

	asm __volatile__("rdtsc" : "=a"(a), "=d"(d));
}
