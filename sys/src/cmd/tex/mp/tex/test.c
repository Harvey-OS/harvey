#include <stdio.h>

main()
{
	char buf[10000];

	memset(buf, -1, sizeof(buf));
	fwrite(buf, 1, sizeof(buf), stdout);
}
