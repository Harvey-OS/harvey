#include <alef.h>
#include <bio.h>
#include <net.h>
#include "ndbhf.h"

enum {
	Dptr,	/* pointer to database file */
	Cptr,	/* pointer to first chain entry */
	Cptr1,	/* pointer to second chain entry */
};

/*
 *  generate a hash value for an ascii string (val) given
 *  a hash table length (hlen)
 */
uint
ndbhash(byte *vp, int hlen)
{
	uint hash;
	byte *val;

	val = (byte*)vp;
	for(hash = 0; *val; val++)
		hash = (hash*13) + *val-'a';
	return hash % hlen;
}

/*
 *  read a hash file with buffering
 */
intern byte*
hfread(Ndbhf *hf, int off, int len)
{
	if(off < hf->off || off + len > hf->off + hf->len){
		if(seek(hf->fd, off, 0) < 0
		|| (hf->len = read(hf->fd, hf->buf, sizeof(hf->buf))) < len){
			hf->off = -1;
			return nil;
		}
		hf->off = off;
	}
	return &hf->buf[off-hf->off];
}

/*
 *  return an opened hash file if one exists for the
 *  attribute and if it is current vis-a-vis the data
 *  base file
 */
intern Ndbhf*
hfopen(Ndb *db, byte *attr)
{
	Ndbhf *hf;
	byte buf[sizeof(hf->attr)+sizeof(db->file)+2];
	byte *p;
	Dir d;

	/* try opening the data base if it's closed */
	if(db->mtime==0 && ndbreopen(db) < 0)
		return nil;

	/* if the database has changed, throw out hash files and reopen db */
	if(dirfstat(db->fildes(), &d) < 0 || db->qid.path != d.qid.path
	|| db->qid.vers != d.qid.vers){
		if(ndbreopen(db) < 0)
			return nil;
	};

	/* see if a hash file exists for this attribute */
	for(hf = db->hf; hf; hf= hf->next){
		if(strcmp(hf->attr, attr) == 0)
			return hf;
	}

	/* create a new one */
	hf = (Ndbhf*)malloc(sizeof(Ndbhf));
	if(hf == nil)
		return nil;

	/* compare it to the database file */
	strncpy(hf->attr, attr, sizeof(hf->attr)-1);
	sprint(buf, "%s.%s", db->file, hf->attr);
	hf->fd = open(buf, OREAD);
	if(hf->fd >= 0){
		hf->len = 0;
		hf->off = 0;
		p = hfread(hf, 0, 2*NDBULLEN);
		if(p){
			hf->dbmtime = NDBGETUL(p);
			hf->hlen = NDBGETUL(p+NDBULLEN);
			if(hf->dbmtime == db->mtime){
				hf->next = db->hf;
				db->hf = hf;
				return hf;
			}
		}
		close(hf->fd);
	}
	free(hf);
	return nil;
}

/*
 *  return the first matching entry
 */
Ndbtuple*
ndbsearch(Ndb *db, Ndbs *s, byte *attr, byte *val)
{
	byte *p;

	s->hf = hfopen(db, attr);
	if(s->hf){
		s->ptr = ndbhash(val, s->hf->hlen)*NDBPLEN;
		p = hfread(s->hf, s->ptr+NDBHLEN, NDBPLEN);
		if(p == nil)
			return nil;
		s->ptr = NDBGETP(p);
		s->type = Cptr1;
	} else {
		s->ptr = 0;
		s->type = Dptr;
	}
	s->db = db;
	return ndbsnext(s, attr, val);
}

intern Ndbtuple*
match(Ndbtuple *t, byte *attr, byte *val)
{
	Ndbtuple *nt;

	for(nt = t; nt; nt = nt->entry)
		if(strcmp(attr, nt->attr) == 0
		&& strcmp(val, nt->val) == 0)
			return nt;
	return nil;
}

/*
 *  return the next matching entry in the hash chain
 */
Ndbtuple*
ndbsnext(Ndbs *s, byte *attr, byte *val)
{
	Ndbtuple *t;
	Ndb *db;
	byte *p;

/* fprint(2, "%s %s %d %lux\n", s->db->file, attr, s->type, s->ptr);/**/

	db = s->db;
	if(s->ptr == NDBNAP)
		goto nextfile;

	for(;;){
		if(s->type == Dptr){
			if(ndbseek(db, s->ptr, 0) < 0)
				break; 
			t = ndbparse(db);
			s->ptr = db->offset;
			if(t == nil)
				break;
			if(s->t = match(t, attr, val))
				return t;
			ndbfree(t);
		} else if(s->type == Cptr){
			if(ndbseek(db, s->ptr, 0) < 0)
				break; 
			s->ptr = s->ptr1;
			s->type = Cptr1;
			t = ndbparse(db);
			if(t == nil)
				break;
			if(s->t = match(t, attr, val))
				return t;
			ndbfree(t);
		} else if(s->type == Cptr1){
			if(s->ptr & NDBCHAIN){	/* hash chain continuation */
				s->ptr &= ~NDBCHAIN;
				p = hfread(s->hf, s->ptr+NDBHLEN, 2*NDBPLEN);
				if(p == nil)
					break;
				s->ptr = NDBGETP(p);
				s->ptr1 = NDBGETP(p+NDBPLEN);
				s->type = Cptr;
			} else {		/* end of hash chain */
				if(ndbseek(db, s->ptr, 0) < 0)
					break; 
				s->ptr = NDBNAP;
				t = ndbparse(db);
				if(t == nil)
					break;
				if(s->t = match(t, attr, val))
					return t;
				ndbfree(t);
				break;
			}
		}
	}

nextfile:

	/* nothing left to search? */
	s->ptr = NDBNAP;
	if(db->next == nil)
		return nil;

	/* advance search to next db file */
	return ndbsearch(db->next, s, attr, val);
}
