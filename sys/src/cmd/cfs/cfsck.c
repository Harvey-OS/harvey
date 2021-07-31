#include <u.h>
#include <libc.h>

#include "cformat.h"
#include "lru.h"
#include "bcache.h"
#include "disk.h"
#include "inode.h"
#include "file.h"

Icache	ic;
int	debug;

void	cachesetup(char*);
void	pointers(void);
void	lost(void);
void	error(char*);
void	warning(char*);
char*	mark(ulong);
int	ninodes(void);
ulong	nused(void);


/*
 *  bit masks of allocated blocks
 */
typedef struct Balloc	Balloc;
struct Balloc
{
	Dahdr;
	ulong	bits[2*1024];
};
Balloc ondisk[4];
Balloc offdisk[4];

void
usage(void)
{
	fprint(2, "cfsck [-d] [-p struct data]\n");
}

main(int argc, char *argv[])
{
	char *part;

	part = "/dev/sdC0/cache";

	ARGBEGIN{
	case 'd':
		debug = 1;
		break;
	default:
		usage();
	}ARGEND

	if(argc > 0)
		part = argv[0];
	cachesetup(part);

	pointers();

	lost();

	return 0;
}

/*
 *  setup the disk cache
 */
void
cachesetup(char *disk)
{
	int f;
	int psize;
	int i;

	psize = 512;

	f = open(disk, OREAD);
	if(f < 0)
		error("opening disk");

	if(iinit(&ic, f, psize)<0)
		error("initing cache");

	print("%d bytes/logical block\n", ic.bsize);
	print("%d allocation block(s)\n", ic.nab);
	print("%d inode blocks\n", ic.nib);

	for(i = 0; i < ic.nab; i++)
		bread(&ic, i, &ondisk[i]);

	for(i = 0; i < ic.nab + ic.nib; i++)
		mark(i);

	print("%d inodes, %d used\n", ic.nino, ninodes());
	print("%d blocks, %d used\n", ic.nb, nused());
}

void
error(char *s)
{
	fprint(2, "cfsck: %s: ", s);
	perror("");
	exits(s);
}

void
warning(char *s)
{
	char buf[ERRLEN];

	errstr(buf);
	fprint(2, "cfsck: %s: %s\n", s, buf);
	if(strstr(buf, "illegal network address"))
		exits("death");
}

char*
mark(ulong bno)
{
	int ab;
	int off;
	ulong m;

	ab = bno/ic.b2b;
	if(ab > ic.nab)
		return "bad block number";
	bno -= ab*ic.b2b;
	off = bno/BtoUL;
	m = 1<<(bno%BtoUL);
	if((ondisk[ab].bits[off] & m) == 0)
		return "used but not allocated";
	if(offdisk[ab].bits[off] & m)
		return "multiply used";
	offdisk[ab].bits[off] |= m;
	return 0;
}

void
walk(Ibuf *ip, Dptr *p, int ind)
{
	char buf[8*1024];
	Dptr *dp;
	char *s;
	int i;

	if(p->bno == Notabno)
		return;
	if(!(p->bno & Indbno)){
		if(s = mark(p->bno))
			fprint(2, "%s bno %lud qid %lux.%lux fbno %lud\n",
				s, p->bno, ip->inode.qid.path, ip->inode.qid.vers,
				p->fbno);
		return;
	}
	if(ind){
		fprint(2, "double indirect ibno %lud qid %lux.%lux\n",
			p->bno, ip->inode.qid.path, ip->inode.qid.vers);
		return;
	}
	p->bno &= ~Indbno;
	if(s = mark(p->bno)){
		fprint(2, "%s ibno %lud qid %lux.%lux\n",
			s, p->bno, ip->inode.qid.path, ip->inode.qid.vers);
		return;
	}
	bread(&ic, p->bno, buf);
	dp = (Dptr *)buf;
	for(i = 0; i < ic.p2b; i++)
		walk(ip, dp+i, 1);
}

void
pointers(void)
{
	Imap *m, *me;
	Ibuf *ip;

	for(m = ic.map, me = &ic.map[ic.nino]; m < me; m++){
		if(!m->inuse)
			continue;
		ip = iget(&ic, m->qid);
		if(ip == 0)
			continue;
		if(debug)
			fprint(2, "qid  %lux.%lux\n", m->qid.path, m->qid.vers);
		walk(ip, &ip->inode.ptr, 0);
	}
}

/*
 *  look for lost blocks
 */
void
lost(void)
{
	int i;
	int j;
	ulong v;

	for(i = 0; i < ic.nab; i++){
		for(j = 0; j < (ic.b2b+BtoUL-1)/BtoUL; j++){
			v = (ondisk[i].bits[j]^offdisk[i].bits[j]) & ondisk[i].bits[j];
			if(v)
				print("lost blocks %d, %lux\n", j*i, v);
		}
	}
}

/*
 *  count inodes
 */
int
ninodes(void)
{
	Imap *m, *me;
	int ino;

	ino = 0;
	for(m = ic.map, me = &ic.map[ic.nino]; m < me; m++)
		if(m->inuse)
			ino++;
	return ino;
}

/*
 *  count used blocks
 */
ulong
nused(void)
{
	ulong nb;
	ulong v, m;
	int i, j, k;

	nb = 0;
	for(i = 0; i < ic.nab; i++){
		for(j = 0; j < (ic.b2b+BtoUL-1)/BtoUL; j++){
			v = ondisk[i].bits[j];
			m = 1;
			for(k = 0; k < BtoUL-1; k++){
				if(m&v)
					nb++;
				m = m<<1;
			}
		}
	}
	return nb;
}
