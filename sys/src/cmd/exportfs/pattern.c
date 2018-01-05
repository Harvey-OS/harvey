/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include <u.h>
#include <libc.h>
#include <fcall.h>
#include <bio.h>
#include <regexp.h>
#include "exportfs.h"

Reprog	**exclude, **include;
char	*patternfile;

void
exclusions(void)
{
	Biobuf *f;
	int ni, nmaxi, ne, nmaxe;
	char *line;

	if(patternfile == nil)
		return;

	f = Bopen(patternfile, OREAD);
	if(f == nil)
		fatal("cannot open patternfile");
	ni = 0;
	nmaxi = 100;
	include = malloc(nmaxi*sizeof(*include));
	if(include == nil)
		fatal(Enomem);
	include[0] = nil;
	ne = 0;
	nmaxe = 100;
	exclude = malloc(nmaxe*sizeof(*exclude));
	if(exclude == nil)
		fatal(Enomem);
	exclude[0] = nil;
	while((line = Brdline(f, '\n')) != nil){
		line[Blinelen(f) - 1] = 0;
		if(strlen(line) < 2 || line[1] != ' ')
			continue;
		switch(line[0]){
		case '+':
			if(ni+1 >= nmaxi){
				nmaxi = 2*nmaxi;
				include = realloc(include, nmaxi*sizeof(*include));
				if(include == nil)
					fatal(Enomem);
			}
			DEBUG(DFD, "\tinclude %s\n", line+2);
			include[ni] = regcomp(line+2);
			include[++ni] = nil;
			break;
		case '-':
			if(ne+1 >= nmaxe){
				nmaxe = 2*nmaxe;
				exclude = realloc(exclude, nmaxe*sizeof(*exclude));
				if(exclude == nil)
					fatal(Enomem);
			}
			DEBUG(DFD, "\texclude %s\n", line+2);
			exclude[ne] = regcomp(line+2);
			exclude[++ne] = nil;
			break;
		default:
			DEBUG(DFD, "ignoring pattern %s\n", line);
			break;
		}
	}
	Bterm(f);
}

int
excludefile(char *path)
{
	Reprog **re;
	char *p;

	if(*(path+1) == 0)
		p = "/";
	else
		p = path+1;

	DEBUG(DFD, "checking %s\n", path);
	for(re = include; *re != nil; re++){
		if(regexec(*re, p, nil, 0) != 1){
			DEBUG(DFD, "excluded+ %s\n", path);
			return -1;
		}
	}
	for(re = exclude; *re != nil; re++){
		if(regexec(*re, p, nil, 0) == 1){
			DEBUG(DFD, "excluded- %s\n", path);
			return -1;
		}
	}
	return 0;
}

int
preaddir(Fid *f, uint8_t *data, int n, int64_t offset)
{
	int r = 0, m;
	Dir *d;

	DEBUG(DFD, "\tpreaddir n=%d wo=%lld fo=%lld\n", n, offset, f->offset);
	if(offset == 0 && f->offset != 0){
		if(seek(f->fid, 0, 0) != 0)
			return -1;
		f->offset = f->cdir = f->ndir = 0;
		free(f->dir);
		f->dir = nil;
	}else if(offset != f->offset){
		werrstr("can't seek dir %lld to %lld", f->offset, offset);
		return -1;
	}

	while(n > 0){
		if(f->dir == nil){
			f->ndir = dirread(f->fid, &f->dir);
			if(f->ndir < 0)
				return f->ndir;
			if(f->ndir == 0)
				return r;
		}
		d = &f->dir[f->cdir++];
		if(exclude){
			char *p = makepath(f->f, d->name);
			if(excludefile(p)){
				free(p);
				goto skipentry;
			}
			free(p);
		}
		m = convD2M(d, data, n);
		DEBUG(DFD, "\t\tconvD2M %d\n", m);
		if(m <= BIT16SZ){
			DEBUG(DFD, "\t\t\tneeded %d\n", GBIT16(data));
			/* not enough room for full entry; leave for next time */
			f->cdir--;
			return r;
		}else{
			data += m;
			n -= m;
			r += m;
			f->offset += m;
		}
skipentry:	if(f->cdir >= f->ndir){
			f->cdir = f->ndir = 0;
			free(f->dir);
			f->dir = nil;
		}
	}
	return r;
}
