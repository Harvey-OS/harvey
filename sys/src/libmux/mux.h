/*
 * file system multiplexer
 */
#define	GET_CHAR(x)	f->x = *p++
#define	GET_SHORT(x)	f->x = (p[0] | (p[1]<<8)); p += 2
#define	GET_LONG(x)	f->x = (p[0] | (p[1]<<8) | (p[2]<<16) | (p[3]<<24)); p += 4
#define	VGET_LONG(x)	f->x = (p[0] | (p[1]<<8) | (p[2]<<16) | (p[3]<<24)); p += 8
#define	GET_STRING(x,n)	memmove(f->x, p, n); p += n
#define	PUT_CHAR(x)	*p++ = f->x
#define	PUT_SHORT(x)	p[0] = f->x; p[1] = f->x>>8; p += 2
#define PUT_2DATA(x)	p[0] = x; p[1] = x>>8; p += 2;
#define	PUT_LONG(x)	p[0] = f->x; p[1] = f->x>>8; p[2] = f->x>>16; p[3] = f->x>>24; p += 4
#define	VPUT_LONG(x)	p[0] = f->x; p[1] = f->x>>8; p[2] = f->x>>16; p[3] = f->x>>24;\
				p[4] = 0; p[5] = 0; p[6] = 0; p[7] = 0; p += 8
#define	PUT_STRING(x,n)	memmove(p, f->x, n); p += n

typedef struct Pfid Pfid;
typedef struct Tag Tag;

struct Pfid
{
	ushort	clientfid;
	ushort	serverfid;
	int	mux;
	Pfid	*free;
	Pfid	*hashclient;
	Pfid	*hashserver;
};

struct	Tag
{
	ushort	tag;
	ushort	clienttag;
	ushort	mux;
	Tag	*hash;
	Tag	*free;
};

enum
{
	Fidstart	= 1,
	Tagstart	= 1,
	Hashsize	= 128,
};

#define Hash(fid)	((fid*11)%Hashsize)

extern int _mountismux;

ushort	mapM2S(ushort, ushort);
ushort	mapS2M(ushort, int);
ushort	tagM2S(ushort, ushort);
ushort	tagS2M(ushort, ushort*, int);
