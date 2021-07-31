#include <u.h>
#include <libc.h>
#include "complete.h"

static int
longestprefixlength(char *a, char *b, int n)
{
	int i, w;
	Rune ra, rb;

	for(i=0; i<n; i+=w){
		w = chartorune(&ra, a);
		chartorune(&rb, b);
		if(ra != rb)
			break;
		a += w;
		b += w;
	}
	return i;
}

void
freecompletion(Completion *c)
{
	if(c){
		free(c->filename);
		free(c);
	}
}

static int
strpcmp(const void *va, const void *vb)
{
	char *a, *b;

	a = *(char**)va;
	b = *(char**)vb;
	return strcmp(a, b);
}

Completion*
complete(char *dir, char *s)
{
	long i, l, n, nmatch, len, nbytes;
	int fd, minlen;
	Dir *dirp;
	char **name, *p;
	ulong* mode;
	Completion *c;

	if(strchr(s, '/') != nil){
		werrstr("slash character in name argument to complete()");
		return nil;
	}

	fd = open(dir, OREAD);
	if(fd < 0)
		return nil;

	n = dirreadall(fd, &dirp);
	if(n <= 0)
		return nil;

	/* find longest string, for allocation */
	len = 0;
	for(i=0; i<n; i++){
		l = strlen(dirp[i].name) + 1 + 1; /* +1 for /   +1 for \0 */
		if(l > len)
			len = l;
	}

	name = malloc(n*sizeof(char*));
	mode = malloc(n*sizeof(ulong));
	c = malloc(sizeof(Completion) + len);
	if(name == nil || mode == nil || c == nil)
		goto Return;
	memset(c, 0, sizeof(Completion));

	/* find the matches */
	len = strlen(s);
	nmatch = 0;
	minlen = 1000000;
	for(i=0; i<n; i++)
		if(strncmp(s, dirp[i].name, len) == 0){
			name[nmatch] = dirp[i].name;
			mode[nmatch] = dirp[i].mode;
			if(minlen > strlen(dirp[i].name))
				minlen = strlen(dirp[i].name);
			nmatch++;
		}

	if(nmatch > 0) {
		/* report interesting results */
		/* trim length back to longest common initial string */
		for(i=1; i<nmatch; i++)
			minlen = longestprefixlength(name[0], name[i], minlen);

		/* build the answer */
		c->complete = (nmatch == 1);
		c->advance = c->complete || (minlen > len);
		c->string = (char*)(c+1);
		memmove(c->string, name[0]+len, minlen-len);
		if(c->complete)
			c->string[minlen++ - len] = (mode[0]&DMDIR)? '/' : ' ';
		c->string[minlen - len] = '\0';
	} else {
		/* no match, so return all possible strings */
		for(i=0; i<n; i++){
			name[i] = dirp[i].name;
			mode[i] = dirp[i].mode;
		}
		nmatch = n;
	}

	/* attach list of names */
	nbytes = nmatch * sizeof(char*);
	for(i=0; i<nmatch; i++)
		nbytes += strlen(name[i]) + 1 + 1;
	c->filename = malloc(nbytes);
	if(c->filename == nil)
		goto Return;
	p = (char*)(c->filename + nmatch);
	for(i=0; i<nmatch; i++){
		c->filename[i] = p;
		strcpy(p, name[i]);
		p += strlen(p);
		if(mode[i] & DMDIR)
			*p++ = '/';
		*p++ = '\0';
	}
	c->nfile = nmatch;
	qsort(c->filename, c->nfile, sizeof(c->filename[0]), strpcmp);

  Return:
	free(name);
	free(mode);
	free(dirp);
	return c;
}
