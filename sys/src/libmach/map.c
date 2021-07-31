/*
 * file map routines
 */
#include <u.h>
#include <libc.h>
#include <bio.h>
#include <mach.h>

static int	reloc(Map *, int, ulong, long *);
static int	rreloc(Map *, int, ulong, long *);

Map *
newmap(Map *map, int fd)
{
	if (map == 0) {
		map = malloc(sizeof(Map));
		if (map == 0) {
			print("out of memory\n");
			return 0;
		}
		map->seg[SEGTEXT].inuse = 0;
		map->seg[SEGDATA].inuse = 0;
		map->seg[SEGUBLK].inuse = 0;
		map->seg[SEGREGS].inuse = 0;
		map->seg[SEGTEXT].name = "text";
		map->seg[SEGDATA].name = "data";
		map->seg[SEGUBLK].name = "ublock";
		map->seg[SEGREGS].name = "regs";
	}
	map->fd = fd;
	return map;
}

int
setmap(Map *map, int s, ulong b, ulong e, ulong f)
{
	if (map == 0 || s == SEGANY)
		return 0;
	map->seg[s].b = b;
	map->seg[s].e = e;
	map->seg[s].f = f;
	map->seg[s].inuse = 1;
	return 1;
}

void
unusemap(Map *map, int s)
{
	if (map != 0 && s != SEGANY)
		map->seg[s].inuse = 0;
}

Map *
loadmap(Map *map, int fd, Fhdr *fp)
{
	map = newmap(map, fd);
	if (map == 0)
		return 0;
	if (mach == 0)
		return 0;
	map->seg[SEGTEXT].b = fp->txtaddr;
	map->seg[SEGTEXT].e = fp->txtaddr+fp->txtsz;
	map->seg[SEGTEXT].f = fp->txtoff;
	map->seg[SEGTEXT].inuse = 1;
	map->seg[SEGDATA].b = fp->dataddr;
	map->seg[SEGDATA].e = fp->dataddr+fp->datsz;
	map->seg[SEGDATA].f = fp->datoff;
	map->seg[SEGDATA].inuse = 1;
	map->seg[SEGUBLK].inuse = 0;
	map->seg[SEGREGS].inuse = 0;
	return map;
}

int
mget(Map *map, int s, ulong addr, char *buf, int size)
{
	long off;
	int i, j;

	if (reloc(map, s, addr, &off) == 0)
		return -1;
	if (seek(map->fd, off, 0) == -1)
		return 0;
	for (i = j = 0; i < 2; i++) {	/* in case read crosses page */
		j += read(map->fd, buf, size-j);
		if (j == size)
			return 1;
	}
	return 0;
}

int
mput(Map *map, int s, ulong addr, char *buf, int size)
{
	long off;
	int i, j;

	if (reloc(map, s, addr,&off) == 0)
		return (-1);
	if (seek(map->fd, off, 0) == -1)
		return 0;
	for (i = j = 0; i < 2; i++) {	/* in case read crosses page */
		j += write(map->fd, buf, size-j);
		if (j == size)
			return 1;
	}
	return 0;
}

/*
 * turn address to file offset
 * returns nonzero if ok
 */
static int
reloc(Map *map, int s, ulong addr, long *offp)
{
	if(map == 0)
		return 0;

	switch(s) {
	case SEGANY:
		if (rreloc(map, SEGTEXT, addr, offp))
			return 1;
		if (rreloc(map, SEGDATA, addr, offp))
			return 1;
		if (rreloc(map, SEGUBLK, addr, offp))
			return 1;
		break;
	case SEGDATA:
		if (rreloc(map, SEGDATA, addr, offp))
			return 1;
		if (rreloc(map, SEGUBLK, addr, offp))
			return 1;
		break;
	default:
		return rreloc(map, s, addr, offp);
	}
	return 0;
}

static int
rreloc(Map *map, int s, ulong addr, long *offp)
{
	if (map->seg[s].inuse) {
		if (map->seg[s].b <= addr && addr < map->seg[s].e) {
			*offp = addr + map->seg[s].f - map->seg[s].b;
			return 1;
		}
	}
	return 0;
}
