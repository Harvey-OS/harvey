#include <u.h>
#include <libc.h>
#include <libg.h>

void
_unpackinfo(Fontchar *i, uchar *p, int n)
{
	int j;

	for(j=0; j<=n; j++,i++,p+=6){
		i->x = BGSHORT(p);
		i->top = p[2];
		i->bottom = p[3];
		i->left = p[4];
		i->width = p[5];
	}
}

Subfont*
rdsubfontfile(int fd, Bitmap *b)
{
	char hdr[3*12+4+1];
	int l, n, height, ascent;
	uchar *p;
	Fontchar *info;
	Subfont *f;
	Dir d;
	int id;
	ulong q1, q2;

	q1 = ~0;
	q2 = ~0;
	if(b == 0){
		if(dirfstat(fd, &d) < 0)
			return 0;
		q1 = d.qid.path;
		q2 = d.qid.vers;
		bflush();
		p = bneed(9);
		p[0] = 'j';
		BPLONG(p+1, q1);
		BPLONG(p+5, q2);
		if(!bwrite()){
			b = rdbitmapfile(fd);
			if(b == 0)
				return 0;
			goto uncached;
		}
		p = _btmp;
		l = read(bitbltfd, p, sizeof _btmp);
		if(l < 2+3*12)
			berror("rdsubfontfile read 1");
		if(p[0] != 'J')
			berror("rdsubfontfile protocol error");
		id = BGSHORT(p+1);
		p += 3;
		n = atoi((char*)p);
		height = atoi((char*)p+12);
		ascent = atoi((char*)p+24);
		info = malloc(sizeof(Fontchar)*(n+1));
		if(info == 0)
			berror("rdsubfontfile malloc");
		_unpackinfo(info, p+36, n);
		f = malloc(sizeof(Subfont));
		if(f == 0){	/* oh bother */
			_btmp[0] = 'g';
			BPSHORT(_btmp+1, id);
			write(bitbltfd, _btmp, 3);
			berror("rdsubfontfile malloc");
		}
		f->n = n;
		f->height = height;
		f->ascent = ascent;
		f->info = info;
		f->id = id;
		return f;
	}

    uncached:
	if(read(fd, hdr, 3*12) != 3*12)
		berror("rdsubfontfile read 2");
	n = atoi(hdr);
	if(6*(n+1) > sizeof _btmp)
		berror("subfont too large");
	if(read(fd, _btmp, 6*(n+1)) != 6*(n+1))
		berror("rdsubfontfile read 3");
	info = malloc(sizeof(Fontchar)*(n+1));
	if(info == 0)
		berror("rdsubfontfile malloc");
	_unpackinfo(info, _btmp, n);
	f = subfalloc(n, atoi(hdr+12), atoi(hdr+24), info, b, q1, q2);
	if(f == 0)
		free(info);
	return f;
}
