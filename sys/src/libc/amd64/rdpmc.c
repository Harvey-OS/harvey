#include <u.h>
#include <libc.h>

void
rdpmc (int counter, int low, int high)
{
     __asm__ __volatile__("rdpmc" : "=a" (low), "=d" (high) : "c" (counter)) ;
}

