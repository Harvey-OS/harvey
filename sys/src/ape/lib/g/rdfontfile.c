#include <stdlib.h>
#include <unistd.h>
#include <libg.h>

Font *
rdfontfile(int fd, Bitmap *b)
{
	char hdr[3*12+1];
	int j, n;
	unsigned char *buf, *p;
	Fontchar *info, *i;
	Font *f;

	if(read(fd, hdr, 3*12) != 3*12)
		berror("rdfontfile read");
	n = atoi(hdr);
	buf = malloc(6*(n+1));
	if(buf == 0)
		berror("rdfontfile malloc");
	if(read(fd, (char *)buf, 6*(n+1)) != 6*(n+1)){
		free(buf);
		berror("rdfontfile read");
	}
	info = malloc(sizeof(Fontchar)*(n+1));
	if(info == 0){
		free(buf);
		berror("rdfontfile malloc");
	}
	for(p=buf,i=info,j=0; j<=n; j++,i++,p+=6){
		i->x = BGSHORT(p);
		i->top = p[2];
		i->bottom = p[3];
		i->left = p[4];
		i->width = p[5];
	}
	f = falloc(n, atoi(hdr+12), atoi(hdr+24), info, b);
	if(f == 0)
		free(info);
	free(buf);
	return f;
}
