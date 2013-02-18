#include <u.h>
#include <libc.h>
#include <fcall.h>

enum
{
	DIRSIZE	= STATFIXLEN + 16 * 4		/* enough for encoded stat buf + some reasonable strings */
};

Dir*
dirstat(char *name)
{
	Dir *d;
	uchar *buf;
	int n, nd, i;

	nd = DIRSIZE;
	for(i=0; i<2; i++){	/* should work by the second try */
		d = malloc(sizeof(Dir) + BIT16SZ + nd);
		if(d == nil)
			return nil;
		buf = (uchar*)&d[1];
		n = stat(name, buf, BIT16SZ+nd);
		if(n < BIT16SZ){
			free(d);
			return nil;
		}
		nd = GBIT16((uchar*)buf);	/* upper bound on size of Dir + strings */
		if(nd <= n){
			convM2D(buf, n, d, (char*)&d[1]);
			return d;
		}
		/* else sizeof(Dir)+BIT16SZ+nd is plenty */
		free(d);
	}
	return nil;
}
