#include <u.h>
#include <libc.h>
#include <libg.h>

#define	CHUNK	2048		/* >> 6*256 */

void
wrsubfontfile(int fd, Subfont *f)
{
	char hdr[3*12+1];
	uchar *data;
	int j;
	uchar *p;
	Fontchar *i;

	sprint(hdr, "%11d %11d %11d ", f->n, f->height, f->ascent);
	if(write(fd, hdr, 3*12) != 3*12)
		berror("wrsubfontfile write");

	data = malloc(6*(f->n+1));
	if(data == 0)
		berror("wrsubfontfile malloc");
	for(p=data,i=f->info,j=0; j<=f->n; j++,i++,p+=6){
		BPSHORT(p, i->x);
		p[2] = i->top;
		p[3] = i->bottom;
		p[4] = i->left;
		p[5] = i->width;
	}

	if(write(fd, data, p-data) != p-data){
		free(data);
		berror("wrsubfontfile write");
	}
	free(data);
}
