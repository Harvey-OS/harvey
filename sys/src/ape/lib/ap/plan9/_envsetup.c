#include "lib.h"
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <unistd.h>
#include <string.h>
#include "sys9.h"
#include "dir.h"

/*
 * Called before main to initialize environ.
 * Some plan9 environment variables
 * have 0 bytes in them (notably $path);
 * we change them to 1's (and execve changes back)
 *
 * Also, register the note handler.
 */

char **environ;
int errno;
unsigned long _clock;
static char name[NAME_MAX+5] = "#e";
static void fdsetup(char *, char *);

enum {
	Envhunk=7000,
};

void
_envsetup(void)
{
	DIR *d;
	struct dirent *de;
	int n, m, i, f;
	int psize, cnt;
	int nohandle;
	char *ps, *p;
	char **pp;
	char cd[DIRLEN];
	Dir db;

	nohandle = 0;
	cnt = 0;
	d = opendir(name);
	if(!d) {
		static char **emptyenvp = 0;
		environ = emptyenvp;
		return;
	}
	name[2] = '/';
	ps = p = malloc(Envhunk);
	psize = Envhunk;
	while((de = readdir(d)) != NULL){
		strcpy(name+3, de->d_name);
		if(_STAT(name, cd) < 0)
			continue;
		convM2D(cd, &db);
		n = strlen(de->d_name);
		m = db.length;
		i = p - ps;
		if(i+n+1+m+1 > psize) {
			psize += (n+m+2 < Envhunk)? Envhunk : n+m+2;
			ps = realloc(ps, psize);
			p = ps + i;
		}
		memcpy(p, de->d_name, n);
		p[n] = '=';
		f = _OPEN(name, O_RDONLY);
		if(f < 0 || _READ(f, p+n+1, m) != m)
			m = 0;
		_CLOSE(f);
		if(p[n+m] == 0)
			m--;
		for(i=0; i<m; i++)
			if(p[n+1+i] == 0)
				p[n+1+i] = 1;
		p[n+1+m] = 0;
		if(strcmp(de->d_name, "_fdinfo") == 0)
			fdsetup(p+n+1, p+n+1+m);
		if(strcmp(de->d_name, "nohandle") == 0)
			nohandle = 1;
		p += n+m+2;
		cnt++;
	}
	closedir(d);
	environ = pp = malloc((1+cnt)*sizeof(char *));
	p = ps;
	for(i = 0; i < cnt; i++) {
		*pp++ = p;
		p = memchr(p, 0, ps+psize-p);
		if (!p)
			break;
		p++;
	}
	*pp = 0;
	if(!nohandle)
		_NOTIFY(_notehandler);
}

static void
fdsetup(char *s, char *se)
{
	unsigned long fd, fl, ofl;
	char *e;

	while(s < se){
		fd = strtoul(s, &e, 10);
		if(s == e)
			break;
		s = e;
		fl = strtoul(s, &e, 10);
		if(s == e)
			break;
		s = e;
		ofl = strtoul(s, &e, 10);
		if(s == e)
			break;
		s = e;
		if(fd < OPEN_MAX){
			_fdinfo[fd].flags = fl;
			_fdinfo[fd].oflags = ofl;
		}
	}
}
