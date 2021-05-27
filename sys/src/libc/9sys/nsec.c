#include <u.h>
#include <libc.h>

#define	U32(x)	(((((((x)[0]<<8)|(x)[1])<<8)|(x)[2])<<8)|(x)[3])

vlong
nsec(void)
{
	uchar b[8];
	int f, n;

	if((f = open("/dev/bintime", OREAD)) >= 0){
		n = pread(f, b, sizeof(b), 0);
		close(f);
		if(n == sizeof(b))
			return (u64int)U32(b)<<32 | U32(b+4);
	}
	return 0;
}
