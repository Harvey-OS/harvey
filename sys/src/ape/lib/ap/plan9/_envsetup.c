#include "lib.h"
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
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
static void sigsetup(char *, char *);

enum {
	Envhunk=7000,
};

void
_envsetup(void)
{
	int dfd;
	struct dirent *de;
	int n, nd, m, i, j, f;
	int psize, cnt;
	int nohandle;
	int fdinited;
	char *ps, *p;
	char **pp;
	Dir *d9, *d9a;

	nohandle = 0;
	fdinited = 0;
	cnt = 0;
	dfd = _OPEN(name, 0);
	if(dfd < 0) {
		static char **emptyenvp = 0;
		environ = emptyenvp;
		return;
	}
	name[2] = '/';
	ps = p = malloc(Envhunk);
	psize = Envhunk;
	nd = _dirreadall(dfd, &d9a);
	_CLOSE(dfd);
	for(j=0; j<nd; j++){
		d9 = &d9a[j];
		n = strlen(d9->name);
		m = d9->length;
		i = p - ps;
		if(i+n+1+m+1 > psize) {
			psize += (n+m+2 < Envhunk)? Envhunk : n+m+2;
			ps = realloc(ps, psize);
			p = ps + i;
		}
		memcpy(p, d9->name, n);
		p[n] = '=';
		strcpy(name+3, d9->name);
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
		if(strcmp(d9->name, "_fdinfo") == 0) {
			_fdinit(p+n+1, p+n+1+m);
			fdinited = 1;
		} else if(strcmp(d9->name, "_sighdlr") == 0)
			sigsetup(p+n+1, p+n+1+m);
		else if(strcmp(d9->name, "nohandle") == 0)
			nohandle = 1;
		p += n+m+2;
		cnt++;
	}
	free(d9a);
	if(!fdinited)
		_fdinit(0, 0);
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
sigsetup(char *s, char *se)
{
	int i, sig;
	char *e;

	while(s < se){
		sig = strtoul(s, &e, 10);
		if(s == e)
			break;
		s = e;
		if(sig <= MAXSIG)
			_sighdlr[sig] = SIG_IGN;
	}
}
