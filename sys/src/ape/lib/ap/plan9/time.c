#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

time_t
time(time_t *tp)
{
	char b[20];
	int f;
	time_t t;

	memset(b, 0, sizeof(b));
	f = open("/dev/time", O_RDONLY);
	if(f >= 0) {
		lseek(f, 0, 0);
		read(f, b, sizeof(b));
		close(f);
	}
	t = atol(b);
	if(tp)
		*tp = t;
	return t;
}
