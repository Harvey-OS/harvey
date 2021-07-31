#include <u.h>
#include <libc.h>
#include <bio.h>
#include <ctype.h>
#include "ndb.h"
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

static Ndbtuple *tfree;

#define ISWHITE(x) ((x) == ' ' || (x) == '\t')
#define EATWHITE(x) while(ISWHITE(*(x)))(x)++

/*
 *  parse a single tuple
 */
static char*
parsetuple(char *cp, Ndbtuple **tp)
{
	char *p;
	int len;
	Ndbtuple *t;

	/* a '#' starts a comment lasting till new line */
	EATWHITE(cp);
	if(*cp == '#' || *cp == '\n')
		return 0;

	/* we keep our own free list to reduce mallocs */
	if(tfree){
		t = tfree;
		tfree = tfree->entry;
	} else {
		t = malloc(sizeof(Ndbtuple));
		if(t == 0)
			return 0;
	}
	memset(t, 0, sizeof(*t));
	*tp = t;

	/* parse attribute */
	p = cp;
	while(*cp != '=' && !ISWHITE(*cp) && *cp != '\n')
		cp++;
	len = cp - p;
	if(len >= Ndbalen)
		len = Ndbalen;
	strncpy(t->attr, p, len);

	/* parse value */
	EATWHITE(cp);
	if(*cp == '='){
		cp++;
		EATWHITE(cp);
		if(*cp == '"'){
			p = ++cp;
			while(*cp != '\n' && *cp != '"')
				cp++;
			len = cp - p;
			if(*cp == '"')
				cp++;
		} else {
			p = cp;
			while(!ISWHITE(*cp) && *cp != '\n')
				cp++;
			len = cp - p;
		}
		if(len >= Ndbvlen)
			len = Ndbvlen;
		strncpy(t->val, p, len);
	}

	return cp;
}

/*
 *  parse all tuples in a line.  we assume that the 
 *  line ends in a '\n'.
 *
 *  the tuples are linked as a list using ->entry and
 *  as a ring using ->line.
 */
static Ndbtuple*
parseline(char *cp)
{
	Ndbtuple *t;
	Ndbtuple *first, *last;

	first = last = 0;
	while(*cp != '#' && *cp != '\n'){
		t = 0;
		cp = parsetuple(cp, &t);
		if(cp == 0)
			break;
		if(first){
			last->line = t;
			last->entry = t;
		} else
			first = t;
		last = t;
		t->line = 0;
		t->entry = 0;
	}
	if(first)
		last->line = first;
	return first;
}

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
		if(db->line){
			line = db->line;
			db->line = 0;
			len = db->linelen;
		} else {
			if((line = Brdline(db, '\n')) == 0)
				break;
			len = Blinelen(db);
			if(line[len-1] != '\n')
				break;
		}
		if(first && !ISWHITE(*line)){
			db->line = line;
			db->linelen = len;
			return first;
		}
		db->offset += len;
		t = parseline(line);
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
 *  free a parsed entry
 */
void
ndbfree(Ndbtuple *t)
{
	Ndbtuple *tn;

	if(t == 0)
		return;
	for(; t; t = tn){
		tn = t->entry;
		t->entry = tfree;
		tfree = t;
	}
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
		Bclose(db);
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
	db->line = 0;
	db->offset = 0;
	Binits(db, fd, OREAD, db->buf, sizeof(db->buf));
	return 0;
}

Ndb*
ndbopen(char *file)
{
	Ndb *db, *gdb;

	if(file == 0){
		db = ndbopen("/lib/ndb/local");
		gdb = ndbopen("/lib/ndb/global");
		if(db){
			db->next = gdb;
			return db;
		}else
			return gdb;
	}

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

void
ndbclose(Ndb *db)
{
	Ndb *nextdb;

	for(; db; db = nextdb){
		nextdb = db->next;
		hffree(db);
		Bclose(db);
		free(db);
	}
}

long
ndbseek(Ndb *db, long off, int whence)
{
	if(whence == 0 && off == db->offset)
		return off;

	db->line = 0;
	db->offset = off;
	return Bseek(db, off, whence);
}
