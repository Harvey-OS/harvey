#include <u.h>
#include <libc.h>

/*
 * Buffer until eof, output with a single write.
 * If write is larger than 8K just put out the first 8K.
 * (Should do better, some day.)
 */

void
main(void)
{
	char buf[8192];
	int i, n;

	for(n=0; n<sizeof buf; n+=i){
		i = read(0, buf+n, sizeof buf-n);
		if(i <= 0)
			break;
	}
	write(1, buf, n);
}
