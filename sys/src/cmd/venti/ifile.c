#include "stdinc.h"
#include "dat.h"
#include "fns.h"

int
readIFile(IFile *f, char *name)
{
	ZBlock *b;

	b = readFile(name);
	if(b == nil)
		return 0;
	f->name = name;
	f->b = b;
	f->pos = 0;
	return 1;
}

void
freeIFile(IFile *f)
{
	freeZBlock(f->b);
	f->b = nil;
	f->pos = 0;
}

int
partIFile(IFile *f, Part *part, u64int start, u32int size)
{
	ZBlock *b;

	b = allocZBlock(size, 0);
	if(b == nil)
		return 0;
	if(!readPart(part, start, b->data, size)){
		setErr(EAdmin, "can't read %s: %R", part->name);
		freeZBlock(b);
		return 0;
	}
	f->name = part->name;
	f->b = b;
	f->pos = 0;
	return 1;
}

/*
 * return the next non-blank input line,
 * stripped of leading white space and with # comments eliminated
 */
char*
ifileLine(IFile *f)
{
	char *s, *e, *t;
	int c;

	for(;;){
		s = (char*)&f->b->data[f->pos];
		e = memchr(s, '\n', f->b->len - f->pos);
		if(e == nil)
			return nil;
		*e++ = '\0';
		f->pos = e - (char*)f->b->data;
		t = strchr(s, '#');
		if(t != nil)
			*t = '\0';
		for(; c = *s; s++)
			if(c != ' ' && c != '\t' && c != '\r')
				return s;
	}
}

int
ifileName(IFile *f, char *dst)
{
	char *s;

	s = ifileLine(f);
	if(s == nil || strlen(s) >= ANameSize)
		return 0;
	nameCp(dst, s);
	return 1;
}

int
ifileU32Int(IFile *f, u32int *r)
{
	char *s;

	s = ifileLine(f);
	if(s == nil)
		return 0;
	return strU32Int(s, r);
}
