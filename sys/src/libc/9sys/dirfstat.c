#include <u.h>
#include <libc.h>
#include <fcall.h>

enum
{
	DIRSIZE	= STATFIXLEN + 16 * 4		/* enough for encoded stat buf + some reasonable strings */
};

Dir*
dirfstat(int fd)
{
	Dir *d;
	uchar *buf;
	int n, nd, i;

	nd = DIRSIZE;
	for(i=0; i<2; i++){	/* should work by the second try */
		d = malloc(sizeof(Dir) + nd);
		if(d == nil)
			return nil;
		buf = (uchar*)&d[1];
		n = fstat(fd, buf, nd);
		if(n < BIT16SZ){
			free(d);
			return nil;
		}
		nd = GBIT16(buf);	/* size needed to store whole stat buffer */
		if(nd <= n){
			convM2D(buf, n, d, (char*)&d[1]);
			return d;
		}
		/* else sizeof(Dir)+nd is plenty */
		free(d);
	}
	return nil;
}
