#include <alef.h>
#include <graphics.h>

#define	CHUNK	2048		/* >> 6*256 */

void
wrsubfontfile(int fd, Subfont *f)
{
	char hdr[3*12+1];
	char *data;
	int j;
	char *p;
	Fontchar *i;

	sprint(hdr, "%11d %11d %11d ", f->n, f->height, f->ascent);
	if(write(fd, hdr, 3*12) != 3*12)
		berror("wrsubfontfile write");

	data = malloc(6*(f->n+1));
	if(data == nil)
		berror("wrsubfontfile malloc");
	p=data;
	i=f->info;
	for(j=0; j<=f->n; j++){
		BPSHORT(p, i->x);
		p[2] = i->top;
		p[3] = i->bottom;
		p[4] = i->left;
		p[5] = i->width;
		i++;
		p+=6;
	}

	if(write(fd, data, p-data) != p-data){
		free(data);
		berror("wrsubfontfile write");
	}
	free(data);
}
