#include <u.h>
#include <libc.h>
#include <bio.h>
#include <mach.h>
#define Extern extern
#include "3210.h"

void
fatal(char *fmt, ...)
{
	char buf[ERRLEN];

	doprint(buf, buf+sizeof(buf), fmt, (&fmt+1));
	fprint(2, "%s: %s\n", argv0, buf);
	exits(buf);
}

void
error(char *fmt, ...)
{
	char buf[8192];

	doprint(buf, buf + sizeof(buf) / sizeof(*buf), fmt, &fmt + 1);
	Bprint(bioout, "%s\n", buf);
	longjmp(errjmp, 0);
}

char *
memio(char *mb, ulong mem, int size, int dir)
{
	int i;
	char *buf, c;

	if(!mb)
		mb = malloc(size);
	if(!mb)
		error("memio: user/kernel copy of %d bytes too long for %s: %d", size, argv0);

	buf = mb;
	switch(dir){
	default:
		fatal("memio");
	case MemRead:
		while(size--)
			*mb++ = getmem_1(mem++);
		break;
	case MemReadstring:
		for(;;) {
			if(size-- == 0)
				error("memio: user/kernel copy too long for %s", argv0);
			c = getmem_1(mem++);
			*mb++ = c;
			if(c == '\0')
				break;
		}
		break;
	case MemWrite:
		for(i = 0; i < size; i++)
			putmem_1(mem++, *mb++);
		break;
	}
	return buf;
}

void *
emalloc(ulong size)
{
	void *a;

	a = malloc(size);
	if(a == 0)
		fatal("out of memory");

	memset(a, 0, size);
	return a;
}

void *
erealloc(void *a, ulong size)
{
	a = realloc(a, size);
	if(a == 0)
		fatal("out of memory");
	return a;
}

/*
 * replacements for standard db access routines
 * these should probably look at the map to pick up registers
 */
int
get4(Map *map, ulong addr, long *x)
{
	USED(map);

	*x = getmem_4(addr);
	return 1;
}

int
get2(Map *map, ulong addr, ushort *x)
{
	USED(map);

	*x = getmem_2(addr);
	return 1;
}

int
get1(Map *map, ulong addr, uchar *x, int size)
{
	USED(map);

	while(size-- > 0)
		*x++ = getmem_1(addr++);
	return 1;
}

int
put4(Map *map, ulong addr, long v)
{
	USED(map);

	putmem_4(addr, v);
	return 1;
}

int
put2(Map *map, ulong addr, ushort v)
{
	USED(map);

	putmem_2(addr, v);
	return 1;
}

int
put1(Map *map, ulong addr, uchar *v, int size)
{
	USED(map);

	while(size-- > 0)
		putmem_1(addr++, *v++);
	return 1;
}
