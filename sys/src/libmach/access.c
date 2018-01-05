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

static	int	mget(Map*, uint64_t, void*, int);
static	int	mput(Map*, uint64_t, void*, int);
static	struct	segment*	reloc(Map*, uint64_t, int64_t*);

/*
 * routines to get/put various types
 */
int
geta(Map *map, uint64_t addr, uint64_t *x)
{
	uint32_t l;
	uint64_t vl;

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
get8(Map *map, uint64_t addr, uint64_t *x)
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
get4(Map *map, uint64_t addr, uint32_t *x)
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
get2(Map *map, uint64_t addr, uint16_t *x)
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
get1(Map *map, uint64_t addr, uint8_t *x, int size)
{
	uint8_t *cp;

	if (!map) {
		werrstr("get1: invalid map");
		return -1;
	}

	if (map->nsegs == 1 && map->seg[0].fd < 0) {
		cp = (uint8_t*)&addr;
		while (cp < (uint8_t*)(&addr+1) && size-- > 0)
			*x++ = *cp++;
		while (size-- > 0)
			*x++ = 0;
	} else
		return mget(map, addr, x, size);
	return 1;
}

int
puta(Map *map, uint64_t addr, uint64_t v)
{
	if (mach->szaddr == 8)
		return put8(map, addr, v);

	return put4(map, addr, v);
}

int
put8(Map *map, uint64_t addr, uint64_t v)
{
	if (!map) {
		werrstr("put8: invalid map");
		return -1;
	}
	v = machdata->swav(v);
	return mput(map, addr, &v, 8);
}

int
put4(Map *map, uint64_t addr, uint32_t v)
{
	if (!map) {
		werrstr("put4: invalid map");
		return -1;
	}
	v = machdata->swal(v);
	return mput(map, addr, &v, 4);
}

int
put2(Map *map, uint64_t addr, uint16_t v)
{
	if (!map) {
		werrstr("put2: invalid map");
		return -1;
	}
	v = machdata->swab(v);
	return mput(map, addr, &v, 2);
}

int
put1(Map *map, uint64_t addr, uint8_t *v, int size)
{
	if (!map) {
		werrstr("put1: invalid map");
		return -1;
	}
	return mput(map, addr, v, size);
}

static int
spread(struct segment *s, void *buf, int n, uint64_t off)
{
	uint64_t base;

	static struct {
		struct segment *s;
		char a[8192];
		uint64_t off;
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
mget(Map *map, uint64_t addr, void *buf, int size)
{
	uint64_t off;
	int i, j, k;
	struct segment *s;

	s = reloc(map, addr, (int64_t*)&off);
	if (!s)
		return -1;
	if (s->fd < 0) {
		werrstr("unreadable map");
		return -1;
	}
	for (i = j = 0; i < 2; i++) {	/* in case read crosses page */
		k = spread(s, (void*)((uint8_t *)buf+j), size-j, off+j);
		if (k < 0) {
			werrstr("can't read address %llux: %r", addr);
			return -1;
		}
		j += k;
		if (j == size)
			return j;
	}
	werrstr("partial read at address %llux (size %d j %d)", addr, size, j);
	return -1;
}

static int
mput(Map *map, uint64_t addr, void *buf, int size)
{
	int64_t off;
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
			werrstr("can't write address %llux: %r", addr);
			return -1;
		}
		j += k;
		if (j == size)
			return j;
	}
	werrstr("partial write at address %llux", addr);
	return -1;
}

/*
 *	convert address to file offset; returns nonzero if ok
 */
static struct segment*
reloc(Map *map, uint64_t addr, int64_t *offp)
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
