#include <alef.h>
#include <graphics.h>

intern char*
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
	uint min, max;
	char *gbuf;

	fd = open(name, OREAD);
	if(fd < 0)
		return nil;

	rescue {
		close(fd);
		return nil;
	}
	if(dirfstat(fd, &dir) < 0)
    		raise;

	buf = malloc(dir.length+1);
	if(buf == nil)
		raise;

	buf[dir.length] = 0;
	i = read(fd, buf, dir.length);
	close(fd);

	rescue {
		free(buf);
		return nil;
	}
	if(i != dir.length)
    		raise;

	close(fd);
	s = buf;
	fnt = malloc(sizeof(Font));
	if(fnt == nil)
    		raise;

	memset(fnt, 0, sizeof(Font));
	fnt->name = strdup(name);
	fnt->ncache = NFCACHE+NFLOOK;
	fnt->nsubf = NFSUBF;
	fnt->cache = malloc(fnt->ncache * sizeof(fnt->cache[0]));
	fnt->subf = malloc(fnt->nsubf * sizeof(fnt->subf[0]));
	if(fnt->name==nil || fnt->cache==nil || fnt->subf==nil)
		raise Err2;

	rescue Err2 {
		free(fnt->name);
		free(fnt->cache);
		free(fnt->subf);
		free(fnt->sub);
		free(fnt);
		raise;
	}

	fnt->height = strtoi(s, &s, 0);
	s = skip(s);
	fnt->ascent = strtoi(s, &s, 0);
	s = skip(s);
	if(fnt->height<=0 || fnt->ascent<=0)
		raise Err2;
	fnt->width = 0;
	fnt->ldepth = ldepth;

	gbuf = bneed(7);
	gbuf[0] = 'n';
	gbuf[1] = fnt->height;
	gbuf[2] = fnt->ascent;
	BPSHORT(gbuf+3, ldepth);
	BPSHORT(gbuf+5, fnt->ncache);
	if(!bwrite())
		raise Err2;
	if(read(bitbltfd, gbuf, 3)!=3 || gbuf[0]!='N')
		raise Err2;

	fnt->id = gbuf[1] | (gbuf[2]<<8);
	fnt->nsub = 0;
	fnt->sub = nil;

	memset(fnt->subf, 0, sizeof(fnt->subf));
	memset(fnt->cache, 0, fnt->ncache*sizeof(fnt->cache[0]));
	fnt->age = 1;
	do{
		min = strtoi(s, &s, 0);
		s = skip(s);
		max = strtoi(s, &s, 0);
		s = skip(s);
		rescue {
			ffree(fnt);
			return nil;
		}

		if(*s==0 || min>=65536 || max>=65536 || min>max)
			raise;

		fnt->sub = realloc(fnt->sub, (fnt->nsub+1)*sizeof(Cachefont*));
		if(fnt->sub == nil){
			/* realloc manual says fnt->sub may have been destroyed */
			fnt->nsub = 0;
			raise;
		}
		c = malloc(sizeof(Cachefont));
		if(c == nil)
			raise;
		fnt->sub[fnt->nsub] = c;
		c->min = min;
		c->max = max;
		t = s;
		while(*s && *s!=' ' && *s!='\n' && *s!='\t')
			s++;
		*s++ = 0;
		c->abs = 0;
		c->name = strdup(t);
		if(c->name == nil){
			free(c);
			raise;
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
	char *b;

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
