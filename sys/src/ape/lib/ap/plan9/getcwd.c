#include "lib.h"
#include <stddef.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include "sys9.h"
#include "dir.h"

#define DBG(s)
#define MAXDEPTH	100

static void namedev(Dir *);
static int  samefile(Dir *, Dir *);

char *
getcwd(char *s , size_t size) 
{
	Dir *p, db[MAXDEPTH], root;
	int depth, l;
	char *x, *v;
	char cd[DIRLEN];

	if(size == 0) {
		errno = EINVAL;
		return 0;
	}
	x = s;
	if(_STAT("/", cd) < 0) {
		errno = EACCES;
		return 0;
	}
	convM2D(cd, &root);

	for(depth = 0; depth < MAXDEPTH; depth++) {
		p = &db[depth];
		if(_STAT(".", cd) < 0) {
			errno = EACCES;
			return 0;
		}
		convM2D(cd, p);

		DBG(print("stat: %s %lux\n", p->name, p->qid);)

		if(samefile(p, &root)) {
			depth--;
			break;
		}

		if(depth > 1 && samefile(p, &db[depth-1])) {
			p->name[0] = '#';
			p->name[1] = p->type;
			p->name[2] = '\0';
			break;
		}

		if((p->name[0] == '.' || p->name[0] == '/') && 
		    p->name[1] == '\0')
			namedev(p);

		if(chdir("..") < 0) {
			errno = EACCES;
			return 0;
		}
	}

	while(depth >= 0) {
		v = db[depth--].name;
		if(v[0] == '.' && v[1] == '\0')
			continue;
		if(v[0] != '#')
			*x++ = '/';
		l = strlen(v);
		size -= l+1;
		if(size <= 0) {
			errno = ERANGE;
			return 0;
		}
		strcpy(x, v);
		x += l;
	}
	DBG(print("chdir %s\n", s);)
	if(chdir(s) < 0) {
		errno = EACCES;
		return 0;
	}
	return s;
}

static void
namedev(Dir *p)
{
	Dir dirb, tdirb;
	char buf[DIRLEN*50], sd[NAMELEN*2];
	char cd[DIRLEN];
	int fd, n;
	char *t, *e;

	fd = _OPEN("..", OREAD);
	if(fd < 0)
		return;

	for(;;) {
		n = _READ(fd, buf, sizeof(buf));
		if(n <= 0) {
			_CLOSE(fd);
			return;
		}
		e = &buf[n];

		for(t = buf; t < e; t += DIRLEN) {
			convM2D(t, &dirb);
			if((dirb.qid.path&CHDIR) == 0)
				continue;
			sprintf(sd, "../%s/.", dirb.name);
			if(_STAT(sd, cd) < 0)
				continue;
			convM2D(cd, &tdirb);

			if(samefile(&tdirb, p) == 0)
				continue;
			_CLOSE(fd);
			DBG(print("%s->%s\n", p->name, dirb.name);)
			strcpy(p->name, dirb.name);
			return;
		}
	}
	_CLOSE(fd);
}

static int
samefile(Dir *a, Dir *b)
{
	if(a->type != b->type)
		return 0;
	if(a->dev != b->dev)
		return 0;
	if(a->qid.path != b->qid.path)
		return 0;
	return 1;
}
