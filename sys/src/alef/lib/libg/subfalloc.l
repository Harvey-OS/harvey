#include <alef.h>
#include <libg.h>

Subfont*
subfalloc(int n, int height, int ascent, Fontchar *info, Bitmap *b, uint q1, uint q2)
{
	int id, i;
	Subfont *f;
	byte *buf, *p, xbuf[3];

	bneed(0);	/* flush so we can get allocs right */
	buf = bneed(15+6*(n+1));

	buf[0] = 'k';
	BPSHORT(buf+1, n);
	buf[3] = height;
	buf[4] = ascent;
	BPSHORT(buf+5, b->id);
	BPLONG(buf+7, q1);
	BPLONG(buf+11, q2);
	p = buf+15;

	for(i=0; i<=n; i++){
		BPSHORT(p, info->x);
		p[2] = info->top;
		p[3] = info->bottom;
		p[4] = info->left;
		p[5] = info->width;
		info++;
		p += 6;
	}

	info -= n+1;
	if(!bwrite())
		return nil; /* non-fatal out of fonts case */

	if(read(bitbltfd, xbuf, 3)!=3 || xbuf[0]!='K')
		berror("falloc read");
	id = xbuf[1] | (xbuf[2]<<8);

	f = malloc(sizeof(Subfont));

	if(f == nil){
		xbuf[0] = 'g';
		write(bitbltfd, xbuf, 3);
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
	byte *buf;

	buf = bneed(3);
	buf[0] = 'g';
	buf[1] = f->id;
	buf[2] = f->id>>8;
	if(f->info)
		free(f->info);	/* note: f->info must have been malloc'ed! */
	free(f);
	bneed(0);	/* make sure resources are freed */
}
