/*
 * functions to read and write an executable or file image
 */

#include <u.h>
#include <libc.h>
#include <bio.h>
#include <mach.h>

static	int	mget(Map*, ulong, char*, int);
static	int	mput(Map*, ulong, char*, int);
static	struct	segment*	reloc(Map*, ulong, long*);

/*
 * routines to get/put various types
 */

int
get8(Map *map, ulong addr, vlong *x)
{
	if (!map) {
		werrstr("get8: invalid map");
		return -1;
	}

	if (map->nsegs == 1 && map->seg[0].fd < 0) {
		*x = (vlong)addr;
		return 1;
	}
	if (mget(map, addr, (char *)x, 8) < 0)
		return -1;
	*x = machdata->swav(*x);
	return (1);
}

int
get4(Map *map, ulong addr, long *x)
{
	if (!map) {
		werrstr("get4: invalid map");
		return -1;
	}

	if (map->nsegs == 1 && map->seg[0].fd < 0) {
		*x = addr;
		return 1;
	}
	if (mget(map, addr, (char *)x, 4) < 0)
		return -1;
	*x = machdata->swal(*x);
	return (1);
}

int
get2(Map *map, ulong addr, ushort *x)
{
	if (!map) {
		werrstr("get2: invalid map");
		return -1;
	}

	if (map->nsegs == 1 && map->seg[0].fd < 0) {
		*x = addr;
		return 1;
	}
	if (mget(map, addr, (char *)x, 2) < 0)
		return -1;
	*x = machdata->swab(*x);
	return (1);
}

int
get1(Map *map, ulong addr, uchar *x, int size)
{
	uchar *cp;

	if (!map) {
		werrstr("get1: invalid map");
		return -1;
	}

	if (map->nsegs == 1 && map->seg[0].fd < 0) {
		cp = (uchar*)&addr;
		while (cp < (uchar*)(&addr+1) && size-- > 0)
			*x++ = *cp++;
		while (size-- > 0)
			*x++ = 0;
	} else
		return mget(map, addr, (char*)x, size);
	return 1;
}

int
put8(Map *map, ulong addr, vlong v)
{
	if (!map) {
		werrstr("put8: invalid map");
		return -1;
	}
	v = machdata->swav(v);
	return mput(map, addr, (char *)&v, 8);
}

int
put4(Map *map, ulong addr, long v)
{
	if (!map) {
		werrstr("put4: invalid map");
		return -1;
	}
	v = machdata->swal(v);
	return mput(map, addr, (char *)&v, 4);
}

int
put2(Map *map, ulong addr, ushort v)
{
	if (!map) {
		werrstr("put2: invalid map");
		return -1;
	}
	v = machdata->swab(v);
	return mput(map, addr, (char *)&v, 2);
}

int
put1(Map *map, ulong addr, uchar *v, int size)
{
	if (!map) {
		werrstr("put1: invalid map");
		return -1;
	}
	return mput(map, addr, (char *)v, size);
}

static int
mget(Map *map, ulong addr, char *buf, int size)
{
	long off;
	uvlong voff;
	int i, j, k;
	struct segment *s;

	s = reloc(map, addr, &off);
	if (!s)
		return -1;
	if (s->fd < 0) {
		werrstr("unreadable map");
		return -1;
	}
	voff = (ulong)off;
	seek(s->fd, voff, 0);
	for (i = j = 0; i < 2; i++) {	/* in case read crosses page */
		k = read(s->fd, buf, size-j);
		if (k < 0) {
			werrstr("can't read address %lux: %r", addr);
			return -1;
		}
		j += k;
		if (j == size)
			return j;
	}
	werrstr("partial read at address %lux", addr);
	return -1;
}

static int
mput(Map *map, ulong addr, char *buf, int size)
{
	long off;
	vlong voff;
	int i, j, k;
	struct segment *s;

	s = reloc(map, addr, &off);
	if (!s)
		return -1;
	if (s->fd < 0) {
		werrstr("unwritable map");
		return -1;
	}

	voff = (ulong)off;
	seek(s->fd, voff, 0);
	for (i = j = 0; i < 2; i++) {	/* in case read crosses page */
		k = write(s->fd, buf, size-j);
		if (k < 0) {
			werrstr("can't write address %lux: %r", addr);
			return -1;
		}
		j += k;
		if (j == size)
			return j;
	}
	werrstr("partial write at address %lux", addr);
	return -1;
}

/*
 *	convert address to file offset; returns nonzero if ok
 */
static struct segment*
reloc(Map *map, ulong addr, long *offp)
{
	int i;

	for (i = 0; i < map->nsegs; i++) {
		if (map->seg[i].inuse)
		if (map->seg[i].b <= addr && addr < map->seg[i].e) {
			*offp = addr + map->seg[i].f - map->seg[i].b;
			return &map->seg[i];
		}
	}
	werrstr("can't translate address %lux", addr);
	return 0;
}
