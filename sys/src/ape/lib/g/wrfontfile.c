#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <libg.h>

#define	CHUNK	2048		/* >> 6*256 */

void
wrfontfile(int fd, Font *f)
{
	char hdr[3*12+1];
	unsigned char *data;
	int j;
	unsigned char *p;
	Fontchar *i;

	sprintf(hdr, "%11d %11d %11d ", f->n, f->height, f->ascent);
	if(write(fd, hdr, 3*12) != 3*12)
		berror("wrfontfile write");

	data = malloc(6*(f->n+1));
	if(data == 0)
		berror("wrfontfile malloc");
	for(p=data,i=f->info,j=0; j<=f->n; j++,i++,p+=6){
		BPSHORT(p, i->x);
		p[2] = i->top;
		p[3] = i->bottom;
		p[4] = i->left;
		p[5] = i->width;
	}

	if(write(fd, (char *)data, p-data) != p-data){
		free(data);
		berror("wrfontfile write");
	}
	free(data);
}
