#include "lib.h"
#include <stddef.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include "sys9.h"
#include "dir.h"

#ifdef OLDWAY
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

#else

enum
{
	BSIZE	= 128,
};

typedef struct Ns Ns;
struct Ns
{
	char	*from;
	char	*to;
	char	*spec;
	int	flags;
	Ns	*next;
};
static Ns	*nshead;

#define DBG	if(0) printf
#define nil 0

static int
tokenize(char *str, char **args, int max)
{
	int na;
	int intok = 0;

	if(max <= 0)
		return 0;	
	for(na=0; ;str++)
		switch(*str) {
		case ' ':
		case '\t':
		case '\n':
			*str = 0;
			if(!intok)
				continue;
			intok = 0;
			if(na < max)
				continue;
		case 0:
			return na;
		default:
			if(intok)
				continue;
			args[na++] = str;
			intok = 1;
		}
	return 0;	/* can't get here; silence compiler */
}

static int
nsresub(Ns *head, char *s, int buflen)
{
	Ns *f;
	int n;
	char *p, *buf;

	buf = malloc(buflen);
	if(buf == nil)
		return 0;
	for(;;) {
		DBG("sub %s\n", s);
		if(s[0] == '#' && s[1] == '/') {
			memmove(s, s+2, strlen(s)-1);
			free(buf);
			return 0;
		}
		p = strchr(s, '/');
		if(p != nil)
			*p = '\0';
		for(f = head; f; f = f->next) {
			if(strcmp(f->to, s) == 0) {
				n = strlen(f->to);
				if(strncmp(f->to, f->from, n) == 0 && f->from[n] == '/')
					continue;

				n = strlen(f->from) + 1;
				if(p != nil)
					n += strlen(p+1) + 1;
				if(n > buflen) {
					free(buf);
					return 0;
				}
				strcpy(buf, f->from);
				if(p != nil) {
					*p = '/';
					strcat(buf, p);
				}
				strcpy(s, buf);
				DBG("map %s->%s %s\n", f->from, f->to, s);
				break;
			}
		}
		if(f == nil) {
			if(p != nil)
				*p = '/';
			break;
		}
	}
	free(buf);
	return 0;
}

static void
nsfree(Ns *head)
{
	Ns *next;

	while(head) {
		next = head->next;
		free(head->from);
		free(head->to);
		if(head->spec != 0)
			free(head->spec);
		free(head);
		head = next;
	}
}

static Ns*
nsmk(char *f, char *t)
{
	Ns *n;

	n = malloc(sizeof(Ns));
	if(n == 0)
		return 0;
	n->spec = 0;
	n->from = strdup(f);
	if(n->from == 0) {
		free(n);
		return 0;
	}
	n->to = strdup(t);
	if(n->to == 0) {
		free(n->from);
		free(n);
		return 0;
	}
	return n;
}

static Ns*
nsread(char *error, int len, int *newkernel)
{
	int fd, n;
	Ns *head, *new;
	char junk[128], *na[6];

	head = 0;
	*newkernel = 0;

	sprintf(junk, "#p/%d/ns", getpid());
	fd = _OPEN(junk, OREAD);
	if(fd < 0) {
		errno = EIO;
		return nil;
	}
	for(;;) {
		n = _READ(fd, junk, sizeof(junk));
		if(n == 0)
			break;
		if(n < 0) {
			nsfree(head);
			errno = EIO;
			return nil;
		}
		junk[n] = '\0';
		if(strncmp(junk, "bind", 4)==0 || strncmp(junk, "mount", 5)==0){
			*newkernel = 1;
			return nil;
		}
		switch(tokenize(junk, na, 6)) {
		default:
			nsfree(head);
			errno = EIO;
			return 0;
		case 4:
			new = nsmk(na[1], na[2]);
			if(new == 0) {
				nsfree(head);
				errno = EIO;
				return 0;
			}
			new->spec = strdup(na[3]);
			new->flags = atoi(na[0]);
			break;
		case 3:
			new = nsmk(na[1], na[2]);
			if(new == 0) {
				nsfree(head);
				errno = ENOMEM;
				return 0;
			}
			new->flags = atoi(na[0]);
			break;
		}
		new->next = head;
		head = new;
	}
	_CLOSE(fd);
	return head;
}

char*
getcwd(char *buf, size_t len)
{
	int fd;
	int newkernel;

	nshead = nsread(buf, len, &newkernel);
	if(nshead == 0 && !newkernel)
		return 0;
	fd = _OPEN(".", OREAD);
	if(fd < 0) {
		nsfree(nshead);
		errno = EACCES;
		return 0;
	}
	if(_FD2PATH(fd, buf, len) < 0) {
		nsfree(nshead);
		errno = EIO;
		_CLOSE(fd);
		return 0;
	}
	_CLOSE(fd);
	if(!newkernel && nsresub(nshead, buf, len)) {
		nsfree(nshead);
		errno = ERANGE;
		return 0;
	}
	nsfree(nshead);
	if(buf[0] == '\0')
		strcpy(buf, "/");
	return buf;
}

#endif
