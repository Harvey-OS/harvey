#include <stdlib.h>
#include <unistd.h>
#include <libg.h>

Font*
falloc(int n, int height, int ascent, Fontchar *info, Bitmap *b)
{
	int id, i;
	Font *f;
	unsigned char *buf, *p, xbuf[3];

	bneed(0);	/* flush so we can get allocs right */
	buf = bneed(7+6*(n+1));
	buf[0] = 'k';
	BPSHORT(buf+1, n);
	buf[3] = height;
	buf[4] = ascent;
	BPSHORT(buf+5, b->id);
	for(p=buf+7,i=0; i<=n; i++,info++,p+=6){
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
	f = malloc(sizeof(Font));
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
	return f;
}

void
ffree(Font *f)
{
	unsigned char *buf;

	buf = bneed(3);
	buf[0] = 'g';
	buf[1] = f->id;
	buf[2] = f->id>>8;
	free(f);
	bneed(0);	/* make sure resources are freed */
}
