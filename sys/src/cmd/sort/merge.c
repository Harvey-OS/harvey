/* Copyright 1990, AT&T Bell Labs */
#include "fsort.h"

#define NMERGE	16
#define REC	500

enum
{
	IEOF, IDUP, IOK
};

int	firstfile;
int	nextfile;

typedef	struct	merge	Merge;

struct	merge
{
	char*	name;		/* name of file for diagnostics */
	FILE*	file;
	Rec*	rec;		/* pointer to the data (and key) */
	uchar*	recmax;		/* length of record */
	int	del;		/* delete at close */
};

Merge	mfile[2*NMERGE];
Merge*	flist[2*NMERGE];
int	nflist = 0;

extern	void	mergephase(int, FILE*, char*);
extern	int	insert(Merge*);
extern	int	compare(Merge*, Merge*);
		

void
recalloc(Merge *m)
{
	m->del = 0;
	if(m->rec)
		return;
	m->rec = (Rec*)malloc(REC);
	if(m->rec == 0)
		fatal("no space for merge records", "", 0);
	m->recmax = (uchar*)m->rec + REC;
}

/* misfortune 1: merging strategy is not stable.   */
/* misfortune 2: fields are parsed in their entirety
   before comparison.  lazy parsing might be in order */

void
merge(int argc, char **argv)
{
	FILE *input, *output;
	char *name;
	int i, n, j, nf;
	Merge *m;

	argc--;
	argv++;
	name = 0;
	i = nf = argc<NMERGE? argc: NMERGE;
	if(uflag)		/* two records per file */
		i *= 2;
	while(--i >= 0)
		recalloc(&mfile[i]);
	for(j=0; j<argc; j+=NMERGE) {
		n = j+NMERGE>=argc? argc-j: NMERGE;
		for(i=0; i<n; i++) {
			m = &mfile[i];
			m->name = argv[j+i];
			m->file = fileopen(m->name, "r");
			mfile[i+n].name = m->name;	/*for uflag*/
			mfile[i+n].file = m->file;
		}
		if(argc>NMERGE || overwrite(argc-j, argv+j))
			name = filename(nextfile++);
		else
			name = oname;
		mergephase(uflag?2*n:n, fileopen(name, "w"), name);
	}
	for(i=0; i<nf; i++)
		free((char*)mfile[i].rec);
	if(nextfile > 1) 
		mergetemp();
	else
	if(nextfile == 1) {
		input = fileopen(filename(0), "r");
		output = fileopen(oname, "w");
		while((i=getc(input)) != EOF)
			if(putc(i, output) == EOF)
				fatal("error writing", oname, 0);
		fileclose(output, name);
	}
}
	

void
mergetemp(void)
{
	char *name;
	Merge *m;
	int i = nextfile<NMERGE? nextfile: NMERGE;
 	int j, n;

	while(--i >= 0)
		recalloc(&mfile[i]);
	for(j=0; j<nextfile; j+=NMERGE) {
		n = j+NMERGE>=nextfile? nextfile-j: NMERGE;
		for(i=0; i<n; i++) {
			m = &mfile[i];
			m->name = filename(j+i);
			m->file = fileopen(m->name, "r");
			m->del = 1;
		}
		if(j+n < nextfile)
			name = filename(nextfile++);
		else
			name = oname;
		mergephase(n, fileopen(name, "w"), name);
	}
}

		/* merge 1st n files in mfile[] */
void
mergephase(int n, FILE *output, char *name)
{
	int i;
	Merge *m;

	nflist = 0;
	for(i=0; i<n; i++)
		while(insert(&mfile[i]) == IDUP)
			continue;
	while(nflist > 0) {
		m = flist[0];
		fprintf(output,"%.*s\n", m->rec->dlen, data(m->rec));
		nflist--;
		memmove(flist, flist+1, nflist*sizeof(*flist));
		while(insert(m) == IDUP)
			continue;
	}

	for(i=0; i<n; i++) {
		fileclose(mfile[i].file, 0);
		if(mfile[i].del)
			remove(mfile[i].name);
	}
	fileclose(output, name);
}

void
recrealloc(Merge *m, int partial)
{
	Rec *r = m->rec;
	Rec *p;
	int n = 2*(m->recmax - (uchar*)r);

	m->rec = r = (Rec*)realloc(r, n);
	if(r == 0)
		fatal("no space for record", "", 0);
	m->recmax = (uchar*)r + n;
	r->klen = 0;
	if(!partial)
		return;
	p = succ(r);
	switch(getline(p, m->recmax, m->file)) {
	case -2:
		n = p->dlen;
		memmove(key(r), data(p), n);
		r->dlen += n;
		recrealloc(m, 1);
		return;
	case -1:
		warn("newline appended", "", 0);
		return;
	}
	n = p->dlen;
	memmove(key(r), data(p), n);
	r->dlen += n;
}	

int
fillrec(Merge *m)
{
	Rec *r = m->rec;

	switch(getline(r, m->recmax, m->file)) {
	case -1:
		return IEOF;
	case -2:
		recrealloc(m, 1);
		r = m->rec;
	}
	if(keyed)
		while((r->klen=fieldcode(data(r),
			     key(r), r->dlen, m->recmax)) < 0) {
			recrealloc(m, 0);
			r = m->rec;
		}
	return IOK;
}

	/* opportunity for optimization:
	   one call of insert is preceded by memmove, which
           insert will undo right away if there is significant
	   clustering so that successive outputs are likely
	   to come from the same input file */

int
insert(Merge *m)
{
	int i, bot, top, t;

	if(fillrec(m) == IEOF)
		return IEOF;
	bot = 0;
	top = nflist;
	while(bot < top) {
		i = (bot+top)/2;
		t = compare(m, flist[i]);
		if(t < 0)
			top = i;
		else
		if(t > 0)
			bot = i + 1;
		else
		if(uflag)
			return IDUP;
		else {
			bot = i;
			break;
		}
	}
	memmove(flist+bot+1,flist+bot,(nflist-bot)*sizeof(*flist));
	flist[bot] = m;
	nflist++;
	return IOK;
}		

int		
compare(Merge *mi, Merge *mj)
{
	uchar *ip, *jp;
	uchar *ei, *ej;
	uchar *trans, *keep;
	int li, lj, k;

	if(simplekeyed) {
		trans = fields->trans;
		keep = fields->keep;
		ip = data(mi->rec);
		jp = data(mj->rec);
		ei = ip + mi->rec->dlen;
		ej = jp + mj->rec->dlen;
		k = 0;
		for( ; ; ip++, jp++) {
			while(ip<ei && !keep[*ip])
				ip++;
			while(jp<ej && !keep[*jp])
				jp++;
			if(ip>=ei || jp>=ej)
				break;
			k = trans[*ip] - trans[*jp];
			if(k != 0)
				break;
		}
		if(ip<ei && jp<ej)
			return k*signedrflag;
		if(ip < ei)
			return -signedrflag;
		if(jp < ej)
			return signedrflag;
		if(uflag)
			return 0;
	} else
	if(keyed) {
		ip = key(mi->rec);
		jp = key(mj->rec);
		li = mi->rec->klen;
		lj = mj->rec->klen;
		for(k=li<lj?li:lj; --k>=0; ip++, jp++)
			if(*ip != *jp)
				break;
		if(k < 0) {
			if(li != lj)
				fatal("theorem disproved","",0);
			if(uflag)
				return 0;
		} else
			return *ip - *jp;
		
	}
	li = mi->rec->dlen;
	lj = mj->rec->dlen;
	ip = data(mi->rec);
	jp = data(mj->rec);
	for(k=li<lj?li:lj; --k>=0; ip++, jp++)
		if(*ip != *jp)
			break;
	return (k<0? li-lj: *ip-*jp)*signedrflag;
}

int
check(char *name)
{
	int i, t;

	recalloc(&mfile[0]);
	recalloc(&mfile[1]);
	mfile[0].file = mfile[1].file = fileopen(name, "r");
	if(mfile[0].file == 0)
		fatal("can't open ", name, 0);

	if(fillrec(&mfile[0]) == IEOF)
		return 0;
	for(i=1; fillrec(&mfile[i])!=IEOF; i^=1) {
		t = compare(&mfile[i^1], &mfile[i]);
		if(t>0 || t==0 && uflag)
			fatal("disorder:",(char*)data(mfile[i].rec),
			       mfile[i].rec->dlen);

	}
	return 0;
}

void
catch(void *a, char *s)
{
	USED(a);
	cleanfiles(nextfile);
	exits(s);
}

void
mergeinit(void)
{
	notify(catch);
}

