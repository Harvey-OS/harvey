#include <u.h>
#include <libc.h>

void
rdpmc (int counter, int low, int high)
{
     asm __volatile__("rdpmc" : "=a" (low), "=d" (high) : "c" (counter)) ;
}

