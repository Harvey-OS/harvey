#include "sam.h"

#define	BLOCKFILL	(BLOCKSIZE/2)

static Discdesc	desc[NBUFFILES];

void		bkalloc(Disc*, int);
void		bkfree(Disc*, int);
void		bkwrite(Disc*, Rune*, int, int, int);
void		bkread(Disc*, Rune*, int, int, int);


Discdesc *
Dstart(void)
{
	int i, fd;
	Discdesc *dd;

	for(i=0, dd=desc; dd->fd; i++, dd++)
		if(i == NBUFFILES-1)
			panic("too many buffer files");
	fd = newtmp(i);
	if(fd < 0)
		panic("can't create buffer file");
	dd->fd = fd;
	return dd;
}

Disc *
Dopen(Discdesc *dd)
{
	Disc *d;

	d = emalloc(sizeof(Disc));
	d->desc = dd;
	return d;
}

void
Dclose(Disc *d)
{
	int i;

	for(i=d->block.nused; --i>=0; )	/* backwards because bkfree() stacks */
		bkfree(d, i);
	free(d->block.listptr);
	free(d);
}

int
Dread(Disc *d, Rune *addr, int n, Posn p1)
{
	int i, nb, nr;
	Posn p = 0, p2 = p1+n;

	for(i=0; i<d->block.nused; i++){
		if((p+=d->block.blkptr[i].nrunes) > p1){
			p -= d->block.blkptr[i].nrunes;
			goto out;
		}
	}
	if(p == p1)
		return 0;	/* eof */
	return -1;		/* past eof */

    out:
	n = 0;
	if(p != p1){	/* trailing partial block */
		nb = d->block.blkptr[i].nrunes;
		if(p2 > p+nb)
			nr = nb-(p1-p);
		else
			nr = p2-p1;
		bkread(d, addr, nr, i, p1-p);
		/* advance to next block */
		p += nb;
		addr += nr;
		n += nr;
		i++;
	}
	/* whole blocks */
	while(p<p2 && (nb = d->block.blkptr[i].nrunes)<=p2-p){
		if(i >= d->block.nused)
			return n;	/* eof */
		bkread(d, addr, nb, i, 0);
		p += nb;
		addr += nb;
		n += nb;
		i++;
	}
	if(p < p2){	/* any initial partial block left? */
		nr = p2-p;
		nb = d->block.blkptr[i].nrunes;
		if(nr>nb)
			nr = nb;		/* eof */
		/* just read in the part that survives */
		bkread(d, addr, nr, i, 0);
		n += nr;
	}
	return n;
}

void
Dinsert(Disc *d, Rune *addr, int n, Posn p0) /* if addr null, just make space */
{
	int i, nb, ni;
	Posn p = 0;
	Rune hold[BLOCKSIZE];
	int nhold;

	for(i=0; i<d->block.nused; i++){
		if((p+=d->block.blkptr[i].nrunes) >= p0){
			p -= d->block.blkptr[i].nrunes;
			goto out;
		}
	}
	if(p != p0)
		panic("Dinsert");	/* beyond eof */

    out:
	d->nrunes += n;
	nhold = 0;
	if(i<d->block.nused && (nb=d->block.blkptr[i].nrunes)>p0-p){
		nhold = nb-(p0-p);
		bkread(d, hold, nhold, i, p0-p);
		d->block.blkptr[i].nrunes -= nhold;	/* no write necessary */
	}
	/* insertion point is now at end of block i (which may not exist) */
	while(n > 0){
		if(i < d->block.nused
		&& (nb=d->block.blkptr[i].nrunes) < BLOCKFILL){
			/* fill this block */
			if(nb+n > BLOCKSIZE)
				ni = BLOCKFILL-nb;
			else
				ni = n;
			if(addr)
				bkwrite(d, addr, ni, i, nb);
			nb += ni;
		}else{	/* make new block */
			if(i < d->block.nused)
				i++;	/* put after this block, if it exists */
			bkalloc(d, i);
			if(n > BLOCKSIZE)
				ni = BLOCKFILL;
			else
				ni = n;
			if(addr)
				bkwrite(d, addr, ni, i, 0);
			nb = ni;
		}
		d->block.blkptr[i].nrunes = nb;
		if(addr)
			addr += ni;
		n -= ni;
	}
	if(nhold){
		if(i < d->block.nused
		&& (nb=d->block.blkptr[i].nrunes)+nhold < BLOCKSIZE){
			/* fill this block */
			bkwrite(d, hold, nhold, i, nb);
			nb += nhold;
		}else{	/* make new block */
			if(i < d->block.nused)
				i++;	/* put after this block, if it exists */
			bkalloc(d, i);
			bkwrite(d, hold, nhold, i, 0);
			nb = nhold;
		}
		d->block.blkptr[i].nrunes = nb;
	}
}

void
Ddelete(Disc *d, Posn p1, Posn p2)
{
	int i, nb, nd;
	Posn p = 0;
	Rune buf[BLOCKSIZE];

	for(i = 0; i<d->block.nused; i++){
		if((p+=d->block.blkptr[i].nrunes) > p1){
			p -= d->block.blkptr[i].nrunes;
			goto out;
		}
	}
	if(p1!=d->nrunes || p2!=p1)
		panic("Ddelete");
	return;	/* beyond eof */

    out:
	d->nrunes -= p2-p1;
	if(p != p1){	/* throw away partial block */
		nb = d->block.blkptr[i].nrunes;
		bkread(d, buf, nb, i, 0);
		if(p2 >= p+nb)
			nd = nb-(p1-p);
		else{
			nd = p2-p1;
			memmove(buf+(p1-p), buf+(p1-p)+nd, RUNESIZE*(nb-((p1-p)+nd)));
		}
		nb -= nd;
		bkwrite(d, buf, nb, i, 0);
		d->block.blkptr[i].nrunes = nb;
		p2 -= nd;
		/* advance to next block */
		p += nb;
		i++;
	}
	/* throw away whole blocks */
	while(p<p2 && (nb = d->block.blkptr[i].nrunes)<=p2-p){
		if(i >= d->block.nused)
			panic("Ddelete 2");
		bkfree(d, i);
		p2 -= nb;
	}
	if(p >= p2)	/* any initial partial block left to delete? */
		return;	/* no */
	nd = p2-p;
	nb = d->block.blkptr[i].nrunes;
	/* just read in the part that survives */
	bkread(d, buf, nb-=nd, i, nd);
	/* a little block merging */
	if(nb<BLOCKSIZE/2 && i>0 && (nd = d->block.blkptr[i-1].nrunes)<BLOCKSIZE/2){
		memmove(buf+nd, buf, RUNESIZE*nb);
		bkread(d, buf, nd, --i, 0);
		bkfree(d, i);
		nb += nd;
	}
	bkwrite(d, buf, nb, i, 0);
	d->block.blkptr[i].nrunes = nb;
}

void
Dreplace(Disc *d, Posn p1, Posn p2, Rune *addr, int n)
{
	int i, nb, nr;
	Posn p = 0;
	Rune buf[BLOCKSIZE];

	if(p2-p1 > n)
		Ddelete(d, p1+n, p2);
	else if(p2-p1 < n)
		Dinsert(d, 0, n-(p2-p1), p2);
	if(n == 0)
		return;
	p2 = p1+n;
	/* they're now conformal; replace in place */
	for(i=0; i<d->block.nused; i++){
		if((p+=d->block.blkptr[i].nrunes) > p1){
			p -= d->block.blkptr[i].nrunes;
			goto out;
		}
	}
	panic("Dreplace");

    out:
	if(p != p1){	/* trailing partial block */
		nb = d->block.blkptr[i].nrunes;
		bkread(d, buf, nb, i, 0);
		if(p2 > p+nb)
			nr = nb-(p1-p);
		else
			nr = p2-p1;
		memmove(buf+p1-p, addr, RUNESIZE*nr);
		bkwrite(d, buf, nb, i, 0);
		/* advance to next block */
		p += nb;
		addr += nr;
		i++;
	}
	/* whole blocks */
	while(p<p2 && (nb = d->block.blkptr[i].nrunes)<=p2-p){
		if(i >= d->block.nused)
			panic("Dreplace 2");
		bkwrite(d, addr, nb, i, 0);
		p += nb;
		addr += nb;
		i++;
	}
	if(p < p2){	/* any initial partial block left? */
		nr = p2-p;
		nb = d->block.blkptr[i].nrunes;
		/* just read in the part that survives */
		bkread(d, buf+nr, nb-nr, i, nr);
		memmove(buf, addr, RUNESIZE*nr);
		bkwrite(d, buf, nb, i, 0);
	}
}

void
bkread(Disc *d, Rune *loc, int n, int bk, int off)
{
	Seek(d->desc->fd, RUNESIZE*(BLOCKSIZE*d->block.blkptr[bk].bnum+off), 0);
	Read(d->desc->fd, loc, n*RUNESIZE);
}

void
bkwrite(Disc *d, Rune *loc, int n, int bk, int off)
{
	Seek(d->desc->fd, RUNESIZE*(BLOCKSIZE*d->block.blkptr[bk].bnum+off), 0);
	Write(d->desc->fd, loc, n*RUNESIZE);
}

void
bkalloc(Disc *d, int n)
{
	Discdesc *dd = d->desc;
	ulong bnum;

	if(dd->free.nused)
		bnum = dd->free.longptr[--dd->free.nused];
	else
		bnum = dd->nbk++;
	if(bnum >= 1<<(8*(sizeof(((Block*)0)->bnum))))
		error(Etmpovfl);
	inslist(&d->block, n, 0L);
	d->block.blkptr[n].bnum = bnum;
}

void
bkfree(Disc *d, int n)
{
	Discdesc *dd = d->desc;

	inslist(&dd->free, dd->free.nused, d->block.blkptr[n].bnum);
	dellist(&d->block, n);
}
