#include <alef.h>

int
dirread(int f, Dir *dbuf, int count)
{
	byte *buf;
	int c, n, i, r;

	n = 0;
	buf = malloc(50*DIRLEN);
	if(buf == nil)
		return -1;
	count = (count/sizeof(Dir)) * DIRLEN;
	while(n < count) {
		c = count - n;
		if(c > 50*DIRLEN)
			c = 50*DIRLEN;
		r = read(f, buf, c);
		if(r == 0)
			break;
		if(r < 0 || r % DIRLEN){
			free(buf);
			return -1;
		}
		for(i=0; i<r; i+=DIRLEN) {
			convM2D(buf+i, dbuf);
			dbuf++;
		}
		n += r;
		if(r != c)
			break;
	}
	free(buf);
	return (n/DIRLEN) * sizeof(Dir);
}
