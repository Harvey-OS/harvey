/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

/*
 * functions to read and write an executable or file image
 */

#include <u.h>
#include <libc.h>
#include <bio.h>
#include <mach.h>

static	int	mget(Map*, u64, void*, int);
static	int	mput(Map*, u64, void*, int);
static	struct	segment*	reloc(Map*, u64, i64*);

/*
 * routines to get/put various types
 */
int
geta(Map *map, u64 addr, u64 *x)
{
	u32 l;
	u64 vl;

	if (mach->szaddr == 8){
		if (get8(map, addr, &vl) < 0)
			return -1;
		*x = vl;
		return 1;
	}

	if (get4(map, addr, &l) < 0)
		return -1;
	*x = l;

	return 1;
}

int
get8(Map *map, u64 addr, u64 *x)
{
	if (!map) {
		werrstr("get8: invalid map");
		return -1;
	}

	if (map->nsegs == 1 && map->seg[0].fd < 0) {
		*x = addr;
		return 1;
	}
	if (mget(map, addr, x, 8) < 0)
		return -1;
	*x = machdata->swav(*x);
	return 1;
}

int
get4(Map *map, u64 addr, u32 *x)
{
	if (!map) {
		werrstr("get4: invalid map");
		return -1;
	}

	if (map->nsegs == 1 && map->seg[0].fd < 0) {
		*x = addr;
		return 1;
	}
	if (mget(map, addr, x, 4) < 0)
		return -1;
	*x = machdata->swal(*x);
	return 1;
}

int
get2(Map *map, u64 addr, u16 *x)
{
	if (!map) {
		werrstr("get2: invalid map");
		return -1;
	}

	if (map->nsegs == 1 && map->seg[0].fd < 0) {
		*x = addr;
		return 1;
	}
	if (mget(map, addr, x, 2) < 0)
		return -1;
	*x = machdata->swab(*x);
	return 1;
}

int
get1(Map *map, u64 addr, u8 *x, int size)
{
	u8 *cp;

	if (!map) {
		werrstr("get1: invalid map");
		return -1;
	}

	if (map->nsegs == 1 && map->seg[0].fd < 0) {
		cp = (u8*)&addr;
		while (cp < (u8*)(&addr+1) && size-- > 0)
			*x++ = *cp++;
		while (size-- > 0)
			*x++ = 0;
	} else
		return mget(map, addr, x, size);
	return 1;
}

int
puta(Map *map, u64 addr, u64 v)
{
	if (mach->szaddr == 8)
		return put8(map, addr, v);

	return put4(map, addr, v);
}

int
put8(Map *map, u64 addr, u64 v)
{
	if (!map) {
		werrstr("put8: invalid map");
		return -1;
	}
	v = machdata->swav(v);
	return mput(map, addr, &v, 8);
}

int
put4(Map *map, u64 addr, u32 v)
{
	if (!map) {
		werrstr("put4: invalid map");
		return -1;
	}
	v = machdata->swal(v);
	return mput(map, addr, &v, 4);
}

int
put2(Map *map, u64 addr, u16 v)
{
	if (!map) {
		werrstr("put2: invalid map");
		return -1;
	}
	v = machdata->swab(v);
	return mput(map, addr, &v, 2);
}

int
put1(Map *map, u64 addr, u8 *v, int size)
{
	if (!map) {
		werrstr("put1: invalid map");
		return -1;
	}
	return mput(map, addr, v, size);
}

static int
spread(struct segment *s, void *buf, int n, u64 off)
{
	u64 base;

	static struct {
		struct segment *s;
		char a[8192];
		u64 off;
	} cache;

	if(s->cache){
		base = off&~(sizeof cache.a-1);
		if(cache.s != s || cache.off != base){
			cache.off = ~0;
			if(seek(s->fd, base, 0) >= 0
			&& readn(s->fd, cache.a, sizeof cache.a) == sizeof cache.a){
				cache.s = s;
				cache.off = base;
			}
		}
		if(cache.s == s && cache.off == base){
			off &= sizeof cache.a-1;
			if(off+n > sizeof cache.a)
				n = sizeof cache.a - off;
			memmove(buf, cache.a+off, n);
			return n;
		}
	}

	return pread(s->fd, buf, n, off);
}

static int
mget(Map *map, u64 addr, void *buf, int size)
{
	u64 off;
	int i, j, k;
	struct segment *s;

	s = reloc(map, addr, (i64*)&off);
	if (!s)
		return -1;
	if (s->fd < 0) {
		werrstr("unreadable map");
		return -1;
	}
	for (i = j = 0; i < 2; i++) {	/* in case read crosses page */
		k = spread(s, (void*)((u8 *)buf+j), size-j, off+j);
		if (k < 0) {
			werrstr("can't read address %p: %r", addr);
			return -1;
		}
		j += k;
		if (j == size)
			return j;
	}
	werrstr("partial read at address %p (size %d j %d)", addr, size, j);
	return -1;
}

static int
mput(Map *map, u64 addr, void *buf, int size)
{
	i64 off;
	int i, j, k;
	struct segment *s;

	s = reloc(map, addr, &off);
	if (!s)
		return -1;
	if (s->fd < 0) {
		werrstr("unwritable map");
		return -1;
	}

	seek(s->fd, off, 0);
	for (i = j = 0; i < 2; i++) {	/* in case read crosses page */
		k = write(s->fd, buf, size-j);
		if (k < 0) {
			werrstr("can't write address %p: %r", addr);
			return -1;
		}
		j += k;
		if (j == size)
			return j;
	}
	werrstr("partial write at address %p", addr);
	return -1;
}

/*
 *	convert address to file offset; returns nonzero if ok
 */
static struct segment*
reloc(Map *map, u64 addr, i64 *offp)
{
	int i;

	for (i = 0; i < map->nsegs; i++) {
		if (map->seg[i].inuse)
		if (map->seg[i].b <= addr && addr < map->seg[i].e) {
			*offp = addr + map->seg[i].f - map->seg[i].b;
			return &map->seg[i];
		}
	}
	werrstr("can't translate address %p", addr);
	return 0;
}
