/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include <u.h>
#include <libc.h>
#include <bio.h>
#include <ndb.h>

/*
 *  make the hash table completely in memory and then write as a file
 */

uint8_t *ht;
uint32_t hlen;
Ndb *db;
uint32_t nextchain;

char*
syserr(void)
{
	static char buf[ERRMAX];

	errstr(buf, sizeof buf);
	return buf;
}

void
enter(char *val, uint32_t dboff)
{
	uint32_t h;
	uint8_t *last;
	uint32_t ptr;

	h = ndbhash(val, hlen);
	h *= NDBPLEN;
	last = &ht[h];
	ptr = NDBGETP(last);
	if(ptr == NDBNAP){
		NDBPUTP(dboff, last);
		return;
	}

	if(ptr & NDBCHAIN){
		/* walk the chain to the last entry */
		for(;;){
			ptr &= ~NDBCHAIN;
			last = &ht[ptr+NDBPLEN];
			ptr = NDBGETP(last);
			if(ptr == NDBNAP){
				NDBPUTP(dboff, last);
				return;
			}
			if(!(ptr & NDBCHAIN)){
				NDBPUTP(nextchain|NDBCHAIN, last);
				break;
			}
		}
	} else
		NDBPUTP(nextchain|NDBCHAIN, last);

	/* add a chained entry */
	NDBPUTP(ptr, &ht[nextchain]);
	NDBPUTP(dboff, &ht[nextchain + NDBPLEN]);
	nextchain += 2*NDBPLEN;
}

uint8_t nbuf[16*1024];

void
main(int argc, char **argv)
{
	Ndbtuple *t, *nt;
	int n;
	Dir *d;	
	uint8_t buf[8];
	char file[128];
	int fd;
	uint32_t off;
	uint8_t *p;

	if(argc != 3){
		fprint(2, "usage: mkhash file attribute\n");
		exits("usage");
	}
	db = ndbopen(argv[1]);
	if(db == 0){
		fprint(2, "mkhash: can't open %s\n", argv[1]);
		exits(syserr());
	}

	/* try a bigger than normal buffer */
	Binits(&db->b, Bfildes(&db->b), OREAD, nbuf, sizeof(nbuf));

	/* count entries to calculate hash size */
	n = 0;

	while((nt = ndbparse(db)) != nil){
		for(t = nt; t; t = t->entry){
			if(strcmp(t->attr, argv[2]) == 0)
				n++;
		}
		ndbfree(nt);
	}

	/* allocate an array large enough for worst case */
	hlen = 2*n+1;
	n = hlen*NDBPLEN + hlen*2*NDBPLEN;
	ht = mallocz(n, 1);
	if(ht == 0){
		fprint(2, "mkhash: not enough memory\n");
		exits(syserr());
	}
	for(p = ht; p < &ht[n]; p += NDBPLEN)
		NDBPUTP(NDBNAP, p);
	nextchain = hlen*NDBPLEN;

	/* create the in core hash table */
	Bseek(&db->b, 0, 0);
	off = 0;
	while((nt = ndbparse(db)) != nil){
		for(t = nt; t != nil; t = t->entry){
			if(strcmp(t->attr, argv[2]) == 0)
				enter(t->val, off);
		}
		ndbfree(nt);
		off = Boffset(&db->b);
	}

	/* create the hash file */
	snprint(file, sizeof(file), "%s.%s", argv[1], argv[2]);
	fd = create(file, ORDWR, 0664);
	if(fd < 0){
		fprint(2, "mkhash: can't create %s\n", file);
		exits(syserr());
	}
	NDBPUTUL(db->mtime, buf);
	NDBPUTUL(hlen, buf+NDBULLEN);
	if(write(fd, buf, NDBHLEN) != NDBHLEN){
		fprint(2, "mkhash: writing %s\n", file);
		exits(syserr());
	}
	if(write(fd, ht, nextchain) != nextchain){
		fprint(2, "mkhash: writing %s\n", file);
		exits(syserr());
	}
	close(fd);

	/* make sure file didn't change while we were making the hash */
	d = dirstat(argv[1]);
	if(d == nil || d->qid.path != db->qid.path
	   || d->qid.vers != db->qid.vers){
		fprint(2, "mkhash: %s changed underfoot\n", argv[1]);
		remove(file);
		exits("changed");
	}

	exits(0);
}
