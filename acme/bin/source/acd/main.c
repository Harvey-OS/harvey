#include "acd.h"

int debug;

void
usage(void)
{
	fprint(2, "usage: acd dev\n");
	threadexitsall("usage");
}

Alt
mkalt(Channel *c, void *v, int op)
{
	Alt a;

	memset(&a, 0, sizeof(a));
	a.c = c;
	a.v = v;
	a.op = op;
	return a;
}

void
freetoc(Toc *t)
{
	int i;

	free(t->title);
	for(i=0; i<t->ntrack; i++)
		free(t->track[i].title);
}

void
eventwatcher(Drive *d)
{
	enum { STATUS, WEVENT, TOCDISP, DBREQ, DBREPLY, NALT };
	Alt alts[NALT+1];
	Toc nt, tdb;
	Event *e;
	Window *w;
	Cdstatus s;
	char buf[40];

	w = d->w;

	alts[STATUS] = mkalt(d->cstatus, &s, CHANRCV);
	alts[WEVENT] = mkalt(w->cevent, &e, CHANRCV);
	alts[TOCDISP] = mkalt(d->ctocdisp, &nt, CHANRCV);
	alts[DBREQ] = mkalt(d->cdbreq, &tdb, CHANNOP);
	alts[DBREPLY] = mkalt(d->cdbreply, &nt, CHANRCV);
	alts[NALT] = mkalt(nil, nil, CHANEND);
	for(;;) {
		switch(alt(alts)) {
		case STATUS:
			//DPRINT(2, "s...");
			d->status = s;
			if(s.state == Scompleted) {
				s.state = Sunknown;
				advancetrack(d, w);
			}
			//DPRINT(2, "status %d %d %d %M %M\n", s.state, s.track, s.index, s.abs, s.rel);
			sprint(buf, "%d:%2.2d", s.rel.m, s.rel.s);
			setplaytime(w, buf);
			break;
		case WEVENT:
			//DPRINT(2, "w...");
			acmeevent(d, w, e);
			break;
		case TOCDISP:
			//DPRINT(2,"td...");
			freetoc(&d->toc);
			d->toc = nt;
			drawtoc(w, d, &d->toc);
			tdb = nt;
			alts[DBREQ].op = CHANSND;
			break;
		case DBREQ:	/* sent */
			//DPRINT(2,"dreq...");
			alts[DBREQ].op = CHANNOP;
			break;
		case DBREPLY:
			//DPRINT(2,"drep...");
			freetoc(&d->toc);
			d->toc = nt;
			redrawtoc(w, &d->toc);
			break;
		}
	}
}

void
threadmain(int argc, char **argv)
{
	Scsi *s;
	Drive *d;
	char buf[80];

	ARGBEGIN{
	case 'v':
		debug++;
		scsiverbose++;
	}ARGEND

	if(argc != 1)
		usage();

	fmtinstall('M', msfconv);

	if((s = openscsi(argv[0])) == nil)
		error("opening scsi: %r");

	d = malloc(sizeof(*d));
	if(d == nil)
		error("out of memory");
	memset(d, 0, sizeof d);

	d->scsi = s;
	d->w = newwindow();
	d->ctocdisp = chancreate(sizeof(Toc), 0);
	d->cdbreq = chancreate(sizeof(Toc), 0);
	d->cdbreply = chancreate(sizeof(Toc), 0);
	d->cstatus = chancreate(sizeof(Cdstatus), 0);

	proccreate(wineventproc, d->w, STACK);
	proccreate(cddbproc, d, STACK);
	proccreate(cdstatusproc, d, STACK);

	cleanname(argv[0]);
	snprint(buf, sizeof(buf), "%s/", argv[0]);
	winname(d->w, buf);

	wintagwrite(d->w, "Stop Pause Resume Eject Ingest ", 5+6+7+6+7);
	eventwatcher(d);
}
