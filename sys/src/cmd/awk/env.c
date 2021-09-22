#include <u.h>
#include <libc.h>
#include <stdio.h>
#include <ctype.h>
#include <math.h>
#include "awk.h"
#include "y.tab.h"

/*
 * Some plan9 environment variables
 * have 0 bytes in them (notably $path);
 * we change them to 1's (and execve changes back)
 *
 * Also, register the note handler.
 */

char **environ;

enum {
	Envhunk=7000,
	NAME_MAX=255,
};

void
envsetup(void)
{
	int dfd, n, nd, m, i, j, f, psize, cnt;
	char *ps, *p;
	char **pp;
	char name[NAME_MAX+5];
	Dir *d9, *d9a;
	static char **emptyenvp = 0;

	environ = emptyenvp;		/* pessimism */
	cnt = 0;
	strcpy(name, "#e");
	dfd = open(name, 0);
	if(dfd < 0)
		return;
	name[2] = '/';
	ps = p = malloc(Envhunk);
	if(p == 0)
		return;
	psize = Envhunk;
	nd = dirreadall(dfd, &d9a);
	close(dfd);
	for(j=0; j<nd; j++){
		d9 = &d9a[j];
		n = strlen(d9->name);
		if(n >= sizeof name - 4)
			continue;	/* shouldn't be possible */
		m = d9->length;
		i = p - ps;
		if(i+n+1+m+1 > psize) {
			psize += (n+m+2 < Envhunk)? Envhunk : n+m+2;
			ps = realloc(ps, psize);
			if (ps == 0) {
				free(d9a);
				return;
			}
			p = ps + i;
		}
		memcpy(p, d9->name, n);
		p[n] = '=';
		strcpy(name+3, d9->name);
		f = open(name, 0);
		if(f < 0 || read(f, p+n+1, m) != m)
			m = 0;
		close(f);
		if(p[n+m] == 0)
			m--;
		for(i=0; i<m; i++)
			if(p[n+1+i] == 0)
				p[n+1+i] = 1;
		p[n+1+m] = 0;
		p += n+m+2;
		cnt++;
	}
	free(d9a);
	pp = malloc((1+cnt)*sizeof(char *));
	if (pp == 0)
		return;
	environ = pp;
	p = ps;
	for(i = 0; i < cnt; i++) {
		*pp++ = p;
		p = memchr(p, 0, ps+psize-p);
		if (!p)
			break;
		p++;
	}
	*pp = 0;
//	notify(_notehandler);		// TODO
}
