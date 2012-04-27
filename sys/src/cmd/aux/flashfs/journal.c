#include <u.h>
#include <libc.h>
#include <auth.h>
#include <fcall.h>
#include <thread.h>
#include <9p.h>
#include "flashfs.h"

enum
{
	debug	= 0,
	diags	= 1,
};

typedef struct	Gen	Gen;
typedef struct	Sect	Sect;

struct Gen
{
	int	gnum;
	int	count;
	int	low;
	int	high;
	Sect*	head;
	Sect*	tail;
	Sect*	dup;
	Sect**	vect;
};

struct Sect
{
	int	sect;
	ulong	seq;
	int	coff;
	int	toff;
	int	sum;
	ulong	time;
	Sect*	next;
};

static	Gen	gens[2];
static	Sect*	freehead;
static	Sect*	freetail;
static	int	nfree;
static	int	nbad;
static	ulong	ltime;
static	int	cursect;

/*
 *	If we have a delta then file times are in the future.
 *	But they drift back to reality.
 */
ulong
now(void)
{
	ulong cur, drift;
	static ulong last;

	cur = time(0);
	if(cur < last)
		delta += last - cur;
	else if(delta != 0 && last != 0) {
		drift = (cur - last + 1) / 2;
		if(drift > delta)
			delta = 0;
		else
			delta -= drift;
	}
	last = cur;
	return cur + delta;
}

static void
damaged(char *mesg)
{
	sysfatal("damaged filesystem: %s", mesg);
}

static void
lddisc(char *mesg)
{
	if(debug)
		fprint(2, "discard %s\n", mesg);
	else
		USED(mesg);
}

static Sect *
getsect(void)
{
	Sect *t;

	t = freehead;
	freehead = t->next;
	if(freehead == nil)
		freetail = nil;
	nfree--;
	return t;
}

static void
newsect(Gen *g, Sect *s)
{
	int m, n, err;
	uchar hdr[2*3];

	if(debug)
		fprint(2, "new %d %ld\n", g->gnum, s->seq);
	if(g->tail == nil)
		g->head = s;
	else
		g->tail->next = s;
	g->tail = s;
	s->next = nil;
	m = putc3(&hdr[0], g->gnum);
	n = putc3(&hdr[m], s->seq);
	s->toff = MAGSIZE + m + n;
	s->coff = s->toff + 4;
	s->time = NOTIME;
	s->sum = 0;
	err = 1;
	for(;;) {
		if(writedata(err, s->sect, hdr, m + n, MAGSIZE)
		&& writedata(err, s->sect, magic, MAGSIZE, 0))
			break;
		clearsect(s->sect);
		err = 0;
	}
}

static Sect *
newsum(Gen *g, ulong seq)
{
	Sect *t;

	if(nfree == 0)
		damaged("no free sector for summary");
	t = getsect();
	t->seq = seq;
	newsect(g, t);
	return t;
}

static void
freesect(Sect *s)
{
	clearsect(s->sect);
	if(freetail == nil)
		freehead = s;
	else
		freetail->next = s;
	freetail = s;
	s->next = nil;
	nfree++;
}

static void
dupsect(Sect *s, int renum)
{
	Sect *t;
	Renum r;
	uchar *b;
	int err, n;
	ulong doff, off;

	if(nfree == 0)
		damaged("no free for copy");
	t = getsect();
	b = sectbuff;
	off = s->coff;
	readdata(s->sect, b, off, 0);
	doff = s->toff;
	if(s->time == NOTIME)
		doff += 4;
	// Window 5
	err = 1;
	for(;;) {
		if(writedata(err, t->sect, b + 1, s->toff - 1, 1)
		&& writedata(err, t->sect, b + doff, off - doff, doff)
		&& writedata(err, t->sect, b, 1, 0))
			break;
		clearsect(t->sect);
		err = 0;
	}
	if(renum) {
		r.old = s->sect;
		r.new = t->sect;
		erenum(&r);
	}
	n = s->sect;
	s->sect = t->sect;
	t->sect = n;
	freesect(t);
	if(cursect >= 0)
		readdata(cursect, b, sectsize, 0);
}

static void
gswap(void)
{
	Gen t;

	t = gens[0];
	gens[0] = gens[1];
	gens[1] = t;
}

static void
checkdata(void)
{
	Gen *g;

	g = &gens[0];
	if(g->dup != nil) {	// Window 5 damage
		if(diags)
			fprint(2, "%s: window 5 damage\n", argv0);
		freesect(g->dup);
		g->dup = nil;
	}
}

static void
checksweep(void)
{
	Gen *g;
	Jrec j;
	uchar *b;
	int n, op;
	Sect *s, *t, *u;
	long off, seq, soff;

	g = &gens[1];
	if(g->dup != nil) {	// Window 5 damage
		if(diags)
			fprint(2, "%s: window 5 damage\n", argv0);
		freesect(g->dup);
		g->dup = nil;
	}

	s = g->head;
	if(s != g->tail) {
		while(s->next != g->tail)
			s = s->next;
	}

	b = sectbuff;
	op = -1;
	seq = -1;
	soff = 0;
	u = nil;
	t = s;
	for(;;) {
		readdata(t->sect, b, sectsize, 0);
		off = t->toff + 4;
		for(;;) {
			n = convM2J(&j, &b[off]);
			if(n < 0) {
				if(j.type != 0xFF) {
					if(debug)
						fprint(2, "s[%d] @ %d %ld\n", j.type, t->sect, off);
					damaged("bad Jrec");
				}
				break;
			}
			switch(j.type) {
			case FT_SUMMARY:
			case FT_SUMBEG:
				seq = j.seq;
			case FT_SUMEND:
				op = j.type;
				soff = off;
				u = t;
				break;
			case FT_WRITE:
			case FT_AWRITE:
				off += j.size;
			}
			off += n;
		}
		t = t->next;
		if(t == nil)
			break;
	}

	if(op == FT_SUMBEG) {		// Window 1 damage
		if(diags)
			fprint(2, "%s: window 1 damage\n", argv0);
		if(u != s) {
			freesect(u);
			s->next = nil;
			g->tail = s;
		}
		s->coff = soff;
		dupsect(s, 0);
		seq--;
	}

	if(seq >= 0) {
		g = &gens[0];
		if(seq > g->low)
			damaged("high sweep");
		if(seq == g->low) {	// Window 2 damage
			if(diags)
				fprint(2, "%s: window 2 damage\n", argv0);
			s = g->head;
			g->head = s->next;
			freesect(s);
			if(g->head == nil) {
				g->tail = nil;
				g->gnum += 2;
				newsum(g, 0);
				gswap();
			}
		}
	}
}

void
load1(Sect *s, int parity)
{
	int n;
	Jrec j;
	uchar *b;
	char *err;
	Extent *x;
	Entry *d, *e;
	ulong ctime, off, mtime;

	if(s->sect < 0 && readonly)	// readonly damaged
		return;

	b = sectbuff;
	readdata(s->sect, b, sectsize, 0);
	s->sum = 0;
	off = s->toff;
	ctime = get4(&b[off]);
	off += 4;

	for(;;) {
		n = convM2J(&j, &b[off]);
		if(n < 0) {
			if(j.type != 0xFF) {
				if(debug)
					fprint(2, "l[%d] @  %d %ld\n", j.type, s->sect, off);
				damaged("bad Jrec");
			}
			s->coff = off;
			break;
		}
		off += n;
		if(debug)
			fprint(2, "get %J\n", &j);
		switch(j.type) {
		case FT_create:
			ctime += j.mtime;
		create:
			d = elookup(j.parent);
			if(d == nil) {
				lddisc("parent");
				break;
			}
			d->ref++;
			e = ecreate(d, j.name, j.fnum, j.mode, ctime, &err);
			if(e == nil) {
				d->ref--;
				lddisc("create");
				break;
			}
			e->ref--;
			if(j.type == FT_trunc)
				e->mode = j.mode;
			break;
		case FT_chmod:
			e = elookup(j.fnum);
			if(e == nil) {
				lddisc("lookup");
				break;
			}
			echmod(e, j.mode, j.mnum);
			break;
		case FT_REMOVE:
			e = elookup(j.fnum);
			if(e == nil) {
				lddisc("lookup");
				break;
			}
			if(eremove(e) != nil) {
				lddisc("remove");
				break;
			}
			break;
		case FT_AWRITE:
			mtime = 0;
			goto write;
		case FT_WRITE:
			ctime += j.mtime;
			mtime = ctime;
		write:
			x = emalloc9p(sizeof(Extent));
			x->sect = s->sect;
			x->addr = off;
			x->off = j.offset;
			x->size = j.size;
			off += j.size;
			e = elookup(j.fnum);
			if(e == nil) {
				lddisc("lookup");
				break;
			}
			ewrite(e, x, parity, mtime);
			break;
		case FT_trunc:
			ctime += j.mtime;
			e = elookup(j.tnum);
			if(e == nil) {
				if(debug)
					fprint(2, "-> create\n");
				goto create;
			}
			etrunc(e, j.fnum, ctime);
			break;
		case FT_SUMMARY:
		case FT_SUMBEG:
		case FT_SUMEND:
			s->sum += n;
			break;
		default:
			damaged("load1 botch");
		}
	}

	if(s->sum > Nsum)
		s->sum = Nsum;

	s->time = ctime;
	if(ctime != NOTIME && ctime > ltime)
		ltime = ctime;
}

void
load0(int parity)
{
	Sect *s;

	if(debug)
		fprint(2, "load %d\n", parity);
	for(s = gens[parity].head; s != nil; s = s->next)
		load1(s, parity);
}

void
loadfs(int ro)
{
	Gen *g;
	Sect *s;
	ulong u, v;
	int i, j, n;
	uchar hdr[MAXHDR];

	readonly = ro;
	fmtinstall('J', Jconv);

	for(i = 0; i < 2; i++) {
		g = &gens[i];
		g->gnum = -1;
		g->low = nsects;
		g->high = -1;
		g->count = 0;
		g->head = nil;
		g->tail = nil;
	}

	for(i = 0; i < nsects; i++) {
		readdata(i, hdr, MAXHDR, 0);
		if(memcmp(hdr, magic, MAGSIZE) == 0) {
			n = MAGSIZE + getc3(&hdr[MAGSIZE], &u);
			getc3(&hdr[n], &v);
			if(debug)
				fprint(2, "%d: %ld %ld\n", i, u, v);
			for(j = 0; j < 2; j++) {
				g = &gens[j];
				if(g->gnum == u) {
					g->count++;
					if(v < g->low)
						g->low = v;
					if(v > g->high)
						g->high = v;
					break;
				}
				else if(g->gnum < 0) {
					g->gnum = u;
					g->count = 1;
					g->low = v;
					g->high = v;
					break;
				}
			}
			if(j == 2)
				damaged("unexpected generation");
		}
		else if(hdr[0] == 0xFF) {
			nfree++;
			s = emalloc9p(sizeof(Sect));
			s->sect = i;
			s->next = freehead;
			freehead = s;
			if(freetail == nil)
				freetail = s;
		}
		else
			nbad++;
	}

	if(nbad > 0)
		damaged("bad sectors");

	if(gens[0].gnum < 0)
		damaged("no data");

	if(gens[1].gnum < 0) {		// Window 3 death
		if(diags)
			fprint(2, "%s: window 3 damage\n", argv0);
		g = &gens[1];
		g->gnum = gens[0].gnum + 1;
		g->low = 0;
		g->high = 0;
		g->count = 1;
		if(!readonly)
			newsum(g, 0);
	}

	if(gens[0].gnum > gens[1].gnum)
		gswap();

	for(i = 0; i < 2; i++) {
		g = &gens[i];
		n = g->count;
		if(n <= g->high - g->low)
			damaged("missing sectors");
		g->vect = emalloc9p(n * sizeof(Sect *));
		for(j = 0; j < n; j++) {
			s = emalloc9p(sizeof(Sect));
			s->sect = -1;
			g->vect[j] = s;
		}
	}

	if(debug) {
		for(j = 0; j < 2; j++) {
			g = &gens[j];
			print("%d\t%d\t%d-%d\n", g->gnum, g->count, g->low, g->high);
		}
	}

	for(i = 0; i < nsects; i++) {
		readdata(i, hdr, MAXHDR, 0);
		if(memcmp(hdr, magic, MAGSIZE) == 0) {
			n = MAGSIZE + getc3(&hdr[MAGSIZE], &u);
			n += getc3(&hdr[n], &v);
			g = nil;
			for(j = 0; j < 2; j++) {
				g = &gens[j];
				if(g->gnum == u)
					break;
			}
			if(j == 2)
				damaged("generation botch");
			s = g->vect[v - g->low];
			s->seq = v;
			s->toff = n;
			if(s->sect < 0)
				s->sect = i;
			else if(v == g->high && g->dup == nil) {
				s = emalloc9p(sizeof(Sect));
				s->sect = i;
				g->dup = s;
			}
			else
				damaged("dup block");
		}
	}

	for(j = 0; j < 2; j++) {
		g = &gens[j];
		for(i = 0; i < g->count; i++) {
			s = g->vect[i];
			if(g->tail == nil)
				g->head = s;
			else
				g->tail->next = s;
			g->tail = s;
			s->next = nil;
		}
		free(g->vect);
	}

	cursect = -1;

	if(!readonly) {
		checkdata();
		checksweep();
	}

	load0(1);
	load0(0);

	if(ltime != 0) {
		u = now();
		if(u < ltime) {
			delta = ltime - u;
			if(diags)
				fprint(2, "%s: check clock: delta = %lud\n", argv0, delta);
		}
	}

	limit = 80 * nsects * sectsize / 100;
	maxwrite = sectsize - MAXHDR - Nwrite - Nsum;
	if(maxwrite > WRSIZE)
		maxwrite = WRSIZE;
}

static int
sputw(Sect *s, Jrec *j, int mtime, Extent *x, void *a)
{
	ulong t;
	int err, n, r;
	uchar buff[Nmax], type[1];

	if(debug)
		fprint(2, "put %J\n", j);

	if(mtime) {
		t = j->mtime;
		if(s->time == NOTIME) {
			put4(buff, t);
			if(!writedata(1, s->sect, buff, 4, s->toff)) {
				dupsect(s, 1);
				writedata(0, s->sect, buff, 4, s->toff);
			}
			s->time = t;
			j->mtime = 0;
		}
		else {
			j->mtime = t - s->time;
			s->time = t;
		}
	}

	n = convJ2M(j, buff);
	if(n < 0)
		damaged("convJ2M");

	// Window 4
	err = 2;
	for(;;) {
		err--;
		if(!err)
			dupsect(s, 1);	// Window 4 damage
		t = s->coff + 1;
		if(!writedata(err, s->sect, buff, n, t))
			continue;

		t += n;
		r = n;
		if(x != nil) {
			x->sect = s->sect;
			x->addr = t;
			if(!writedata(err, s->sect, a, j->size, t))
				continue;
			t += j->size;
			r += j->size;
		}

		type[0] = j->type;
		if(!writedata(err, s->sect, type, 1, s->coff))
			continue;
		r++;
		break;
	}
	s->coff = t;
	return r;
}

static void
summarize(void)
{
	Gen *g;
	uchar *b;
	Entry *e;
	Extent *x;
	Jrec j, sum;
	Sect *s, *t;
	ulong off, ctime;
	int n, bytes, more, mtime, sumd;

	g = &gens[eparity];
	s = g->head;
	g->head = s->next;
	if(g->head == nil)
		g->tail = nil;
	g = &gens[eparity^1];
	t = g->tail;
	b = sectbuff;
	x = nil;

	if(debug)
		fprint(2, "summarize\n");

	for(;;) {	// Window 1
		readdata(s->sect, b, sectsize, 0);
		off = s->toff;
		ctime = get4(&b[off]);
		off += 4;
		sumd = 0;

		cursect = s->sect;
		while(b[off] != 0xFF) {
			n = convM2J(&j, &b[off]);
			if(n < 0)
				damaged("bad Jrec");
			if(debug)
				fprint(2, "read %J\n", &j);
			off += n;
			bytes = n;
			mtime = 0;
			switch(j.type) {
			case FT_create:
				ctime += j.mtime;
				mtime = 1;
			create:
				e = elookup(j.fnum);
				if(e == nil)
					continue;
				break;
			case FT_chmod:
				e = elookup(j.fnum);
				if(e == nil || j.mnum != e->mnum)
					continue;
				break;
			case FT_REMOVE:
				e = elookup(j.fnum);
				if(e == nil)
					continue;
				break;
			case FT_trunc:
				ctime += j.mtime;
				mtime = 1;
				e = elookup(j.tnum);
				if(e == nil) {
					if(debug)
						fprint(2, "-> create\n");
					j.type = FT_create;
					goto create;
				}
				break;
			case FT_AWRITE:
				goto write;
			case FT_WRITE:
				ctime += j.mtime;
				mtime = 1;
			write:
				e = elookup(j.fnum);
				if(e == nil) {
					off += j.size;
					continue;
				}
				x = esum(e, s->sect, off, &more);
				if(x == nil) {
					damaged("missing extent");
					off += j.size;
					continue;
				}
				if(more) {
					j.type = FT_AWRITE;
					mtime = 0;
				}
				bytes += j.size;
				break;
			case FT_SUMMARY:
			case FT_SUMBEG:
			case FT_SUMEND:
				continue;
			default:
				damaged("summarize botch");
			}

			if(!sumd) {
				if(t->coff + Nsumbeg >= sectsize - 1)
					t = newsum(g, t->seq + 1);
				sum.type = FT_SUMBEG;
				sum.seq = s->seq;
				if(debug)
					fprint(2, "+ %J\n", &sum);
				t->sum += sputw(t, &sum, 0, nil, nil);
				if(t->sum >= Nsum)
					t->sum = Nsum;
				sumd = 1;
			}

			if(t->coff + bytes >= sectsize - Nsum + t->sum - 1)
				t = newsum(g, t->seq + 1);

			if(mtime)
				j.mtime = ctime;

			switch(j.type) {
			case FT_AWRITE:
			case FT_WRITE:
				sputw(t, &j, mtime, x, &b[off]);
				off += j.size;
				break;
			default:
				sputw(t, &j, mtime, nil, nil);
			}
		}
		cursect = -1;

		if(t->coff + Nsumbeg >= sectsize - 1)
			t = newsum(g, t->seq + 1);
		if(sumd)
			sum.type = FT_SUMEND;
		else {
			sum.type = FT_SUMMARY;
			sum.seq = s->seq;
		}
		if(debug)
			fprint(2, "+ %J\n", &sum);
		t->sum += sputw(t, &sum, 0, nil, nil);
		if(t->sum >= Nsum)
			t->sum = Nsum;

		// Window 2
		freesect(s);
		g = &gens[eparity];
		s = g->head;
		if(s == nil) {
			// Window 3
			g->gnum += 2;
			newsum(g, 0);
			eparity ^= 1;
			return;
		}

		if(nfree >= Nfree)
			return;

		g->head = s->next;
		if(g->head == nil)
			g->tail = nil;
		g = &gens[eparity^1];
	}
}

char *
need(int bytes)
{
	Gen *g;
	int sums;
	Sect *s, *t;

	sums = 0;
	for(;;) {
		s = gens[eparity].tail;
		if(s->coff + bytes < sectsize - Nsum + s->sum - 1)
			return nil;

		if(nfree >= Nfree)
			break;

		if(sums == 2) {
			readonly = 1;
			return "generation full";
		}

		summarize();
		sums++;
	}

	g = &gens[eparity];
	t = getsect();
	t->seq = g->tail->seq + 1;
	newsect(g, t);
	return nil;
}

void
putw(Jrec *j, int mtime, Extent *x, void *a)
{
	sputw(gens[eparity].tail, j, mtime, x, a);
}

void
put(Jrec *j, int mtime)
{
	sputw(gens[eparity].tail, j, mtime, nil, nil);
}
