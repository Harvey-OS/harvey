#include "lib.h"
#include <string.h>
#include "sys9.h"
#include "dir.h"

#define	CHAR(x)		*p++ = f->x
#define	SHORT(x)	p[0] = f->x; p[1] = f->x>>8; p += 2
#define	LONG(x)		p[0] = f->x; p[1] = f->x>>8; p[2] = f->x>>16; p[3] = f->x>>24; p += 4
#define	VLONG(x)	p[0] = f->x; p[1] = f->x>>8; p[2] = f->x>>16; p[3] = f->x>>24;\
			p[4] = 0; p[5] = 0; p[6] = 0; p[7] = 0; p += 8
#define	STRING(x,n)	memcpy(p, f->x, n); p += n

int
convD2M(Dir *f, char *ap)
{
	unsigned char *p;

	p = (unsigned char*)ap;
	STRING(name, sizeof(f->name));
	STRING(uid, sizeof(f->uid));
	STRING(gid, sizeof(f->gid));
	LONG(qid.path);
	LONG(qid.vers);
	LONG(mode);
	LONG(atime);
	LONG(mtime);
	VLONG(length);
	SHORT(type);
	SHORT(dev);
	return p - (unsigned char*)ap;
}

#undef	CHAR
#undef	SHORT
#undef	LONG
#undef	VLONG
#undef	STRING

#define	CHAR(x)		f->x = *p++
#define	SHORT(x)	f->x = (p[0] | (p[1]<<8)); p += 2
#define	LONG(x)		f->x = (p[0] | (p[1]<<8) |\
			(p[2]<<16) | (p[3]<<24)); p += 4
#define	VLONG(x)	f->x = (p[0] | (p[1]<<8) |\
			(p[2]<<16) | (p[3]<<24)); p += 8
#define	STRING(x,n)	memcpy(f->x, p, n); p += n

int
convM2D(char *ap, Dir *f)
{
	unsigned char *p;

	p = (unsigned char*)ap;
	STRING(name, sizeof(f->name));
	STRING(uid, sizeof(f->uid));
	STRING(gid, sizeof(f->gid));
	LONG(qid.path);
	LONG(qid.vers);
	LONG(mode);
	LONG(atime);
	LONG(mtime);
	VLONG(length);
	SHORT(type);
	SHORT(dev);
	return p - (unsigned char*)ap;
}
