#include <u.h>
#include <libc.h>
#include <draw.h>

enum
{
	Max = 100
};

static
void
drawchar(Image *dst, Point p, Font *font, int index)
{
	Rectangle r;
	Point sp;

	r.min.x = p.x+font->cache[index].left;
	r.min.y = p.y;
	r.max.x = r.min.x+font->width;
	r.max.y = r.min.y+font->height;
	sp.x = font->cache[index].x;
	sp.y = 0;
	draw(dst, r, font->cacheimage, font->cacheimage, sp);
}

Point
greystring(Image *dst, Point pt, Image *src, Point sp, Font *f, char *s)
{
	int n, i, wid, max;
	ushort cbuf[Max];
	char *subfontname;
	char **sptr;
	Font *def;
	int len;

	USED(src);
	USED(sp);
	len = strlen(s);
	sptr = &s;
	while(*s){
		max = Max;
		if(len < max)
			max = len;
		n = cachechars(f, sptr, nil, cbuf, max, &wid, &subfontname);
		if(n > 0){
			for(i=0; i<n; i++){
				drawchar(dst, pt, f, cbuf[i]);
				pt.x += f->cache[cbuf[i]].width;
			}
			agefont(f);
			len -= n;
		}
		if(subfontname){
			if(_getsubfont(f->display, subfontname) == 0){
				def = f->display->defaultfont;
				if(def && f!=def)
					f = def;
				else
					break;
			}
		}
	}
	return pt;
}
