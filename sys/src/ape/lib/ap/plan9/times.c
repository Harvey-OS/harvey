#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/times.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

static
char*
skip(char *p)
{

	while(*p == ' ')
		p++;
	while(*p != ' ' && *p != 0)
		p++;
	return p;
}

clock_t
times(struct tms *buf)
{
	char b[200], *p;
	static int f = -1;
	unsigned long r;

	memset(b, 0, sizeof(b));
	if(f < 0)
		f = open("/dev/cputime", O_RDONLY);
	if(f >= 0) {
		lseek(f, SEEK_SET, 0);
		read(f, b, sizeof(b));
	}
	p = b;
	if(buf)
		buf->tms_utime = atol(p);
	p = skip(p);
	if(buf)
		buf->tms_stime = atol(p);
	p = skip(p);
	r = atol(p);
	if(buf){
		p = skip(p);
		buf->tms_cutime = atol(p);
		p = skip(p);
		buf->tms_cstime = atol(p);
	}
	return r;
}
