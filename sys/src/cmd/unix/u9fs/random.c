#include <plan9.h>
#include <fcall.h>
#include <u9fs.h>
#include <stdlib.h>
#include <sys/time.h>
#include <fcntl.h>

static long
getseed(void)
{
	struct timeval tv;
	long seed;
	int fd, len;

	len = 0;
	fd = open("/dev/urandom", O_RDONLY);
	if(fd > 0){
		len = readn(fd, &seed, sizeof(seed));
		close(fd);
	}
	if(len != sizeof(seed)){
		gettimeofday(&tv, nil);
		seed = tv.tv_sec ^ tv.tv_usec ^ (getpid()<<8);
	}
	return seed;
}

static int seeded;

void
randombytes(uchar *r, uint nr)
{
	int i;
	ulong l;

	if(!seeded){
		seeded=1;
		srand48(getseed());
	}
	for(i=0; i+4<=nr; i+=4,r+=4){
		l = (ulong)mrand48();
		r[0] = l;
		r[1] = l>>8;
		r[2] = l>>16;
		r[3] = l>>24;
	}
	if(i<nr){
		l = (ulong)mrand48();
		switch(nr-i){
		case 3:
			r[2] = l>>16;
		case 2:
			r[1] = l>>8;
		case 1:
			r[0] = l;
		}
	}
}
