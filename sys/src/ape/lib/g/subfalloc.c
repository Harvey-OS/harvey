#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <libg.h>

typedef unsigned char uchar; 
typedef unsigned long ulong; 

Subfont*
subfalloc(int n, int height, int ascent, Fontchar *info, Bitmap *b, ulong q1, ulong q2)
{
	int id, i;
	Subfont *f;
	uchar *buf, *p, xbuf[3];

	bneed(0);	/* flush so we can get allocs right */
	buf = bneed(15+6*(n+1));
	buf[0] = 'k';
	BPSHORT(buf+1, n);
	buf[3] = height;
	buf[4] = ascent;
	BPSHORT(buf+5, b->id);
	BPLONG(buf+7, q1);
	BPLONG(buf+11, q2);
	for(p=buf+15,i=0; i<=n; i++,info++,p+=6){
		BPSHORT(p, info->x);
		p[2] = info->top;
		p[3] = info->bottom;
		p[4] = info->left;
		p[5] = info->width;
	}
	info -= n+1;
	if(!bwrite())
		return 0; /* non-fatal out of fonts case */
	if(read(bitbltfd, (char *)xbuf, 3)!=3 || xbuf[0]!='K')
		berror("falloc read");
	id = xbuf[1] | (xbuf[2]<<8);
	f = malloc(sizeof(Subfont));
	if(f == 0){	/* oh bother */
		xbuf[0] = 'g';
		write(bitbltfd, (char *)xbuf, 3);
		berror("falloc malloc");
	}
	f->n = n;
	f->height = height;
	f->ascent = ascent;
	f->info = info;
	f->id = id;
	free(b);
	return f;
}

void
subffree(Subfont *f)
{
	uchar *buf;

	buf = bneed(3);
	buf[0] = 'g';
	buf[1] = f->id;
	buf[2] = f->id>>8;
	if(f->info)
		free(f->info);	/* note: f->info must have been malloc'ed! */
	free(f);
	bneed(0);	/* make sure resources are freed */
}
