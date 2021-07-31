#include	<u.h>
#include	<libc.h>
#include	<auth.h>
#include	<fcall.h>

#define	CHAR(x)		*p++ = f->x
#define	SHORT(x)	p[0]=f->x; p[1]=f->x>>8; p += 2
#define	LONG(x)		p[0]=f->x; p[1]=f->x>>8; p[2]=f->x>>16; p[3]=f->x>>24; p += 4
#define	VLONG(x)	p[0]=f->x;	p[1]=f->x>>8;\
			p[2]=f->x>>16;	p[3]=f->x>>24;\
			p[4]=f->x>>32;	p[5]=f->x>>40;\
			p[6]=f->x>>48;	p[7]=f->x>>56;\
			p += 8
#define	STRING(x,n)	memmove(p, f->x, n); p += n

int
convD2M(Dir *f, char *ap)
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
