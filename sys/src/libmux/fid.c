#include <u.h>
#include <libc.h>
#include <fcall.h>
#include <mux.h>

static	Pfid	*pool;
static	Pfid	*clienthash[Hashsize];
static	Pfid	*serverhash[Hashsize];
static	int	fidalloc = Fidstart;
int	_mountismux;

/*
 * Map multiplex fid/mux to 16-bit fid space for the server
 */
ushort
mapM2S(ushort fid, ushort mux)
{
	Pfid *p, **h, **s;

	h = &clienthash[Hash(fid)];
	for(p = *h; p; p = p->hashclient)
		if(p->clientfid == fid && p->mux == mux)
			return p->serverfid;

	if(p = pool)
		pool = p->free;
	else {
		p = malloc(sizeof(Pfid));
		if(p == 0) {
			fprint(2, "mapM2S: no memory\n");
			exits("mapM2S");
		}
		memset(p, 0, sizeof(Pfid));
		p->serverfid = fidalloc++;
		s = &serverhash[Hash(p->serverfid)];
		p->hashserver = *s;
		*s = p;
	}
	p->clientfid = fid;
	p->mux = mux;
	p->hashclient = *h;
	*h = p;

	return p->serverfid;
}

ushort
mapS2M(ushort fid, int clunk)
{
	Pfid *p, **s, *f;

	for(p = serverhash[Hash(fid)]; p; p = p->hashserver) {
		if(p->serverfid == fid) {
			if(clunk) {
				s = &clienthash[Hash(p->clientfid)];
				for(f = *s; f; f = f->hashclient) {
					if(f == p) {
						*s = p->hashclient;
						break;
					}
					s = &p->hashclient;
				}
				p->free = pool;
				pool = p;
			}
			return p->clientfid;
		}
	}

	fprint(2, "mapS2M: unseen server fid %d\n", fid);
	exits("mapS2M");
}
