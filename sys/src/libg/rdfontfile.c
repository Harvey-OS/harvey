#include <u.h>
#include <libc.h>
#include <libg.h>


static char*
skip(char *s)
{
	while(*s==' ' || *s=='\n' || *s=='\t')
		s++;
	return s;
}

Font*
rdfontfile(char *name, int ldepth)
{
	Font *fnt;
	Cachefont *c;
	int fd, i;
	char *buf, *s, *t;
	Dir dir;
	ulong min, max;
	uchar *gbuf;
	int offset;

	fd = open(name, OREAD);
	if(fd < 0)
		return 0;
	if(dirfstat(fd, &dir) < 0){
    Err0:
		close(fd);
		return 0;
	}
	buf = malloc(dir.length+1);
	if(buf == 0)
		goto Err0;
	buf[dir.length] = 0;
	i = read(fd, buf, dir.length);
	close(fd);
	if(i != dir.length){
    Err1:
		free(buf);
		return 0;
	}

	s = buf;
	fnt = malloc(sizeof(Font));
	if(fnt == 0)
		goto Err1;
	memset(fnt, 0, sizeof(Font));
	fnt->name = strdup(name);
	fnt->ncache = NFCACHE+NFLOOK;
	fnt->nsubf = NFSUBF;
	fnt->cache = malloc(fnt->ncache * sizeof(fnt->cache[0]));
	fnt->subf = malloc(fnt->nsubf * sizeof(fnt->subf[0]));
	if(fnt->name==0 || fnt->cache==0 || fnt->subf==0){
    Err2:
		free(fnt->name);
		free(fnt->cache);
		free(fnt->subf);
		free(fnt->sub);
		free(fnt);
		goto Err1;
	}
	fnt->height = strtol(s, &s, 0);
	s = skip(s);
	fnt->ascent = strtol(s, &s, 0);
	s = skip(s);
	if(fnt->height<=0 || fnt->ascent<=0)
		goto Err2;
	fnt->width = 0;
	fnt->ldepth = ldepth;

	gbuf = bneed(7);
	gbuf[0] = 'n';
	gbuf[1] = fnt->height;
	gbuf[2] = fnt->ascent;
	BPSHORT(gbuf+3, ldepth);
	BPSHORT(gbuf+5, fnt->ncache);
	if(!bwrite())
		goto Err2;
	if(read(bitbltfd, gbuf, 3)!=3 || gbuf[0]!='N')
		goto Err2;
	fnt->id = gbuf[1] | (gbuf[2]<<8);
	fnt->nsub = 0;
	fnt->sub = 0;

	memset(fnt->subf, 0, fnt->nsubf * sizeof(fnt->subf[0]));
	memset(fnt->cache, 0, fnt->ncache*sizeof(fnt->cache[0]));
	fnt->age = 1;
	do{
		min = strtol(s, &s, 0);
		s = skip(s);
		max = strtol(s, &s, 0);
		s = skip(s);
		if(*s==0 || min>=65536 || max>=65536 || min>max){
    Err3:
			ffree(fnt);
			return 0;
		}
		t = s;
		offset = strtol(s, &t, 0);
		if(t>s && (*t==' ' || *t=='\t' || *t=='\n'))
			s = skip(t);
		else
			offset = 0;
		fnt->sub = realloc(fnt->sub, (fnt->nsub+1)*sizeof(Cachefont*));
		if(fnt->sub == 0){
			/* realloc manual says fnt->sub may have been destroyed */
			fnt->nsub = 0;
			goto Err3;
		}
		c = malloc(sizeof(Cachefont));
		if(c == 0)
			goto Err3;
		fnt->sub[fnt->nsub] = c;
		c->min = min;
		c->max = max;
		c->offset = offset;
		t = s;
		while(*s && *s!=' ' && *s!='\n' && *s!='\t')
			s++;
		*s++ = 0;
		c->abs = 0;
		c->name = strdup(t);
		if(c->name == 0){
			free(c);
			goto Err3;
		}
		s = skip(s);
		fnt->nsub++;
	}while(*s);
	free(buf);
	return fnt;
}

void
ffree(Font *f)
{
	int i;
	Cachefont *c;
	uchar *b;

	for(i=0; i<f->nsub; i++){
		c = f->sub[i];
		free(c->name);
		free(c);
	}
	for(i=0; i<f->nsubf; i++)
		if(f->subf[i].f)
			subffree(f->subf[i].f);
	free(f->cache);
	free(f->subf);
	free(f->sub);
	if(f->id >= 0){
		b = bneed(3);
		b[0] = 'h';
		BPSHORT(b+1, f->id);
	}
	free(f);
}
