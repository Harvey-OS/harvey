/* emit code to look up n bits at offset o; table entries are 1<<l bytes long */
#define TABLE(o,n,l)				\
	if(32-((o)+(n))-(l) >= 0) {		\
		*p++ = Ortable;			\
		*p++ = 32-((o)+(n))-(l);	\
	} else {				\
		*p++ = Oltable;			\
		*p++ = l;			\
	}					\
	*p++ = ((1<<(n))-1)<<(l)

/* emit code to assemble low n bits of Rd into offset o in Rs */
#define ASSEMBLE(o,n)				\
	if((o) == 0) {				\
		*p++ = Olsh_RsRd;		\
		*p++ = 32-(n);			\
	} else if((o) == 32-(n)) {		\
		*p++ = ODorS;			\
	} else {				\
		*p++ = Oorlsh_RsRd;		\
		*p++ = 32-((o)+(n));		\
	}

static int
expand(Arg *a)
{
	Type *p, *li;
	int o, c, w, sf, df, sh, le, dl, b, db, firstd, firsts;

	o = a->fp->s1;
	w = a->width;
	sh = a->shift;
	p = a->memp;
	le = a->le;
	b = (le <= 2) ? 8 : 32 / (1 << le);
	db = b << le;
	dl = (le <= 2) ? le : 2;
/* print("expand soff %d doff %d sh %d w %d le %d dl %d lmask 0x%8ux rmask 0x%8ux\n",
a->soff, a->doff, sh, w, le, dl, a->lmask, a->rmask); */
	*p++ = Inittab; *p++ = (Type)a->tab; *p++ = (Type)a->tabosiz;

	/*
	 * method:
	 * load the source a word at a time, into Rt;
	 * (if there is a shift, use Ru to hold next or last partial word)
	 * take b bits at a time from source and convert via table into Rd
	 * (each table lookup yields db bits, in 1<<dl bytes);
	 * assemble into Rs until have 32 bits;
	 * fetch dest word into Rd, operate into Rs, store in dest
	 */

	if(sh == 0) {
		*p++ = Load_Rt_P;
	} else if(sh > 0) {
		*p++ = Load_Rt_P;
		*p++ = Olsh_RtRt;	*p++ = sh;
		if((a->doff+w)>>le  > (32 - sh)) {
			*p++ = Load_Ru_P;
			*p++ = Oorrsh_RtRu;	*p++ = 32 - sh;
			a->sc++;
		}
	} else {
		*p++ = Load_Ru_P;
		*p++ = Orsh_RtRu;	*p++ = -sh;
	}
	a->sc ++;
	sf = (a->soff - sh) & ~(b-1);
	firstd = 1;
	firsts = 1;
	while(sf < 32 && w > 0) {
		if(firstd)
			df = (sf << le) & 31;
		else
			df = 0;
		while(df < 32) {
			TABLE(sf,b,dl);
			if(df==0 || firsts) {
				*p++ = Olsh_RsRd;
				*p++ = 32 - (df + db);
			} else if(df == 32 - db) {
				*p++ = ODorS;
			} else {
				*p++ = Oorlsh_RsRd;
				*p++ = 32 - (df + db);
			}
			sf += b;
			df += db;
			firsts = 0;
		}
		*p++ = Fetch_Rd;
		if(o)
			*p++ = o;
		if(firstd) {
			w -= 32 - a->doff;
			if(w > 0 && a->lmask != ~0UL) {
					*p++ = Ofield;
					*p++ = a->lmask;
			} else if(w <= 0) {
				*p++ = Ofield;
				*p++ = a->lmask & a->rmask;
			}
		} else {
			w -= 32;
			if(w < 0) {
				*p++ = Ofield;
				*p++ = a->rmask;
			}
		}
		*p++ = Store_Rs_P;
		a->dc++;
		firstd = 0;
	}
	if(w <= 0)
		goto ret;

	c = w >> (5+le);
	if(c) {
		li = 0;
		if(c > 1) {
			*p++ = Ilabel;	*p++ = (Type)c;
			li = p;
		}
		if(sh == 0) {
			*p++ = Load_Rt_P;
		} else if(sh > 0) {
			*p++ = Olsh_RtRu;	*p++ = sh;
			*p++ = Load_Ru_P;
			*p++ = Oorrsh_RtRu;	*p++ = 32 - sh;
		} else {
			*p++ = Olsh_RtRu;	*p++ = 32 + sh;
			*p++ = Load_Ru_P;
			*p++ = Oorrsh_RtRu;	*p++ = -sh;
		}
		a->sc += c;
		for(sf = 0; sf < 32;) {
			for(df = 0; df < 32;) {
				TABLE(sf,b,dl);
				ASSEMBLE(df,db);
				sf += b;
				df += db;
			}
			if(a->fp->fetchd)
				*p++ = Fetch_Rd;
			if(o)
				*p++ = o;
			*p++ = Store_Rs_P;
			a->dc += c;
		}
		if(c > 1) {
			*p++ = Iloop;	*p++ = (Type)li;
		}
	}
	w -= c << (5+le);
	if(w <= 0)
		goto ret;

	if(sh == 0) {
		*p++ = Load_Rt_P;
		a->sc++;
	} else if(sh > 0) {
		*p++ = Olsh_RtRu;	*p++ = sh;
		if(w>>le  > (32 - sh)) {
			*p++ = Load_Ru_P;
			*p++ = Oorrsh_RtRu;	*p++ = 32 - sh;
			a->sc++;
		}
	} else {
		*p++ = Olsh_RtRu;	*p++ = 32 + sh;
		if(w>>le > (-sh)) {
			*p++ = Load_Ru_P;
			*p++ = Oorrsh_RtRu;	*p++ = -sh;
			a->sc++;
		}
	}
	for(sf = 0; sf < 32 && w > 0; ) {
		for(df = 0; df < 32;) {
			TABLE(sf,b,dl);
			ASSEMBLE(df,db);
			sf += b;
			df += db;
		}
		*p++ = Fetch_Rd;
		if(o)
			*p++ = o;
		w -= 32;
		if(w < 0) {
			*p++ = Ofield;
			*p++ = a->rmask;
		}
		*p++ = Store_Rs_P;
		a->dc++;
	}

ret:
	a->memp = p;
	return 1;
}

static int
contract(Arg *a)
{
	Type *p, *li;
	int o, c, w, sf, df, sh, db, firstd, firsts, nskip;

	o = a->fp->s1;
	w = a->width;
	sh = a->shift;
	p = a->memp;
	db = 8 >> -a->le;

	*p++ = Inittab; *p++ = (Type)a->tab; *p++ = (Type)a->tabosiz;

	/*
	 * method:
	 * load the source a word at a time, into Rt;
	 * (if there is a shift, use Ru to hold next or last partial word)
	 * take 8 bits at a time from source and convert via table into Rd
	 * (each table lookup yields db bits, in 1 byte);
	 * assemble into Rs until have 32 bits (takes several src words);
	 * fetch dest word into Rd, operate into Rs, store in dest.
	 *
	 * Something should be done to improve this code, but
	 * it isn't used much.
	 */

	if(sh < 0) {
		nskip = (-sh)/32;
		sh += nskip*32;
	} else
		nskip = 0;
	firstd = 1;
	firsts = 1;
	for(df = nskip*db*4; df < 32 && df < a->doff + w; ) {
		/* c = number of source bits needed to fill rest */
		c = (a->doff + w - df) << -a->le;
		if(sh == 0) {
			*p++ = Load_Rt_P;
			a->sc++;
		} else if(sh > 0) {
			if(firsts) {
				*p++ = Load_Rt_P;
				a->sc++;
				*p++ = Olsh_RtRt;	*p++ = sh;
			} else {
				*p++ = Olsh_RtRu;	*p++ = sh;
			}
			if(32 - sh < c) {
				*p++ = Load_Ru_P;
				a->sc++;
				*p++ = Oorrsh_RtRu;	*p++ = 32 - sh;
			}
		} else {
			if(!firsts) {
				*p++ = Olsh_RtRu;	*p++ = 32 + sh;
			}
			if((-sh) < c) {
				*p++ = Load_Ru_P;
				a->sc++;
				if(firsts) {
					*p++ = Orsh_RtRu;	*p++ = -sh;
				} else {
					*p++ = Oorrsh_RtRu;	*p++ = -sh;
				}
			}
		}
		firsts = 0;
		for(sf = 0; sf < 32; ) {
			TABLE(sf,8,0);
			if(firstd) {
				*p++ = Olsh_RsRd;	*p++ = 32-(df+db);
			} else {
				ASSEMBLE(df,db);
			}
			df += db;
			sf += 8;
			firstd = 0;
		}
	}
	*p++ = Fetch_Rd;
	if(o)
		*p++ = o;
	w -= 32 - a->doff;
	if(w > 0 && a->lmask != ~0UL) {
			*p++ = Ofield;
			*p++ = a->lmask;
	} else if(w <= 0) {
		*p++ = Ofield;
		*p++ = a->lmask & a->rmask;
	}
	*p++ = Store_Rs_P;
	a->dc++;
	if(w <= 0)
		goto ret;

	c = w >> 5;
	if(c) {
		li = 0;
		if(c > 1) {
			*p++ = Ilabel;	*p++ = (Type)c;
			li = p;
		}
		for(df = 0; df < 32; ) {
			if(sh == 0) {
				*p++ = Load_Rt_P;
			} else if(sh > 0) {
				*p++ = Olsh_RtRu;	*p++ = sh;
				*p++ = Load_Ru_P;
				*p++ = Oorrsh_RtRu;	*p++ = 32 - sh;
			} else {
				*p++ = Olsh_RtRu;	*p++ = 32 + sh;
				*p++ = Load_Ru_P;
				*p++ = Oorrsh_RtRu;	*p++ = -sh;
			}
			a->sc += c;
			for(sf = 0; sf < 32; ) {
				TABLE(sf,8,0);
				ASSEMBLE(df,db);
				df += db;
				sf += 8;
			}
		}
		if(a->fp->fetchd)
			*p++ = Fetch_Rd;
		if(o)
			*p++ = o;
		*p++ = Store_Rs_P;
		a->dc += c;
		if(c > 1) {
			*p++ = Iloop;	*p++ = (Type)li;
		}
	}

	w -= c << 5;
	if(w <= 0)
		goto ret;

	for(df = 0; df < w; ) {
		c = (w - df) << -a->le;
		if(sh == 0) {
			*p++ = Load_Rt_P;
			a->sc++;
		} else if(sh > 0) {
			*p++ = Olsh_RtRu;	*p++ = sh;
			if(32 - sh < c) {
				*p++ = Load_Ru_P;
				*p++ = Oorrsh_RtRu;	*p++ = 32 - sh;
				a->sc++;
			}
		} else {
			*p++ = Olsh_RtRu;	*p++ = 32 + sh;
			if(-sh < c) {
				*p++ = Load_Ru_P;
				*p++ = Oorrsh_RtRu;	*p++ = -sh;
				a->sc++;
			}
		}
		for(sf = 0; sf < 32; ) {
			TABLE(sf,8,0);
			ASSEMBLE(df,db);
			df += db;
			sf += 8;
		}
	}
	*p++ = Fetch_Rd;
	if(o)
		*p++ = o;
	*p++ = Ofield;
	*p++ = a->rmask;
	*p++ = Store_Rs_P;
	a->dc++;

ret:
	a->memp = p;
	return 1;
}
