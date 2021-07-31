#include <u.h>
#include <libc.h>
#include <draw.h>

int
_stringnwidth(Font *f, char *s, Rune *r, int len)
{
	int wid, twid, n, max, l;
	enum { Max = 64 };
	ushort cbuf[Max];
	Rune **rptr;
	char *subfontname, **sptr;
	Font *def;

	if(s == nil){
		s = "";
		sptr = nil;
	}else
		sptr = &s;
	if(r == nil){
		r = L"";
		rptr = nil;
	}else
		rptr = &r;
	twid = 0;
	while((*s || *r)&& len){
		max = Max;
		if(len < max)
			max = len;
		n = 0;
		while((l = cachechars(f, sptr, rptr, cbuf, max, &wid, &subfontname)) <= 0){
			if(++n > 10){
				_drawprint(2, "stringwidth: bad character set\n");
				return twid;
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
		agefont(f);
		twid += wid;
		len -= l;
	}
	return twid;
}

int
stringnwidth(Font *f, char *s, int len)
{
	return _stringnwidth(f, s, nil, len);
}

int
stringwidth(Font *f, char *s)
{
	return _stringnwidth(f, s, nil, 1<<24);
}

Point
stringsize(Font *f, char *s)
{
	return Pt(_stringnwidth(f, s, nil, 1<<24), f->height);
}

int
runestringnwidth(Font *f, Rune *r, int len)
{
	return _stringnwidth(f, nil, r, len);
}

int
runestringwidth(Font *f, Rune *r)
{
	return _stringnwidth(f, nil, r, 1<<24);
}

Point
runestringsize(Font *f, Rune *r)
{
	return Pt(_stringnwidth(f, nil, r, 1<<24), f->height);
}
