#include <u.h>
#include <libc.h>
#include <bio.h>
#include <ctype.h>
#include <ndb.h>
#include "ndbhf.h"

/*
 *  Parse a data base entry.  Entries may span multiple
 *  lines.  An entry starts on a left margin.  All subsequent
 *  lines must be indented by white space.  An entry consists
 *  of tuples of the forms:
 *	attribute-name
 *	attribute-name=value
 *	attribute-name="value with white space"
 *
 *  The parsing returns a 2-dimensional structure.  The first
 *  dimension joins all tuples. All tuples on the same line
 *  form a ring along the second dimension.
 */

/*
 *  parse the next entry in the file
 */
Ndbtuple*
ndbparse(Ndb *db)
{
	char *line;
	Ndbtuple *t;
	Ndbtuple *first, *last;
	int len;

	first = last = 0;
	for(;;){
		if((line = Brdline(&db->b, '\n')) == 0)
			break;
		len = Blinelen(&db->b);
{
extern int debug;
if(debug)
print("read %.*s", len, line);
}
		if(line[len-1] != '\n')
			break;
		if(first && !ISWHITE(*line) && *line != '#'){
			Bseek(&db->b, -len, 1);
			return first;
		}
		t = _ndbparseline(line);
		if(t == 0)
			continue;
		if(first)
			last->entry = t;
		else
			first = t;
		last = t;
		while(last->entry)
			last = last->entry;
	}
	return first;
}


/*
 *  free the hash files belonging to a db
 */
static void
hffree(Ndb *db)
{
	Ndbhf *hf, *next;

	for(hf = db->hf; hf; hf = next){
		next = hf->next;
		close(hf->fd);
		free(hf);
	}
	db->hf = 0;
}

int
ndbreopen(Ndb *db)
{
	int fd;
	Dir d;

	/* forget what we know about the open files */
	if(db->mtime){
		hffree(db);
		close(Bfildes(&db->b));
		Bterm(&db->b);
		db->mtime = 0;
	}

	/* try the open again */
	fd = open(db->file, OREAD);
	if(fd < 0)
		return -1;
	if(dirfstat(fd, &d) < 0){
		close(fd);
		return -1;
	}

	db->qid = d.qid;
	db->mtime = d.mtime;
	Binits(&db->b, fd, OREAD, db->buf, sizeof(db->buf));
	return 0;
}

static char *deffile = "/lib/ndb/local";

/*
 *  lookup the list of files to use in the specified database file
 */
static Ndb*
doopen(char *file)
{
	Ndb *db;

	db = (Ndb*)malloc(sizeof(Ndb));
	if(db == 0)
		return 0;
	strncpy(db->file, file, sizeof(db->file)-1);
	db->next = 0;
	db->hf = 0;
	db->mtime = 0;

	if(ndbreopen(db) < 0){
		free(db);
		return 0;
	}

	return db;
}
Ndb*
ndbopen(char *file)
{
	Ndb *db, *first, *last;
	Ndbs s;
	Ndbtuple *t, *nt;

	if(file == 0)
		file = deffile;
	db = doopen(file);
	if(db == 0)
		return 0;
	first = last = db;
	t = ndbsearch(db, &s, "database", "");
	Bseek(&db->b, 0, 0);
	if(t == 0)
		return db;
	for(nt = t; nt; nt = nt->entry){
		if(strcmp(nt->attr, "file") != 0)
			continue;
		if(strcmp(nt->val, file) == 0){
			/* default file can be reordered in the list */
			if(first->next == 0)
				continue;
			if(strcmp(first->file, file) == 0){
				db = first;
				first = first->next;
				last->next = db;
				db->next = 0;
				last = db;
			}
			continue;
		}
		db = doopen(nt->val);
		if(db == 0)
			continue;
		last->next = db;
		last = db;
	}
	return first;
}

void
ndbclose(Ndb *db)
{
	Ndb *nextdb;

	for(; db; db = nextdb){
		nextdb = db->next;
		hffree(db);
		close(Bfildes(&db->b));
		Bterm(&db->b);
		free(db);
	}
}
