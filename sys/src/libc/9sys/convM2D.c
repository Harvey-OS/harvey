#include	<u.h>
#include	<libc.h>
#include	<auth.h>
#include	<fcall.h>

#define	CHAR(x)		f->x = *p++
#define	SHORT(x)	f->x = (p[0] | (p[1]<<8)); p += 2
#define	LONG(x)		f->x = (p[0] | (p[1]<<8) |\
				(p[2]<<16) | (p[3]<<24)); p += 4
#define	VLONG(x)	f->x = (vlong)(p[0] | (p[1]<<8) |\
					(p[2]<<16) | (p[3]<<24)) |\
				((vlong)(p[4] | (p[5]<<8) |\
					(p[6]<<16) | (p[7]<<24)) << 32); p += 8
#define	STRING(x,n)	memmove(f->x, p, n); p += n

int
convM2D(char *ap, Dir *f)
{
	uchar *p;

	p = (uchar*)ap;
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
	return p - (uchar*)ap;
}
