#include "acd.h"
#include <ctype.h>

/* see CDDBPROTO */
static ulong 
cddb_sum(int n)
{
	int ret;
	ret = 0;
	while(n > 0) {
		ret += n%10;
		n /= 10;
	}
	return ret;
}

static ulong
diskid(Toc *t)
{
	int i, n, tmp;
	Msf *ms, *me;

	n = 0;
	for(i=0; i < t->ntrack; i++)
		n += cddb_sum(t->track[i].start.m*60+t->track[i].start.s);

	ms = &t->track[0].start;
	me = &t->track[t->ntrack].start;
	tmp = (me->m*60+me->s) - (ms->m*60+ms->s);

	/*
	 * the spec says n%0xFF rather than n&0xFF.  it's unclear which is correct.
	 * most CDs are in the database under both entries.
	 */
	return ((n & 0xFF) << 24 | (tmp << 8) | t->ntrack);
}

static void
append(char **d, char *s)
{
	char *r;
	if (*d == nil)
		*d = estrdup(s);
	else {
		r = emalloc(strlen(*d) + strlen(s) + 1);
		strcpy(r, *d);
		strcat(r, s);
		free(*d);
		*d = r;
	}
}

static int
cddbfilltoc(Toc *t)
{
	int fd;
	int i;
	char *p, *q;
	Biobuf bin;
	Msf *m;
	char *f[10];
	int nf;
	char *id, *categ;
	char gottrack[MTRACK];
	int gottitle;

	fd = dial("tcp!freedb.freedb.org!888", 0, 0, 0);
	if(fd < 0) {
		fprint(2, "cannot dial: %r\n");
		return -1;
	}
	Binit(&bin, fd, OREAD);

	if((p=Brdline(&bin, '\n')) == nil || atoi(p)/100 != 2) {
	died:
		close(fd);
		Bterm(&bin);
		fprint(2, "error talking to server\n");
		if(p) {
			p[Blinelen(&bin)-1] = 0;
			fprint(2, "server says: %s\n", p);
		}
		return -1;
	}

	fprint(fd, "cddb hello gre plan9 9cd 1.0\r\n");
	if((p = Brdline(&bin, '\n')) == nil || atoi(p)/100 != 2)
		goto died;

	fprint(fd, "cddb query %8.8lux %d", diskid(t), t->ntrack);
	DPRINT(2, "cddb query %8.8lux %d", diskid(t), t->ntrack);
	for(i=0; i<t->ntrack; i++) {
		m = &t->track[i].start;
		fprint(fd, " %d", (m->m*60+m->s)*75+m->f);
		DPRINT(2, " %d", (m->m*60+m->s)*75+m->f);
	}
	m = &t->track[t->ntrack-1].end;
	fprint(fd, " %d\r\n", m->m*60+m->s);
	DPRINT(2, " %d\r\n", m->m*60+m->s);

	if((p = Brdline(&bin, '\n')) == nil || atoi(p)/100 != 2)
		goto died;
	p[Blinelen(&bin)-1] = 0;
	DPRINT(2, "cddb: %s\n", p);
	nf = tokenize(p, f, nelem(f));
	if(nf < 1)
		goto died;

	switch(atoi(f[0])) {
	case 200:	/* exact match */
		if(nf < 3)
			goto died;
		categ = f[1];
		id = f[2];
		break;
	case 211:	/* close matches */
		if((p = Brdline(&bin, '\n')) == nil)
			goto died;
		if(p[0] == '.')	/* no close matches? */
			goto died;
		p[Blinelen(&bin)-1] = '\0';

		/* accept first match */
		nf = tokenize(p, f, nelem(f));
		if(nf < 2)
			goto died;
		categ = f[0];
		id = f[1];

		/* snarf rest of buffer */
		while(p[0] != '.') {
			if((p = Brdline(&bin, '\n')) == nil)
				goto died;
			p[Blinelen(&bin)-1] = '\0';
			DPRINT(2, "cddb: %s\n", p);
		}
		break;
	case 202: /* no match */
	default:
		goto died;
	}

	/* fetch results for this cd */
	fprint(fd, "cddb read %s %s\r\n", categ, id);

	memset(gottrack, 0, sizeof(gottrack));
	gottitle = 0;
	do {
		if((p = Brdline(&bin, '\n')) == nil)
			goto died;
		q = p+Blinelen(&bin)-1;
		while(isspace(*q))
			*q-- = 0;
DPRINT(2, "cddb %s\n", p);
		if(strncmp(p, "DTITLE=", 7) == 0) {
			if (gottitle)
				append(&t->title, p + 7);
			else
				t->title = estrdup(p+7);
			gottitle = 1;
		} else if(strncmp(p, "TTITLE", 6) == 0 && isdigit(p[6])) {
			i = atoi(p+6);
			if(i < t->ntrack) {
				p += 6;
				while(isdigit(*p))
					p++;
				if(*p == '=')
					p++;

				if (gottrack[i])
					append(&t->track[i].title, p);
				else
					t->track[i].title = estrdup(p);
				gottrack[i] = 1;
			}
		} 
	} while(*p != '.');

	fprint(fd, "quit\r\n");
	close(fd);
	Bterm(&bin);

	return 0;
}

void
cddbproc(void *v)
{
	Drive *d;
	Toc t;

	threadsetname("cddbproc");
	d = v;
	while(recv(d->cdbreq, &t))
		if(cddbfilltoc(&t) == 0)
			send(d->cdbreply, &t);
}
