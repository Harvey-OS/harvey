#include "stdinc.h"
#include "dat.h"
#include "fns.h"

int
readIFile(IFile *f, char *name)
{
	Part *p;
	ZBlock *b;
	int m;
	u8int *z;

	p = initPart(name, 1);
	if(p == nil)
		return 0;
	b = allocZBlock(8192, 1);
	if(b == nil){
		setErr(EOk, "can't alloc %s: %R", name);
		goto err;
	}
	if(p->size > PartBlank){
		/*
		 * this is likely a real venti partition, in which case
		 * we're looking for the config file stored as 8k at end of PartBlank.
		 */
		if(!readPart(p, PartBlank-8192, b->data, 8192)){
			setErr(EOk, "can't read %s: %R", name);
			goto err;
		}
		m = 5+1+6+1;
		if(memcmp(b->data, "venti config\n", m) != 0){
			setErr(EOk, "bad venti config magic in %s", name);
			goto err;
		}
		b->data += m;
		b->len -= m;
		z = memchr(b->data, 0, b->len);
		if(z)
			b->len = z - b->data;
	}else if(p->size > 8192){
		setErr(EOk, "config file is too large");
		freePart(p);
		freeZBlock(b);
		return 0;
	}else{
		if(!readPart(p, 0, b->data, p->size)){
			setErr(EOk, "can't read %s: %R", name);
			goto err;
		}
		b->len = p->size;
	}
	f->name = name;
	f->b = b;
	f->pos = 0;
	return 1;

err:
	freePart(p);
	if(b)
		freeZBlock(b);
	return 0;
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
