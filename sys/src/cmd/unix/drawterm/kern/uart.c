#include "u.h"
#include "lib.h"
#include "dat.h"
#include "fns.h"
#include "error.h"

extern int panicking;
void
uartputs(char *s, int n)
{
	if(panicking)
		write(1, s, n); 
}


