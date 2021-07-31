#include	<alef.h>

int
avg(int n, ...)
{
	int i, tot;

	tot = 0;
	for(i = 0; i < n; i++)
		tot += ((int*)...)[i];
	return tot/n;
}

void
main(void)
{
	print("expect 29 ... %d\n", avg(5, 10, 20, 66, 19, 33));
}
