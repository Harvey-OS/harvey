/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

/*
 * file map routines
 */
#include <u.h>
#include <libc.h>
#include <bio.h>
#include <mach.h>

Map *
newmap(Map *map, int n)
{
	int size;

	size = sizeof(Map)+(n-1)*sizeof(struct segment);
	if (map == 0)
		map = malloc(size);
	else
		map = realloc(map, size);
	if (map == 0) {
		werrstr("out of memory: %r");
		return 0;
	}
	memset(map, 0, size);
	map->nsegs = n;
	return map;
}

int
setmap(Map *map, int fd, uint64_t b, uint64_t e, int64_t f, char *name)
{
	int i;

	if (map == 0)
		return 0;
	for (i = 0; i < map->nsegs; i++)
		if (!map->seg[i].inuse)
			break;
	if (i >= map->nsegs)
		return 0;
	map->seg[i].b = b;
	map->seg[i].e = e;
	map->seg[i].f = f;
	map->seg[i].inuse = 1;
	map->seg[i].name = name;
	map->seg[i].fd = fd;
	return 1;
}

static uint64_t
stacktop(int pid)
{
	char buf[64];
	int fd;
	int i, n;
	char *cp;

	// Gets the stack segment details by parsing the first line of
	// /proc/pid/segment, assuming it'll be 'Stack'.
	// This line will be of the form:
	// Stack  rw-   7ffffee00000 7fffffe00000    1
	// We need to get the 4th token - the top of the stack.
	snprint(buf, sizeof(buf), "/proc/%d/segment", pid);
	fd = open(buf, 0);
	if (fd < 0)
		return 0;
	n = read(fd, buf, sizeof(buf)-1);
	close(fd);

	buf[n] = 0;
	if (strncmp(buf, "Stack", 5))
		return 0;

	for (i = 0, cp = buf; i < 3; i++) {
		// Skip over the token to the next space
		cp = strchr(cp, ' ');
		if (!cp)
			return 0;

		// Skip over spaces
		while (*cp == ' ')
			cp++;
		if (!*cp)
			return 0;
	}

	// cp now points to the token representing the end of the stack,
	// so parse and return it.
	return strtoull(cp, 0, 16);
}

Map*
attachproc(int pid, int kflag, int corefd, Fhdr *fp)
{
	char buf[64], *regs;
	int fd;
	Map *map;
	uint64_t n;

	map = newmap(0, 4);
	if (!map)
		return 0;
	if(kflag)
		regs = "kregs";
	else
		regs = "regs";
	if (mach->regsize) {
		sprint(buf, "/proc/%d/%s", pid, regs);
		fd = open(buf, ORDWR);
		if(fd < 0)
			fd = open(buf, OREAD);
		if(fd < 0) {
			free(map);
			return 0;
		}
		setmap(map, fd, 0, mach->regsize, 0, "regs");
	}
	if (mach->fpregsize) {
		sprint(buf, "/proc/%d/fpregs", pid);
		fd = open(buf, ORDWR);
		if(fd < 0)
			fd = open(buf, OREAD);
		if(fd < 0) {
			close(map->seg[0].fd);
			free(map);
			return 0;
		}
		setmap(map, fd, mach->regsize, mach->regsize+mach->fpregsize, 0, "fpregs");
	}
	setmap(map, corefd, fp->txtaddr, fp->txtaddr+fp->txtsz, fp->txtaddr, "text");
	if(kflag || fp->dataddr >= mach->utop) {
		setmap(map, corefd, fp->dataddr, ~0, fp->dataddr, "data");
		return map;
	}
	n = stacktop(pid);
	if (n == 0) {
		setmap(map, corefd, fp->dataddr, mach->utop, fp->dataddr, "data");
		return map;
	}
	setmap(map, corefd, fp->dataddr, n, fp->dataddr, "data");
	return map;
}
	
int
findseg(Map *map, char *name)
{
	int i;

	if (!map)
		return -1;
	for (i = 0; i < map->nsegs; i++)
		if (map->seg[i].inuse && !strcmp(map->seg[i].name, name))
			return i;
	return -1;
}

void
unusemap(Map *map, int i)
{
	if (map != 0 && 0 <= i && i < map->nsegs)
		map->seg[i].inuse = 0;
}

Map*
loadmap(Map *map, int fd, Fhdr *fp)
{
	map = newmap(map, 2);
	if (map == 0)
		return 0;

	map->seg[0].b = fp->txtaddr;
	map->seg[0].e = fp->txtaddr+fp->txtsz;
	map->seg[0].f = fp->txtoff;
	map->seg[0].fd = fd;
	map->seg[0].inuse = 1;
	map->seg[0].name = "text";
	map->seg[1].b = fp->dataddr;
	map->seg[1].e = fp->dataddr+fp->datsz;
	map->seg[1].f = fp->datoff;
	map->seg[1].fd = fd;
	map->seg[1].inuse = 1;
	map->seg[1].name = "data";
	return map;
}
