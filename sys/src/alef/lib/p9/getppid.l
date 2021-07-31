#include <alef.h>

int
getppid(void)
{
	int f;
	byte b[20];

	memset(b, 0, sizeof(b));
	f = open("#c/ppid", 0);
	if(f >= 0) {
		read(f, b, sizeof(b));
		close(f);
	}
	return atoi(b);
}
