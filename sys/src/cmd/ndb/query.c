#include <u.h>
#include <libc.h>
#include <bio.h>
#include <ndb.h>

/*
 *  search the database for matches
 */
void
usage(void)
{
	fprint(2, "usage: query attr value [returned attribute]\n");
	exits("usage");
}

void
search(Ndb *db, char *attr, char *val, char *rattr)
{
	Ndbs s;
	Ndbtuple *t;
	Ndbtuple *nt;
	char buf[Ndbvlen];

	if(rattr){
		t = ndbgetval(db, &s, attr, val, rattr, buf);
		if(t){
			print("%s\n", buf);
			ndbfree(t);
		}
		return;
	}

	t = ndbsearch(db, &s, attr, val);
	while(t){
		for(nt = t; nt; nt = nt->entry)
			print("%s=%s ", nt->attr, nt->val);
		print("\n");
		ndbfree(t);
		t = ndbsnext(&s, attr, val);
	}
}

void
main(int argc, char **argv)
{
	char *rattr = 0;
	Ndb *db;
	int reps = 1;

	switch(argc){
	case 5:
		reps = atoi(argv[4]);
		/* fall through */
	case 4:
		rattr = argv[3];
		break;
	case 3:
		rattr = 0;
		break;
	default:
		usage();
	}
	
	db = ndbopen(0);
	if(db == 0){
		fprint(2, "no db files\n");
		exits("no db");
	}
	while(reps--)
		search(db, argv[1], argv[2], rattr);
	ndbclose(db);

	exits(0);
}
