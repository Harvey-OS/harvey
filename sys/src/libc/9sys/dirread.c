#include <u.h>
#include <libc.h>
#include <auth.h>
#include <fcall.h>

long
dirread(int f, Dir *dbuf, long count)
{
	char buf[DIRLEN*50];
	int c, n, i, r;

	n = 0;
	count = (count/sizeof(Dir)) * DIRLEN;
	while(n < count) {
		c = count - n;
		if(c > sizeof(buf))
			c = sizeof(buf);
		r = read(f, buf, c);
		if(r == 0)
			break;
		if(r < 0 || r % DIRLEN)
			return -1;
		for(i=0; i<r; i+=DIRLEN) {
			convM2D(buf+i, dbuf);
			dbuf++;
		}
		n += r;
		if(r != c)
			break;
	}
	return (n/DIRLEN) * sizeof(Dir);
}
