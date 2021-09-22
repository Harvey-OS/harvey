#include <u.h>
#include <libc.h>

void
main(void)
{
	int n, j;
	
	n = 0;
	for(int i=0; i<10; i++)
		n += i;
	for(j=0; j<10; j++)
		n += j;
	int k = 10;
	n += k;
	print("%d\n", n);
}
