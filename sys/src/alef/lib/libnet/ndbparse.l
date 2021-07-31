#include <alef.h>
#include <bio.h>
#include <net.h>
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

intern Ndbtuple *tfree;

#define ISWHITE(x) ((x) == ' ' || (x) == '\t')
#define EATWHITE(x) while(ISWHITE(*(x)))(x)++

/*
 *  parse a single tuple
 */
intern byte*
parsetuple(byte *cp, Ndbtuple **tp)
{
	byte *p;
	int len;
	Ndbtuple *t;

	/* a '#' starts a comment lasting till new line */
	EATWHITE(cp);
	if(*cp == '#' || *cp == '\n')
		return nil;

	/* we keep our own free list to reduce mallocs */
	if(tfree){
		t = tfree;
		tfree = tfree->entry;
	} else {
		t = malloc(sizeof(Ndbtuple));
		if(t == nil)
			return nil;
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
intern Ndbtuple*
parseline(byte *cp)
{
	Ndbtuple *t;
	Ndbtuple *first, *last;

	first = nil;
	last = nil;
	while(*cp != '#' && *cp != '\n'){
		t = nil;
		cp = parsetuple(cp, &t);
		if(cp == nil)
			break;
		if(first){
			last->line = t;
			last->entry = t;
		} else
			first = t;
		last = t;
		t->line = nil;
		t->entry = nil;
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
	byte *line;
	Ndbtuple *t;
	Ndbtuple *first, *last;
	int len;

	last = nil;
	first = nil;
	for(;;){
		if(db->line){
			line = db->line;
			db->line = nil;
			len = db->linelen;
		} else {
			line = db->rdline('\n');
			if(line == nil)
				break;
			len = db->Biobufhdr.linelen();
			if(line[len-1] != '\n')
				break;
		}
		if(first != nil && !ISWHITE(*line)){
			db->line = line;
			db->linelen = len;
			return first;
		}
		db->offset += len;
		t = parseline(line);
		if(t == nil)
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

	if(t == nil)
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
intern void
hffree(Ndb *db)
{
	Ndbhf *hf, *next;

	for(hf = db->hf; hf; hf = next){
		next = hf->next;
		close(hf->fd);
		free(hf);
	}
	db->hf = nil;
}

int
ndbreopen(Ndb *db)
{
	int fd;
	Dir d;

	/* forget what we know about the open files */
	if(db->mtime){
		hffree(db);
		db->term();
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
	db->line = nil;
	db->offset = 0;
	db->inits(fd, OREAD, db->buf, sizeof(db->buf));
	return 0;
}

intern byte *deffile = "/lib/ndb/local";

/*
 *  either the file name is passed as a parameter or we use /lib/ndb/local
 *  and lookup the list of files to use
 */
Ndb*
ndbopen(byte *file)
{
	Ndb *db, *first, *last;
	Ndbs s;
	Ndbtuple *t, *nt;

	if(file == nil){
		db = ndbopen(deffile);
		if(db == nil)
			return nil;
		first = last = db;
		t = ndbsearch(db, &s, "database", "");
		if(t == nil)
			return db;
		for(nt = t; nt; nt = nt->entry){
			if(strcmp(nt->attr, "file") != 0)
				continue;
			if(strcmp(nt->val, deffile) == 0){
				/* default file can be reordered in the list */
				if(first->next == nil)
					continue;
				if(strcmp(first->file, deffile) == 0){
					db = first;
					first = first->next;
					last->next = db;
					db->next = nil;
					last = db;
				}
				continue;
			}
			db = ndbopen(nt->val);
			if(db == nil)
				continue;
			last->next = db;
			last = db;
		}
		return first;
	}

	db = malloc(sizeof(Ndb));
	if(db == nil)
		return nil;
	strncpy(db->file, file, sizeof(db->file)-1);
	db->next = nil;
	db->hf = nil;
	db->mtime = 0;

	if(ndbreopen(db) < 0){
		free(db);
		return nil;
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
		db->term();
		free(db);
	}
}

int
ndbseek(Ndb *db, int off, int whence)
{
	if(whence == 0 && off == db->offset)
		return off;

	db->line = nil;
	db->offset = off;
	return db->seek(off, whence);
}
