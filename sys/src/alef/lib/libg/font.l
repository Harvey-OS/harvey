#include <alef.h>
#include <graphics.h>

intern int	resize(Font*, int, int);
intern int	freeup(Font*);

int
cachechars(Font *f, char **s, usint *cp, int max, int *wp)
{
	int i, th, sh, h, ld, w, wid, nc;
	char *sp;
	Rune r, vr;
	int a;
	Cacheinfo *c, *tc, *ec;

	sp = *s;
	wid = 0;
	for(i=0; *sp && i<max; sp+=w){
		r = *(char*)sp;
		if(r < Runeself)
			w = 1;
		else{
			w = chartorune(&vr, sp);
			r = vr;
		}

		sh = (17 * (uint)r) & (f->ncache-NFLOOK-1);
		c = &f->cache[sh];
		ec = c+NFLOOK;
		h = sh;
		while(c < ec){
			if(c->value==r && c->age)
				goto Found;
			c++;
			h++;
		}
	
		/*
		 * Not found; toss out oldest entry
		 */
		a = ~0;
		th = sh;
		tc = &f->cache[th];
		while(tc < ec){
			if(tc->age < a){
				a = tc->age;
				h = th;
				c = tc;
			}
			tc++;
			th++;
		}

		if(a && (f->age-a)<2000){	/* kicking out too recent; resize */
			nc = 2*(f->ncache-NFLOOK) + NFLOOK;
			if(nc <= MAXFCACHE){
				if(i == 0)
					resize(f, f->width, nc);
				/* else flush first; retry will resize */
				break;
			}
		}

		if(c->age == f->age)	/* flush pending string output */
			break;

		ld = loadchar(f, r, c, h, i);
		if(ld <= 0){
			if(ld == 0)
				continue;
			break;
		}
		c = &f->cache[h];	/* may have reallocated f->cache */
	
	    Found:
		wid += c->width;
		c->age = f->age;
		cp[i] = h;
		i++;
	}
	*s = sp;
	*wp = wid;
	return i;
}

int
charwidth(Font *f, Rune r)
{
	char buf[UTFmax+1];
	char *p;
	usint c;
	int wid;
	int n;

	if(r == 0)
		berror("NUL in charwidth");	/* difficult BUG */

	if(r < Runeself){
		buf[0] = r;
		buf[1] = 0;
	}else{
		n = runetochar(buf, &r);
		buf[n] = 0;
	}
	p = buf;
	n = 0;
	while(cachechars(f, &p, &c, 1, &wid) <= 0)
		if(++n > 10)
			berror("charwidth");
	agefont(f);
	return wid;
}

void
agefont(Font *f)
{
	Cacheinfo *c, *ec;
	Cachesubf *s, *es;

	f->age++;
	if(f->age == 65536){
		/*
		 * Renormalize ages
		 */
		c = f->cache;
		ec = c+f->ncache;
		while(c < ec){
			if(c->age){
				c->age >>= 2;
				c->age++;
			}
			c++;
		}
		s = f->subf;
		es = s+f->nsubf;
		while(s < es){
			if(s->age){
				if(s->age<SUBFAGE && s->cf->name != nil){
					/* clean up */
					subffree(s->f);
					s->cf = nil;
					s->f = nil;
					s->age = 0;
				}else{
					s->age >>= 2;
					s->age++;
				}
			}
			s++;
		}
		f->age = (65536>>2) + 1;
	}
}

/* return 1 if load succeeded, 0 if failed, -1 if must resize */
int
loadchar(Font *f, Rune r, Cacheinfo *c, int h, int noflush)
{
	int fd, i, oi, wid, try;
	Rune pic;
	Fontchar *fi;
	Cachefont *cf;
	Cachesubf *subf, *of;
	char *b;

	pic = r;
	try = 1;
    Again:
	for(i=0; i<f->nsub; i++){
		cf = f->sub[i];
		if(cf->min<=pic && pic<=cf->max)
			goto Found;
	}
	if(++try == 2){
		pic = 0;	/* use NUL==pjw for invisible characters */
		goto Again;
	}
	return 0;

    Found:
	/*
	 * Choose exact or oldest
	 */
	oi = 0;
	subf = &f->subf[0];
	for(i=0; i<f->nsubf; i++){
		if(cf == subf->cf)
			goto Found2;
		if(subf->age < f->subf[oi].age)
			oi = i;
		subf++;
	}
	subf = &f->subf[oi];

	if(subf->f){
		if(f->age-subf->age>SUBFAGE || f->nsubf>MAXSUBF){
    Toss:
			/* ancient data; toss */
			subffree(subf->f);
			subf->cf = nil;
			subf->f = nil;
			subf->age = 0;
		}else{				/* too recent; grow instead */
			of = f->subf;
			f->subf = malloc((f->nsubf+DSUBF)*sizeof *subf);
			if(f->subf == nil){
				f->subf = of;
				goto Toss;
			}
			memmove(f->subf, of, (f->nsubf+DSUBF)*sizeof *subf);
			memset(f->subf+f->nsubf, 0, DSUBF*sizeof *subf);
			subf = &f->subf[f->nsubf];
			f->nsubf += DSUBF;
			free(of);
		}
	}
	subf->age = 0;
	subf->cf = nil;
	fd = -1;
	if(!cf->abs){
		char tmp1[128], tmp2[128];
		char *t, *u;
		/*
		 * prefix name with directory if necessary
		 * and try to find a file suffixed with the right ldepth.
		 */
		t = cf->name;
		if(t[0] != '/'){
			strcpy(tmp2, f->name);
			u = utfrrune(tmp2, '/');
			if(u)
				u[0] = 0;
			else
				strcpy(tmp2, ".");
			sprint(tmp1, "%s/%s", tmp2, t);
			t = tmp1;
		}
		sprint(tmp2, "%s.%d", t, f->ldepth);
		if(access(tmp2, 0) == 0)
			t = tmp2;
		else if(access(t, 0) < 0)
			goto Err;
		t = strdup(t);
		if(t == nil)
			goto Err;
		free(cf->name);
		cf->name = t;
		cf->abs = 1;
	}
	fd = open(cf->name, OREAD);
	if(fd < 0){
    Err:
		if(fd >= 0)
			close(fd);
		fprint(2, "loadchar: can't open %s: %r\n", cf->name);
		return 0;
	}
	subf->f = rdsubfontfile(fd, nil);
	if(subf->f == nil){
		freeup(f);
		subf->f = rdsubfontfile(fd, nil);
		if(subf->f == nil){
			close(fd);
			goto Err;
		}
	}
	close(fd);
	subf->cf = cf;

    Found2:
	subf->age = f->age;

	if(pic-cf->min >= subf->f->n)
		return 0;
	fi = &subf->f->info[pic - cf->min];
	if(fi->width==0 && pic!=0){
		pic = 0;
		try = 2;
		goto Again;
	}
	wid = (fi+1)->x - fi->x;
	if(f->width < wid || f->width == 0){
		/*
		 * Flush, free, reload (easier than reformatting f->b)
		 */
		if(noflush)
			return -1;
		i = resize(f, wid, f->ncache);
		if(i <= 0)
			return i;
		/* c is still valid as didn't reallocate f->cache */
	}
	c->value = r;
	c->top = fi->top + (f->ascent-subf->f->ascent);
	c->bottom = fi->bottom + (f->ascent-subf->f->ascent);
	c->width = fi->width;
	c->left = fi->left;
	c->x = h*f->width;
	c->xright = c->x + wid;
	b = bneed(9);
	b[0] = 'y';
	BPSHORT(b+1, f->id);
	BPSHORT(b+3, h);
	BPSHORT(b+5, subf->f->id);
	BPSHORT(b+7, pic-cf->min);
	return 1;
}

/* release all subfonts, return number freed */
intern int
freeup(Font *f)
{
	Cachesubf *s, *es;
	int nf;

	if(f->sub[0]->name == nil)	/* font from mkfont; don't free */
		return 0;
	s = f->subf;
	es = s+f->nsubf;
	nf = 0;
	while(s < es){
		if(s->age){
			subffree(s->f);
			s->cf = nil;
			s->f = nil;
			s->age = 0;
			nf++;
		}
		s++;
	}
	return nf;
}

/* return whether resize succeeded && f->cache is unchanged */
intern int
resize(Font *f, int wid, int ncache)
{
	char *b;
	Cacheinfo *i;
	int nf, ret;

	b = bneed(7);
	b[0] = 'v';
	BPSHORT(b+1, f->id);
	BPSHORT(b+3, ncache);
	BPSHORT(b+5, wid);
	if(!bwrite()){
		/* try freeing some memory; subfonts can be big */
		nf = freeup(f);
		if(f != font)
			nf += freeup(font);
		if(wid != f->width){
			if(nf == 0)
				berror("font resize failed");
			return -1;	/* will trigger retry */
		}
		/* it's survivable: just clean existing cache */
		ncache = f->ncache;
	}
    ok:
	f->width = wid;
	ret = 1;
	if(f->ncache != ncache){
		ret = 0;
		i = malloc(ncache*sizeof f->cache[0]);
		if(i){
			free(f->cache);
			f->ncache = ncache;
			f->cache = i;
		}
		/* else just wipe the cache clean and things will be ok */
	}
	memset(f->cache, 0, f->ncache*sizeof f->cache[0]);
	return ret;
}
