#include <u.h>
#include <libc.h>
#include <ctype.h>
#include "git.h"

enum {
	Sinit,
	Siter,
};

static int
crackidx(char *path, int *np)
{
	int fd;
	char buf[4];

	if((fd = open(path, OREAD)) == -1)
		return -1;
	if(seek(fd, 8 + 255*4, 0) == -1)
		return -1;
	if(readn(fd, buf, sizeof(buf)) != sizeof(buf))
		return -1;
	*np = GETBE32(buf);
	return fd;
}

int
isloosedir(char *s)
{
	return strlen(s) == 2 && isxdigit(s[0]) && isxdigit(s[1]);
}

int
endswith(char *n, char *s)
{
	int nn, ns;

	nn = strlen(n);
	ns = strlen(s);
	return nn > ns && strcmp(n + nn - ns, s) == 0;
}

int
olsreadpacked(Objlist *ols, Hash *h)
{
	char *p;
	int i, j;

	i = ols->packidx;
	j = ols->entidx;

	if(ols->state == Siter)
		goto step;
	for(i = 0; i < ols->npack; i++){
		if(!endswith(ols->pack[i].name, ".idx"))
			continue;
		if((p = smprint(".git/objects/pack/%s", ols->pack[i].name)) == nil)
			sysfatal("smprint: %r");
		ols->fd = crackidx(p, &ols->nent);
		free(p);
		if(ols->fd == -1)
			continue;
		j = 0;
		while(j < ols->nent){
			if(readn(ols->fd, h->h, sizeof(h->h)) != sizeof(h->h))
				continue;
			ols->state = Siter;
			ols->packidx = i;
			ols->entidx = j;
			return 0;
step:
			j++;
		}
		close(ols->fd);
	}
	ols->state = Sinit;
	return -1;
}


int
olsreadloose(Objlist *ols, Hash *h)
{
	char buf[64], *p;
	int i, j, n;

	i = ols->topidx;
	j = ols->looseidx;
	if(ols->state == Siter)
		goto step;
	for(i = 0; i < ols->ntop; i++){
		if(!isloosedir(ols->top[i].name))
			continue;
		if((p = smprint(".git/objects/%s", ols->top[i].name)) == nil)
			sysfatal("smprint: %r");
		ols->fd = open(p, OREAD);
		free(p);
		if(ols->fd == -1)
			continue;
		while((ols->nloose = dirread(ols->fd, &ols->loose)) > 0){
			j = 0;
			while(j < ols->nloose){
				n = snprint(buf, sizeof(buf), "%s%s", ols->top[i].name, ols->loose[j].name);
				if(n >= sizeof(buf))
					goto step;
				if(hparse(h, buf) == -1)
					goto step;
				ols->state = Siter;
				ols->topidx = i;
				ols->looseidx = j;
				return 0;
step:
				j++;
			}
			free(ols->loose);
			ols->loose = nil;
		}
		close(ols->fd);
		ols->fd = -1;
	}
	ols->state = Sinit;
	return -1;
}

Objlist*
mkols(void)
{
	Objlist *ols;

	ols = emalloc(sizeof(Objlist));
	if((ols->ntop = slurpdir(".git/objects", &ols->top)) == -1)
		sysfatal("read top level: %r");
	if((ols->npack = slurpdir(".git/objects/pack", &ols->pack)) == -1)
		ols->pack = nil;
	ols->fd = -1;
	return ols;
}

void
olsfree(Objlist *ols)
{
	if(ols == nil)
		return;
	if(ols->fd != -1)
		close(ols->fd);
	free(ols->top);
	free(ols->loose);
	free(ols->pack);
	free(ols);
}

int
olsnext(Objlist *ols, Hash *h)
{
	if(ols->stage == 0){
		if(olsreadloose(ols, h) != -1){
			ols->idx++;
			return 0;
		}
		ols->stage++;
	}
	if(ols->stage == 1){
		if(olsreadpacked(ols, h) != -1){
			ols->idx++;
			return 0;
		}
		ols->stage++;
	}
	return -1;
}
