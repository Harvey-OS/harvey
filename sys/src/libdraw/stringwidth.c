/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include <u.h>
#include <libc.h>
#include <draw.h>

static Rune empty[] = { 0 };

int
_stringnwidth(Font *f, char *s, Rune *r, int len)
{
	int wid, twid, n, max, l;
	char *name;
	enum { Max = 64 };
	uint16_t cbuf[Max];
	Rune rune, **rptr;
	char *subfontname, **sptr;
	Font *def;

	if(s == nil){
		s = "";
		sptr = nil;
	}else
		sptr = &s;
	if(r == nil){
		r = empty;
		rptr = nil;
	}else
		rptr = &r;
	twid = 0;
	while(len>0 && (*s || *r)){
		max = Max;
		if(len < max)
			max = len;
		n = 0;
		while((l = cachechars(f, sptr, rptr, cbuf, max, &wid, &subfontname)) <= 0){
			if(++n > 10){
				if(*r)
					rune = *r;
				else
					chartorune(&rune, s);
				if(f->name != nil)
					name = f->name;
				else
					name = "unnamed font";
				fprint(2, "stringwidth: bad character set for rune 0x%.4x in %s\n", rune, name);
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
