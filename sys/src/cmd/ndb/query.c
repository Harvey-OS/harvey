/*
 *  search the network database for matches
 */
#include <u.h>
#include <libc.h>
#include <bio.h>
#include <ndb.h>

static int all, multiple;
static Biobuf bout;

void
usage(void)
{
	fprint(2, "usage: query [-am] [-f ndbfile] attr value "
		"[returned-attr [reps]]\n");
	exits("usage");
}

/* print values of nt's attributes matching rattr */
static void
prmatch(Ndbtuple *nt, char *rattr)
{
	for(; nt; nt = nt->entry)
		if (strcmp(nt->attr, rattr) == 0)
			Bprint(&bout, "%s\n", nt->val);
}

void
search(Ndb *db, char *attr, char *val, char *rattr)
{
	char *p;
	Ndbs s;
	Ndbtuple *t, *nt;

	/* first entry with a matching rattr */
	if(rattr && !all){
		p = ndbgetvalue(db, &s, attr, val, rattr, &t);
		if (multiple)
			prmatch(t, rattr);
		else if(p)
			Bprint(&bout, "%s\n", p);
		ndbfree(t);
		free(p);
		return;
	}

	/* all entries with matching rattrs */
	if(rattr) {
		for(t = ndbsearch(db, &s, attr, val); t != nil;
		    t = ndbsnext(&s, attr, val)){
			prmatch(t, rattr);
			ndbfree(t);
		}
		return;
	}

	/* all entries */
	for(t = ndbsearch(db, &s, attr, val); t; t = ndbsnext(&s, attr, val)){
		for(nt = t; nt; nt = nt->entry)
			Bprint(&bout, "%s=%s ", nt->attr, nt->val);
		Bprint(&bout, "\n");
		ndbfree(t);
	}
}

void
main(int argc, char **argv)
{
	int reps = 1;
	char *rattr = nil, *dbfile = nil;
	Ndb *db;
	
	ARGBEGIN{
	case 'a':
		all++;
		break;
	case 'm':
		multiple++;
		break;
	case 'f':
		dbfile = EARGF(usage());
		break;
	default:
		usage();
	}ARGEND;

	switch(argc){
	case 4:
		reps = atoi(argv[3]);	/* wtf use is this? */
		/* fall through */
	case 3:
		rattr = argv[2];
		break;
	case 2:
		rattr = nil;
		break;
	default:
		usage();
	}

	if(Binit(&bout, 1, OWRITE) == -1)
		sysfatal("Binit: %r");
	db = ndbopen(dbfile);
	if(db == nil){
		fprint(2, "%s: no db files\n", argv0);
		exits("no db");
	}
	while(reps--)
		search(db, argv[0], argv[1], rattr);
	ndbclose(db);

	exits(0);
}
