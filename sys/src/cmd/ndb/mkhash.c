#include <u.h>
#include <libc.h>
#include <bio.h>
#include <ndb.h>

/*
 *  make the hash table completely in memory and then write as a file
 */

uchar *ht;
ulong hlen;
Ndb *db;
ulong nextchain;

char*
syserr(void)
{
	static char buf[ERRLEN];

	errstr(buf);
	return buf;
}

void
enter(char *val, ulong dboff)
{
	ulong h;
	uchar *last;
	ulong ptr;

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

uchar nbuf[8*1024];

void
main(int argc, char **argv)
{
	Ndbtuple *t, *nt;
	int n;
	Dir d;	
	uchar buf[8];
	char file[3*NAMELEN+2];
	int fd;
	ulong off;
	uchar *p;

	if(argc != 3){
		fprint(2, "mkhash: usage file attribute\n");
		exits("usage");
	}
	db = ndbopen(argv[1]);
	if(db == 0){
		fprint(2, "mkhash: can't open %s\n", argv[1]);
		exits(syserr());
	}
	Binits(db, Bfildes(db), OREAD, nbuf, sizeof(nbuf));
	if(dirstat(argv[1], &d) < 0){
		fprint(2, "mkhash: can't stat %s\n", argv[1]);
		exits(syserr());
	}

	/* count entries to calculate hash size */
	n = 0;
	while(nt = ndbparse(db)){
		for(t = nt; t; t = t->entry){
			if(strcmp(t->attr, argv[2]) == 0){
				n++;
				break;
			}
		}
		ndbfree(nt);
	}

	/* allocate an array large enough for worst case */
	hlen = 2*n+1;
	n = hlen*NDBPLEN + hlen*2*NDBPLEN;
	ht = malloc(n);
	if(ht == 0){
		fprint(2, "mkhash: not enough memory\n");
		exits(syserr());
	}
	for(p = ht; p < &ht[n]; p += NDBPLEN)
		NDBPUTP(NDBNAP, p);
	nextchain = hlen*NDBPLEN;

	/* create the in core hash table */
	ndbseek(db, 0, 0);
	off = db->offset;
	while(nt = ndbparse(db)){
		for(t = nt; t; t = t->entry){
			if(strcmp(t->attr, argv[2]) == 0)
				enter(t->val, off);
		}
		ndbfree(nt);
		off = db->offset;
	}

	/* create the hash file */
	snprint(file, sizeof(file), "%s.%s", argv[1], argv[2]);
	fd = create(file, ORDWR, 0664);
	if(fd < 0){
		fprint(2, "mkhash: can't create %s\n", file);
		exits(syserr());
	}
	NDBPUTUL(d.mtime, buf);
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

	exits(0);
}
