/*
 * Extraordinarily brute force Lempel & Ziv-like
 * compressor.  The input file must fit in memory
 * during compression, and the output file will
 * be reconstructed in memory during decompression.
 * We search for large common sequences and use a
 * greedy algorithm to choose which sequence gets
 * compressed first.
 *
 * Files begin with "BLZ\n" and a 32-bit uncompressed file length.
 *
 * Output format is a series of blocks followed by
 * a raw data section.  Each block begins with a 32-bit big-endian
 * number.  The top bit is type and the next 31 bits
 * are uncompressed size.  Type is one of
 *	0 - use raw data for this length
 *	1 - a 32-bit offset follows
 * After the blocks come the raw data.  (The end of the blocks can be
 * noted by summing block lengths until you reach the file length.)
 */

#include <u.h>
#include <libc.h>
#include <bio.h>

#define malloc sbrk

int minrun = 16;
int win = 16;
ulong outn;
int verbose;
int mindist;

enum { Prime = 16777213 };	/* smallest prime < 2^24 (so p*256+256 < 2^32) */
enum { NOFF = 3 };

Biobuf bout;
ulong length;
uchar *data;
ulong sum32(ulong, void*, long);
uchar *odat;
int nodat;
int nraw;
int rawstart;
int acct;
int zlength;
int maxchain;
int maxrle[256];
int nnew;

typedef struct Node Node;
struct Node {
	Node *link;
	ulong key;
	ulong offset[NOFF];
};

Node *nodepool;
int nnodepool;

Node **hash;
uint nhash;

uint maxlen;
uint maxsame;
uint replacesame = 8*1024*1024;

Node *freelist, **freeend;
uint nalloc;

Node*
allocnode(void)
{
	int i;
	Node *n;

	if(nnodepool == 0){
		nnodepool = 256*1024;
		nodepool = malloc(sizeof(Node)*nnodepool);
	}
	if(freelist){
		n = freelist;
		freelist = n->link;
		return n;
	}
	assert(nnodepool > 0);
	nalloc++;
	n = &nodepool[--nnodepool];
	for(i=0; i<NOFF; i++)
		n->offset[i] = -1;

	return n;
}

void
freenode(Node *n)
{
	if(freelist == nil)
		freelist = n;
	else
		*freeend = n;
	freeend = &n->link;
	n->link = nil;
}

Node**
llookup(ulong key)
{
	uint c;
	Node **l, **top, *n;
	
	if(nhash == 0){
		uint x;

		x = length/8;
		for(nhash=1; nhash<x; nhash<<=1)
			;
		hash = sbrk(sizeof(Node*)*nhash);
	}

	top = &hash[key&(nhash-1)];
	c = 0;
	for(l=top; *l; l=&(*l)->link){
		c++;
		if((*l)->key == key){
			/* move to front */
			n = *l;
			*l = n->link;
			n->link = *top;
			*top = n;
			return top;
		}
	}
	if(c > maxlen)
		maxlen = c;
	return l;
}

Node*
lookup(ulong key)
{
	return *llookup(key);
}

void
insertnode(ulong key, ulong offset)
{
	int i;
	Node *n, **l;

	l = llookup(key);
	if(*l == nil){
		if(l==&hash[key&(nhash-1)])
			nnew++;
		*l = allocnode();
		(*l)->key = key;
	}
	n = *l;

	/* add or replace last */
	for(i=0; i<NOFF-1 && n->offset[i]!=-1; i++)
		;
	n->offset[i] = offset;
}

void
Bputint(Biobufhdr *b, int n)
{
	uchar p[4];

	p[0] = n>>24;
	p[1] = n>>16;
	p[2] = n>>8;
	p[3] = n;
	Bwrite(b, p, 4);
}

void
flushraw(void)
{
	if(nraw){
		if(verbose)
			fprint(2, "Raw %d+%d\n", rawstart, nraw);
		zlength += 4+nraw;
		Bputint(&bout, (1<<31)|nraw);
		memmove(odat+nodat, data+rawstart, nraw);
		nodat += nraw;
		nraw = 0;
	}
}

int
rawbyte(int i)
{
	assert(acct == i);
	if(nraw == 0)
		rawstart = i;
	acct++;
	nraw++;
	return 1;
}

int
refblock(int i, int len, int off)
{
	assert(acct == i);
	acct += len;
	if(nraw)
		flushraw();
	if(verbose)
		fprint(2, "Copy %d+%d from %d\n", i, len, off);
	Bputint(&bout, len);
	Bputint(&bout, off);
	zlength += 4+4;
	return len;
}

int
cmprun(uchar *a, uchar *b, int len)
{
	int i;

	if(a==b)
		return 0;
	for(i=0; i<len && a[i]==b[i]; i++)
		;
	return i;
}

int
countrle(uchar *a)
{
	int i;

	for(i=0; a[i]==a[0]; i++)
		;
	return i;
}

void
compress(void)
{
	int best, i, j, o, rle, run, maxrun, maxoff;
	ulong sum;
	Node *n;

	sum = 0;
	for(i=0; i<win && i<length; i++)
		sum = (sum*256+data[i])%Prime;
	for(i=0; i<length-win; ){
		maxrun = 0;
		maxoff = 0;
		if(verbose)
			fprint(2, "look %.6lux\n", sum);
		n = lookup(sum);
		if(n){
			best = -1;
			for(o=0; o<NOFF; o++){
				if(n->offset[o] == -1)
					break;
				run = cmprun(data+i, data+n->offset[o], length-i);
				if(run > maxrun && n->offset[o]+mindist < i){
					maxrun = run;
					maxoff = n->offset[o];
					best = o;
				}
			}
			if(best > 0){
				o = n->offset[best];
				for(j=best; j>0; j--)
					n->offset[j] = n->offset[j-1];
				n->offset[0] = o;
			}
		}
				
		if(maxrun >= minrun)
			j = i+refblock(i, maxrun, maxoff);
		else
			j = i+rawbyte(i);
		for(; i<j; i++){
			/* avoid huge chains from large runs of same byte */
			rle = countrle(data+i);
			if(rle<4)
				insertnode(sum, i);
			else if(rle>maxrle[data[i]]){
				maxrle[data[i]] = rle;
				insertnode(sum, i);
			}
			sum = (sum*256+data[i+win]) % Prime;
			sum = (sum + data[i]*outn) % Prime;
		}
	}
	/* could do better here */
	for(; i<length; i++)
		rawbyte(i);
	flushraw();
}

void
usage(void)
{
	fprint(2, "usage: bflz [-n winsize] [file]\n");
	exits("usage");
}

void
main(int argc, char **argv)
{
	int fd, i, n;
	char buf[10485760];

	ARGBEGIN{
	case 'd':
		verbose = 1;
		break;
	case 's':
		replacesame = atoi(EARGF(usage()));
		break;
	case 'm':
		mindist = atoi(EARGF(usage()));
		break;
	case 'n':
		win = atoi(EARGF(usage()));
		minrun = win;
		break;
	default:
		usage();
	}ARGEND

	switch(argc){
	default:
		usage();
	case 0:
		fd = 0;
		break;
	case 1:
		if((fd = open(argv[0], OREAD)) < 0)
			sysfatal("open %s: %r", argv[0]);
		break;
	}

	while((n = readn(fd, buf, sizeof buf)) > 0){
		data = realloc(data, length+n);
		if(data == nil)
			sysfatal("realloc: %r");
		memmove(data+length, buf, n);
		length += n;
		if(n < sizeof buf)
			break;
	}
	odat = malloc(length);
	if(odat == nil)
		sysfatal("malloc: %r");

	Binit(&bout, 1, OWRITE);
	Bprint(&bout, "BLZ\n");
	Bputint(&bout, length);
	outn = 1;
	for(i=0; i<win; i++)
		outn = (outn * 256) % Prime;

	if(verbose)
		fprint(2, "256^%d = %.6lux\n", win, outn);
	outn = Prime - outn;
	if(verbose)
		fprint(2, "outn = %.6lux\n", outn);

	compress();
	Bwrite(&bout, odat, nodat);
	Bterm(&bout);
	fprint(2, "brk %p\n", sbrk(1));
	fprint(2, "%d nodes used; %d of %d hash slots used\n", nalloc, nnew, nhash);
	exits(nil);	
}
