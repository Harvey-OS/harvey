#include <u.h>
#include <libc.h>
#include <ctype.h>

#include "git.h"

typedef struct Buf	Buf;
typedef struct Metavec	Metavec;
typedef struct Meta	Meta;
typedef struct Compout	Compout;
typedef struct Packf	Packf;

#define max(x, y) ((x) > (y) ? (x) : (y))

struct Metavec {
	Meta	**meta;
	int	nmeta;
	int	metasz;
};

struct Meta {
	Object	*obj;
	char	*path;
	vlong	mtime;

	/* The best delta we picked */
	Meta	*head;
	Meta	*prev;
	Delta	*delta;
	int	ndelta;
	int	nchain;

	/* Only used for delta window */
	Dtab 	dtab;

	/* Only used for writing offset deltas */
	vlong	off;
};

struct Compout {
	Biobuf *bfd;
	DigestState *st;
};

struct Buf {
	int len;
	int sz;
	int off;
	char *data;
};

struct Packf {
	char	path[128];
	char	*idx;
	vlong	nidx;

	int	refs;
	Biobuf	*pack;
	vlong	opentm;
};

static int	readpacked(Biobuf *, Object *, int);
static Object	*readidxobject(Biobuf *, Hash, int);

Objset objcache;
Object *lruhead;
Object *lrutail;
int	ncache;
int	cachemax = 4096;
Packf	*packf;
int	npackf;
int	openpacks;
int	gitdirmode = -1;

static void
clear(Object *o)
{
	if(!o)
		return;

	assert(o->refs == 0);
	assert((o->flag & Ccache) == 0);
	assert(o->flag & Cloaded);
	switch(o->type){
	case GCommit:
		if(!o->commit)
			break;
		free(o->commit->parent);
		free(o->commit->author);
		free(o->commit->committer);
		free(o->commit);
		o->commit = nil;
		break;
	case GTree:
		if(!o->tree)
			break;
		free(o->tree->ent);
		free(o->tree);
		o->tree = nil;
		break;
	default:
		break;
	}

	free(o->all);
	o->all = nil;
	o->data = nil;
	o->flag &= ~(Cloaded|Cparsed);
}

void
unref(Object *o)
{
	if(!o)
		return;
	o->refs--;
	if(o->refs == 0)
		clear(o);
}

Object*
ref(Object *o)
{
	o->refs++;
	return o;
}

void
cache(Object *o)
{
	Object *p;

	if(o == lruhead)
		return;
	if(o == lrutail)
		lrutail = lrutail->prev;
	if(!(o->flag & Cexist)){
		osadd(&objcache, o);
		o->id = objcache.nobj;
		o->flag |= Cexist;
	}
	if(o->prev != nil)
		o->prev->next = o->next;
	if(o->next != nil)
		o->next->prev = o->prev;
	if(lrutail == o){
		lrutail = o->prev;
		if(lrutail != nil)
			lrutail->next = nil;
	}else if(lrutail == nil)
		lrutail = o;
	if(lruhead)
		lruhead->prev = o;
	o->next = lruhead;
	o->prev = nil;
	lruhead = o;

	if(!(o->flag & Ccache)){
		o->flag |= Ccache;
		ref(o);
		ncache++;
	}
	while(ncache > cachemax && lrutail != nil){
		p = lrutail;
		lrutail = p->prev;
		if(lrutail != nil)
			lrutail->next = nil;
		p->flag &= ~Ccache;
		p->prev = nil;
		p->next = nil;
		unref(p);
		ncache--;
	}		
}

static int
loadpack(Packf *pf, char *name)
{
	char buf[128];
	int i, ifd;
	Dir *d;

	memset(pf, 0, sizeof(Packf));
	snprint(buf, sizeof(buf), ".git/objects/pack/%s.idx", name);
	snprint(pf->path, sizeof(pf->path), ".git/objects/pack/%s.pack", name);
	/*
	 * if we already have the pack open, just
	 * steal the loaded info
	 */
	for(i = 0; i < npackf; i++){
		if(strcmp(pf->path, packf[i].path) == 0){
			pf->pack = packf[i].pack;
			pf->idx = packf[i].idx;
			pf->nidx = packf[i].nidx;
			packf[i].idx = nil;
			packf[i].pack = nil;
		}
	}
	if((ifd = open(buf, OREAD)) == -1)
		return -1;
	if((d = dirfstat(ifd)) == nil){
		close(ifd);
		return -1;
	}
	pf->nidx = d->length;
	pf->idx = emalloc(pf->nidx);
	if(readn(ifd, pf->idx, pf->nidx) != pf->nidx){
		close(ifd);
		free(pf->idx);
		free(d);
		return -1;
	}
	close(ifd);
	free(d);
	return 0;
}

static void
refreshpacks(void)
{
	Packf *pf, *new;
	int i, n, l, nnew;
	Dir *d;

	if((n = slurpdir(".git/objects/pack", &d)) == -1)
		return;
	nnew = 0;
	new = eamalloc(n, sizeof(Packf));
	for(i = 0; i < n; i++){
		l = strlen(d[i].name);
		if(l > 4 && strcmp(d[i].name + l - 4, ".idx") != 0)
			continue;
		d[i].name[l - 4] = 0;
		if(loadpack(&new[nnew], d[i].name) != -1)
			nnew++;
	}
	for(i = 0; i < npackf; i++){
		pf = &packf[i];
		free(pf->idx);
		if(pf->pack != nil)
			Bterm(pf->pack);
	}
	free(packf);
	packf = new;
	npackf = nnew;
	free(d);
}

static Biobuf*
openpack(Packf *pf)
{
	vlong t;
	int i, best;

	if(pf->pack == nil){
		if((pf->pack = Bopen(pf->path, OREAD)) == nil)
			return nil;
		openpacks++;
	}
	if(openpacks == Npackcache){
		t = pf->opentm;
		best = -1;
		for(i = 0; i < npackf; i++){
			if(packf[i].opentm < t && packf[i].refs > 0){
				t = packf[i].opentm;
				best = i;
			}
		}
		if(best != -1){
			Bterm(packf[best].pack);
			packf[best].pack = nil;
			openpacks--;
		}
	}
	pf->opentm = nsec();
	pf->refs++;
	return pf->pack;
}

static void
closepack(Packf *pf)
{
	if(--pf->refs == 0){
		Bterm(pf->pack);
		pf->pack = nil;
	}
}

static u32int
crc32(u32int crc, char *b, int nb)
{
	static u32int crctab[256] = {
		0x00000000, 0x77073096, 0xee0e612c, 0x990951ba, 0x076dc419, 0x706af48f, 0xe963a535, 
		0x9e6495a3, 0x0edb8832, 0x79dcb8a4, 0xe0d5e91e, 0x97d2d988, 0x09b64c2b, 0x7eb17cbd, 
		0xe7b82d07, 0x90bf1d91, 0x1db71064, 0x6ab020f2, 0xf3b97148, 0x84be41de, 0x1adad47d, 
		0x6ddde4eb, 0xf4d4b551, 0x83d385c7, 0x136c9856, 0x646ba8c0, 0xfd62f97a, 0x8a65c9ec, 
		0x14015c4f, 0x63066cd9, 0xfa0f3d63, 0x8d080df5, 0x3b6e20c8, 0x4c69105e, 0xd56041e4, 
		0xa2677172, 0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b, 0x35b5a8fa, 0x42b2986c, 
		0xdbbbc9d6, 0xacbcf940, 0x32d86ce3, 0x45df5c75, 0xdcd60dcf, 0xabd13d59, 0x26d930ac, 
		0x51de003a, 0xc8d75180, 0xbfd06116, 0x21b4f4b5, 0x56b3c423, 0xcfba9599, 0xb8bda50f, 
		0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924, 0x2f6f7c87, 0x58684c11, 0xc1611dab, 
		0xb6662d3d, 0x76dc4190, 0x01db7106, 0x98d220bc, 0xefd5102a, 0x71b18589, 0x06b6b51f, 
		0x9fbfe4a5, 0xe8b8d433, 0x7807c9a2, 0x0f00f934, 0x9609a88e, 0xe10e9818, 0x7f6a0dbb, 
		0x086d3d2d, 0x91646c97, 0xe6635c01, 0x6b6b51f4, 0x1c6c6162, 0x856530d8, 0xf262004e, 
		0x6c0695ed, 0x1b01a57b, 0x8208f4c1, 0xf50fc457, 0x65b0d9c6, 0x12b7e950, 0x8bbeb8ea, 
		0xfcb9887c, 0x62dd1ddf, 0x15da2d49, 0x8cd37cf3, 0xfbd44c65, 0x4db26158, 0x3ab551ce, 
		0xa3bc0074, 0xd4bb30e2, 0x4adfa541, 0x3dd895d7, 0xa4d1c46d, 0xd3d6f4fb, 0x4369e96a, 
		0x346ed9fc, 0xad678846, 0xda60b8d0, 0x44042d73, 0x33031de5, 0xaa0a4c5f, 0xdd0d7cc9, 
		0x5005713c, 0x270241aa, 0xbe0b1010, 0xc90c2086, 0x5768b525, 0x206f85b3, 0xb966d409, 
		0xce61e49f, 0x5edef90e, 0x29d9c998, 0xb0d09822, 0xc7d7a8b4, 0x59b33d17, 0x2eb40d81, 
		0xb7bd5c3b, 0xc0ba6cad, 0xedb88320, 0x9abfb3b6, 0x03b6e20c, 0x74b1d29a, 0xead54739, 
		0x9dd277af, 0x04db2615, 0x73dc1683, 0xe3630b12, 0x94643b84, 0x0d6d6a3e, 0x7a6a5aa8, 
		0xe40ecf0b, 0x9309ff9d, 0x0a00ae27, 0x7d079eb1, 0xf00f9344, 0x8708a3d2, 0x1e01f268, 
		0x6906c2fe, 0xf762575d, 0x806567cb, 0x196c3671, 0x6e6b06e7, 0xfed41b76, 0x89d32be0, 
		0x10da7a5a, 0x67dd4acc, 0xf9b9df6f, 0x8ebeeff9, 0x17b7be43, 0x60b08ed5, 0xd6d6a3e8, 
		0xa1d1937e, 0x38d8c2c4, 0x4fdff252, 0xd1bb67f1, 0xa6bc5767, 0x3fb506dd, 0x48b2364b, 
		0xd80d2bda, 0xaf0a1b4c, 0x36034af6, 0x41047a60, 0xdf60efc3, 0xa867df55, 0x316e8eef, 
		0x4669be79, 0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236, 0xcc0c7795, 0xbb0b4703, 
		0x220216b9, 0x5505262f, 0xc5ba3bbe, 0xb2bd0b28, 0x2bb45a92, 0x5cb36a04, 0xc2d7ffa7, 
		0xb5d0cf31, 0x2cd99e8b, 0x5bdeae1d, 0x9b64c2b0, 0xec63f226, 0x756aa39c, 0x026d930a, 
		0x9c0906a9, 0xeb0e363f, 0x72076785, 0x05005713, 0x95bf4a82, 0xe2b87a14, 0x7bb12bae, 
		0x0cb61b38, 0x92d28e9b, 0xe5d5be0d, 0x7cdcefb7, 0x0bdbdf21, 0x86d3d2d4, 0xf1d4e242, 
		0x68ddb3f8, 0x1fda836e, 0x81be16cd, 0xf6b9265b, 0x6fb077e1, 0x18b74777, 0x88085ae6, 
		0xff0f6a70, 0x66063bca, 0x11010b5c, 0x8f659eff, 0xf862ae69, 0x616bffd3, 0x166ccf45, 
		0xa00ae278, 0xd70dd2ee, 0x4e048354, 0x3903b3c2, 0xa7672661, 0xd06016f7, 0x4969474d, 
		0x3e6e77db, 0xaed16a4a, 0xd9d65adc, 0x40df0b66, 0x37d83bf0, 0xa9bcae53, 0xdebb9ec5, 
		0x47b2cf7f, 0x30b5ffe9, 0xbdbdf21c, 0xcabac28a, 0x53b39330, 0x24b4a3a6, 0xbad03605, 
		0xcdd70693, 0x54de5729, 0x23d967bf, 0xb3667a2e, 0xc4614ab8, 0x5d681b02, 0x2a6f2b94, 
		0xb40bbe37, 0xc30c8ea1, 0x5a05df1b, 0x2d02ef8d
	};
	int i;

	crc ^=  0xFFFFFFFF;
	for(i = 0; i < nb; i++)
		crc = (crc >> 8) ^ crctab[(crc ^ b[i]) & 0xFF];
	return crc ^ 0xFFFFFFFF;
}

int
bappend(void *p, void *src, int len)
{
	Buf *b = p;
	char *n;

	while(b->len + len >= b->sz){
		b->sz = b->sz*2 + 64;
		n = realloc(b->data, b->sz);
		if(n == nil)
			return -1;
		b->data = n;
	}
	memmove(b->data + b->len, src, len);
	b->len += len;
	return len;
}

int
breadc(void *p)
{
	return Bgetc(p);
}

int
bdecompress(Buf *d, Biobuf *b, vlong *csz)
{
	vlong o;

	o = Boffset(b);
	if(inflatezlib(d, bappend, b, breadc) == -1 || d->data == nil){
		free(d->data);
		return -1;
	}
	if (csz)
		*csz = Boffset(b) - o;
	return d->len;
}

int
decompress(void **p, Biobuf *b, vlong *csz)
{
	Buf d = {.len=0, .data=nil, .sz=0};

	if(bdecompress(&d, b, csz) == -1){
		free(d.data);
		return -1;
	}
	*p = d.data;
	return d.len;
}

static vlong
readvint(char *p, char **pp)
{
	int s, c;
	vlong n;
	
	s = 0;
	n = 0;
	do {
		c = *p++;
		n |= (c & 0x7f) << s;
		s += 7;
	} while (c & 0x80 && s < 63);
	*pp = p;

	return n;
}

static int
applydelta(Object *dst, Object *base, char *d, int nd)
{
	char *r, *b, *ed, *er;
	int n, nr, c;
	vlong o, l;

	ed = d + nd;
	b = base->data;
	n = readvint(d, &d);
	if(n != base->size){
		werrstr("mismatched source size");
		return -1;
	}

	nr = readvint(d, &d);
	r = emalloc(nr + 64);
	n = snprint(r, 64, "%T %d", base->type, nr) + 1;
	dst->all = r;
	dst->type = base->type;
	dst->data = r + n;
	dst->size = nr;
	er = dst->data + nr;
	r = dst->data;

	while(d != ed){
		c = *d++;
		/* copy from base */
		if(c & 0x80){
			o = 0;
			l = 0;
			/* Offset in base */
			if(d != ed && (c & 0x01)) o |= (*d++ <<  0) & 0x000000ff;
			if(d != ed && (c & 0x02)) o |= (*d++ <<  8) & 0x0000ff00;
			if(d != ed && (c & 0x04)) o |= (*d++ << 16) & 0x00ff0000;
			if(d != ed && (c & 0x08)) o |= (*d++ << 24) & 0xff000000;

			/* Length to copy */
			if(d != ed && (c & 0x10)) l |= (*d++ <<  0) & 0x0000ff;
			if(d != ed && (c & 0x20)) l |= (*d++ <<  8) & 0x00ff00;
			if(d != ed && (c & 0x40)) l |= (*d++ << 16) & 0xff0000;
			if(l == 0) l = 0x10000;

			if(o + l > base->size){
				werrstr("garbled delta: out of bounds copy");
				return -1;
			}
			memmove(r, b + o, l);
			r += l;
		/* inline data */
		}else{
			if(c > ed - d){
				werrstr("garbled delta: write past object");
				return -1;
			}
			memmove(r, d, c);
			d += c;
			r += c;
		}
	}
	if(r != er){
		werrstr("truncated delta");
		return -1;
	}

	return nr;
}

static int
readrdelta(Biobuf *f, Object *o, int nd, int flag)
{
	Object *b;
	Hash h;
	char *d;
	int n;

	d = nil;
	if(Bread(f, h.h, sizeof(h.h)) != sizeof(h.h))
		goto error;
	if(hasheq(&o->hash, &h))
		goto error;
	if((n = decompress(&d, f, nil)) == -1)
		goto error;
	o->len = Boffset(f) - o->off;
	if(d == nil || n != nd)
		goto error;
	if((b = readidxobject(f, h, flag|Cthin)) == nil)
		goto error;
	if(applydelta(o, b, d, n) == -1)
		goto error;
	free(d);
	return 0;
error:
	free(d);
	return -1;
}

static int
readodelta(Biobuf *f, Object *o, vlong nd, vlong p, int flag)
{
	Object b;
	char *d;
	vlong r;
	int c, n;

	d = nil;
	if((c = Bgetc(f)) == -1)
		return -1;
	r = c & 0x7f;
	while(c & 0x80 && r < (1ULL<<56)){
		if((c = Bgetc(f)) == -1)
			return -1;
		r = ((r + 1)<<7) | (c & 0x7f);
	}

	if(r > p || r >= (1ULL<<56)){
		werrstr("junk offset -%lld (from %lld)", r, p);
		goto error;
	}
	if((n = decompress(&d, f, nil)) == -1)
		goto error;
	o->len = Boffset(f) - o->off;
	if(d == nil || n != nd)
		goto error;
	if(Bseek(f, p - r, 0) == -1)
		goto error;
	memset(&b, 0, sizeof(Object));
	if(readpacked(f, &b, flag) == -1)
		goto error;
	if(applydelta(o, &b, d, nd) == -1)
		goto error;
	clear(&b);
	free(d);
	return 0;
error:
	free(d);
	return -1;
}

static int
readpacked(Biobuf *f, Object *o, int flag)
{
	int c, s, n;
	vlong l, p;
	int t;
	Buf b;

	p = Boffset(f);
	c = Bgetc(f);
	if(c == -1)
		return -1;
	l = c & 0xf;
	s = 4;
	t = (c >> 4) & 0x7;
	if(!t){
		werrstr("unknown type for byte %x at %lld", c, p);
		return -1;
	}
	while(c & 0x80){
		if((c = Bgetc(f)) == -1)
			return -1;
		l |= (c & 0x7f) << s;
		s += 7;
	}
	if(l >= (1ULL << 32)){
		werrstr("object too big");
		return -1;
	}
	switch(t){
	default:
		werrstr("invalid object at %lld", Boffset(f));
		return -1;
	case GCommit:
	case GTree:
	case GTag:
	case GBlob:
		b.sz = 64 + l;
		b.data = emalloc(b.sz);
		n = snprint(b.data, 64, "%T %lld", t, l) + 1;
		b.len = n;
		if(bdecompress(&b, f, nil) == -1){
			free(b.data);
			return -1;
		}
		o->len = Boffset(f) - o->off;
		o->type = t;
		o->all = b.data;
		o->data = b.data + n;
		o->size = b.len - n;
		break;
	case GOdelta:
		if(readodelta(f, o, l, p, flag) == -1)
			return -1;
		break;
	case GRdelta:
		if(readrdelta(f, o, l, flag) == -1)
			return -1;
		break;
	}
	o->flag |= Cloaded|flag;
	return 0;
}

static int
readloose(Biobuf *f, Object *o, int flag)
{
	struct { char *tag; int type; } *p, types[] = {
		{"blob", GBlob},
		{"tree", GTree},
		{"commit", GCommit},
		{"tag", GTag},
		{nil},
	};
	char *d, *s, *e;
	vlong sz, n;
	int l;

	n = decompress(&d, f, nil);
	if(n == -1)
		return -1;

	s = d;
	o->type = GNone;
	for(p = types; p->tag; p++){
		l = strlen(p->tag);
		if(strncmp(s, p->tag, l) == 0){
			s += l;
			o->type = p->type;
			while(!isspace(*s))
				s++;
			break;
		}
	}
	if(o->type == GNone){
		free(o->data);
		return -1;
	}
	sz = strtol(s, &e, 0);
	if(e == s || *e++ != 0){
		werrstr("malformed object header");
		goto error;
	}
	if(sz != n - (e - d)){
		werrstr("mismatched sizes");
		goto error;
	}
	o->size = sz;
	o->data = e;
	o->all = d;
	o->flag |= Cloaded|flag;
	return 0;

error:
	free(d);
	return -1;
}

vlong
searchindex(char *idx, int nidx, Hash h)
{
	int lo, hi, hidx, i, r, nent;
	vlong o, oo;
	void *s;

	o = 8;
	if(nidx < 8 + 256*4)
		return -1;
	/*
	 * Read the fanout table. The fanout table
	 * contains 256 entries, corresponsding to
	 * the first byte of the hash. Each entry
	 * is a 4 byte big endian integer, containing
	 * the total number of entries with a leading
	 * byte <= the table index, allowing us to
	 * rapidly do a binary search on them.
	 */
	if (h.h[0] == 0){
		lo = 0;
		hi = GETBE32(idx + o);
	} else {
		o += h.h[0]*4 - 4;
		lo = GETBE32(idx + o);
		hi = GETBE32(idx + o + 4);
	}
	if(hi == lo)
		goto notfound;
	nent=GETBE32(idx + 8 + 255*4);

	/*
	 * Now that we know the range of hashes that the
	 * entry may exist in, search them
	 */
	r = -1;
	hidx = -1;
	o = 8 + 256*4;
	while(lo < hi){
		hidx = (hi + lo)/2;
		s = idx + o + hidx*sizeof(h.h);
		r = memcmp(h.h, s, sizeof(h.h));
		if(r < 0)
			hi = hidx;
		else if(r > 0)
			lo = hidx + 1;
		else
			break;
	}
	if(r != 0)
		goto notfound;

	/*
	 * We found the entry. If it's 32 bits, then we
	 * can just return the oset, otherwise the 32
	 * bit entry contains the oset to the 64 bit
	 * entry.
	 */
	oo = 8;			/* Header */
	oo += 256*4;		/* Fanout table */
	oo += Hashsz*nent;	/* Hashes */
	oo += 4*nent;		/* Checksums */
	oo += 4*hidx;		/* Offset offset */
	if(oo < 0 || oo + 4 > nidx)
		goto err;
	i = GETBE32(idx + oo);
	o = i & 0xffffffffULL;
	/*
	 * Large offsets (i.e. 64-bit) are encoded as an index
	 * into the next table with the MSB bit set.
	 */
	if(o & (1ull << 31)){
		o &= 0x7fffffffULL;
		oo = 8;				/* Header */
		oo += 256*4;			/* Fanout table */
		oo += Hashsz*nent;		/* Hashes */
		oo += 4*nent;			/* Checksums */
		oo += 4*nent;			/* 32-bit Offsets */
		oo += 8*o;			/* 64-bit Offset offset */
		if(oo < 0 || oo + 8 >= nidx)
			goto err;
		o = GETBE64(idx + oo);
	}
	return o;

err:
	werrstr("out of bounds read");
	return -1;
notfound:
	werrstr("not present");
	return -1;		
}

/*
 * Scans for non-empty word, copying it into buf.
 * Strips off word, leading, and trailing space
 * from input.
 * 
 * Returns -1 on empty string or error, leaving
 * input unmodified.
 */
static int
scanword(char **str, int *nstr, char *buf, int nbuf)
{
	char *p;
	int n, r;

	r = -1;
	p = *str;
	n = *nstr;
	while(n && isblank(*p)){
		n--;
		p++;
	}

	for(; n && *p && !isspace(*p); p++, n--){
		r = 0;
		*buf++ = *p;
		nbuf--;
		if(nbuf == 0)
			return -1;
	}
	while(n && isblank(*p)){
		n--;
		p++;
	}
	*buf = 0;
	*str = p;
	*nstr = n;
	return r;
}

static void
nextline(char **str, int *nstr)
{
	char *s;

	if((s = strchr(*str, '\n')) != nil){
		*nstr -= s - *str + 1;
		*str = s + 1;
	}
}

static int
parseauthor(char **str, int *nstr, char **name, vlong *time)
{
	char buf[128];
	Resub m[4];
	char *p;
	int n, nm;

	if((p = strchr(*str, '\n')) == nil)
		sysfatal("malformed author line");
	n = p - *str;
	if(n >= sizeof(buf))
		sysfatal("overlong author line");
	memset(m, 0, sizeof(m));
	snprint(buf, n + 1, *str);
	*str = p;
	*nstr -= n;
	
	if(!regexec(authorpat, buf, m, nelem(m)))
		sysfatal("invalid author line %s", buf);
	nm = m[1].ep - m[1].sp;
	*name = emalloc(nm + 1);
	memcpy(*name, m[1].sp, nm);
	buf[nm] = 0;
	
	nm = m[2].ep - m[2].sp;
	memcpy(buf, m[2].sp, nm);
	buf[nm] = 0;
	*time = atoll(buf);
	return 0;
}

static void
parsecommit(Object *o)
{
	char *p, *t, buf[128];
	int np;

	p = o->data;
	np = o->size;
	o->commit = emalloc(sizeof(Cinfo));
	while(1){
		if(scanword(&p, &np, buf, sizeof(buf)) == -1)
			break;
		if(strcmp(buf, "tree") == 0){
			if(scanword(&p, &np, buf, sizeof(buf)) == -1)
				sysfatal("invalid commit: tree missing");
			if(hparse(&o->commit->tree, buf) == -1)
				sysfatal("invalid commit: garbled tree");
		}else if(strcmp(buf, "parent") == 0){
			if(scanword(&p, &np, buf, sizeof(buf)) == -1)
				sysfatal("invalid commit: missing parent");
			o->commit->parent = realloc(o->commit->parent, ++o->commit->nparent * sizeof(Hash));
			if(!o->commit->parent)
				sysfatal("unable to malloc: %r");
			if(hparse(&o->commit->parent[o->commit->nparent - 1], buf) == -1)
				sysfatal("invalid commit: garbled parent");
		}else if(strcmp(buf, "author") == 0){
			parseauthor(&p, &np, &o->commit->author, &o->commit->mtime);
		}else if(strcmp(buf, "committer") == 0){
			parseauthor(&p, &np, &o->commit->committer, &o->commit->ctime);
		}else if(strcmp(buf, "gpgsig") == 0){
			/* just drop it */
			if((t = strstr(p, "-----END PGP SIGNATURE-----")) == nil)
				sysfatal("malformed gpg signature");
			np -= t - p;
			p = t;
		}
		nextline(&p, &np);
	}
	while (np && isspace(*p)) {
		p++;
		np--;
	}
	o->commit->msg = p;
	o->commit->nmsg = np;
}

static void
parsetree(Object *o)
{
	int m, a, entsz, nent;
	Dirent *t, *ent;
	char *p, *ep;

	p = o->data;
	ep = p + o->size;

	nent = 0;
	entsz = 16;
	ent = eamalloc(entsz, sizeof(Dirent));	
	o->tree = emalloc(sizeof(Tinfo));
	while(p != ep){
		if(nent == entsz){
			entsz *= 2;
			ent = earealloc(ent, entsz, sizeof(Dirent));	
		}
		t = &ent[nent++];
		m = strtol(p, &p, 8);
		if(*p != ' ')
			sysfatal("malformed tree %H: *p=(%d) %c\n", o->hash, *p, *p);
		p++;
		/*
		 * only the stored permissions for the user
		 * are relevant; git fills group and world
		 * bits with whatever -- so to serve with
		 * useful permissions, replicate the mode
		 * of the git repo dir.
		 */
		a = (m & 0777)>>6;
		t->mode = ((a<<6)|(a<<3)|a) & gitdirmode;
		t->ismod = 0;
		t->islink = 0;
		if(m == 0160000){
			t->mode |= DMDIR;
			t->ismod = 1;
		}else if(m == 0120000){
			t->mode = 0;
			t->islink = 1;
		}
		if(m & 0040000)
			t->mode |= DMDIR;
		t->name = p;
		p = memchr(p, 0, ep - p);
		if(*p++ != 0 ||  ep - p < sizeof(t->h.h))
			sysfatal("malformed tree %H, remaining %d (%s)", o->hash, (int)(ep - p), p);
		memcpy(t->h.h, p, sizeof(t->h.h));
		p += sizeof(t->h.h);
	}
	o->tree->ent = ent;
	o->tree->nent = nent;
}

static void
parsetag(Object *)
{
}

void
parseobject(Object *o)
{
	if(o->flag & Cparsed)
		return;
	switch(o->type){
	case GTree:	parsetree(o);	break;
	case GCommit:	parsecommit(o);	break;
	case GTag:	parsetag(o);	break;
	default:	break;
	}
	o->flag |= Cparsed;
}

static Object*
readidxobject(Biobuf *idx, Hash h, int flag)
{
	char path[Pathmax], hbuf[41];
	Object *obj, *new;
	int i, r, retried;
	Biobuf *f;
	vlong o;

	if((obj = osfind(&objcache, h)) != nil){
		if(flag & Cidx){
			/*
			 * If we're indexing, we need to be careful
			 * to only return objects within this pack,
			 * so if the objects exist outside the pack,
			 * we don't index the wrong copy.
			 */
			if(!(obj->flag & Cidx))
				return nil;
			if(obj->flag & Cloaded)
				return obj;
			o = Boffset(idx);
			if(Bseek(idx, obj->off, 0) == -1)
				return nil;
			if(readpacked(idx, obj, flag) == -1)
				return nil;
			if(Bseek(idx, o, 0) == -1)
				sysfatal("could not restore offset");
			cache(obj);
			return obj;
		}
		if(obj->flag & Cloaded)
			return obj;
	}
	if(flag & Cthin)
		flag &= ~Cidx;
	if(flag & Cidx)
		return nil;
	new = nil;
	if(obj == nil){
		new = emalloc(sizeof(Object));
		new->id = objcache.nobj + 1;
		new->hash = h;
		obj = new;
	}

	o = -1;
	retried = 0;
retry:
	for(i = 0; i < npackf; i++){
		o = searchindex(packf[i].idx, packf[i].nidx, h);
		if(o != -1){
			if((f = openpack(&packf[i])) == nil)
				goto error;
			if((r = Bseek(f, o, 0)) != -1)
				r = readpacked(f, obj, flag);
			closepack(&packf[i]);
			if(r == -1)
				goto error;
			parseobject(obj);
			cache(obj);
			return obj;
		}
	}
			

	snprint(hbuf, sizeof(hbuf), "%H", h);
	snprint(path, sizeof(path), ".git/objects/%c%c/%s", hbuf[0], hbuf[1], hbuf + 2);
	if((f = Bopen(path, OREAD)) != nil){
		if(readloose(f, obj, flag) == -1)
			goto errorf;
		Bterm(f);
		parseobject(obj);
		cache(obj);
		return obj;
	}

	if(o == -1){
		if(retried)
			goto error;
		retried = 1;
		refreshpacks();
		goto retry;
	}
errorf:
	Bterm(f);
error:
	free(new);
	return nil;
}

/*
 * Loads and returns a cached object.
 */
Object*
readobject(Hash h)
{
	Object *o;
	Dir *d;

	if(gitdirmode == -1){
		if((d = dirstat(".git")) == nil)
			sysfatal("stat .git: %r");
		gitdirmode = d->mode & 0777;
		free(d);
	}
	if((o = readidxobject(nil, h, 0)) == nil)
		return nil;
	parseobject(o);
	ref(o);
	return o;
}

/*
 * Creates and returns a cached, cleared object
 * that will get loaded some other time. Useful
 * for performance if need to mark that a blob
 * exists, but we don't care about its contents.
 *
 * The refcount of the returned object is 0, so
 * it doesn't need to be unrefed.
 */
Object*
clearedobject(Hash h, int type)
{
	Object *o;

	if((o = osfind(&objcache, h)) != nil)
		return o;

	o = emalloc(sizeof(Object));
	o->hash = h;
	o->type = type;
	osadd(&objcache, o);
	o->id = objcache.nobj;
	o->flag |= Cexist;
	return o;
}

int
objcmp(void *pa, void *pb)
{
	Object *a, *b;

	a = *(Object**)pa;
	b = *(Object**)pb;
	return memcmp(a->hash.h, b->hash.h, sizeof(a->hash.h));
}

static int
hwrite(Biobuf *b, void *buf, int len, DigestState **st)
{
	*st = sha1(buf, len, nil, *st);
	return Bwrite(b, buf, len);
}

static u32int
objectcrc(Biobuf *f, Object *o)
{
	char buf[8096];
	int n, r;

	o->crc = 0;
	Bseek(f, o->off, 0);
	for(n = o->len; n > 0; n -= r){
		r = Bread(f, buf, n > sizeof(buf) ? sizeof(buf) : n);
		if(r == -1)
			return -1;
		if(r == 0)
			return 0;
		o->crc = crc32(o->crc, buf, r);
	}
	return 0;
}

int
indexpack(char *pack, char *idx, Hash ph)
{
	char hdr[4*3], buf[8];
	int nobj, npct, nvalid, nbig;
	int i, n, pct;
	Object *o, **obj;
	DigestState *st;
	char *valid;
	Biobuf *f;
	Hash h;
	int c;

	if((f = Bopen(pack, OREAD)) == nil)
		return -1;
	if(Bread(f, hdr, sizeof(hdr)) != sizeof(hdr)){
		werrstr("short read on header");
		return -1;
	}
	if(memcmp(hdr, "PACK\0\0\0\2", 8) != 0){
		werrstr("invalid header");
		return -1;
	}

	pct = 0;
	npct = 0;
	nvalid = 0;
	nobj = GETBE32(hdr + 8);
	obj = eamalloc(nobj, sizeof(Object*));
	valid = eamalloc(nobj, sizeof(char));
	if(interactive)
		fprint(2, "indexing %d objects:   0%%", nobj);
	while(nvalid != nobj){
		n = 0;
		for(i = 0; i < nobj; i++){
			if(valid[i]){
				n++;
				continue;
			}
			pct = showprogress((npct*100)/nobj, pct);
			if(obj[i] == nil){
				o = emalloc(sizeof(Object));
				o->off = Boffset(f);
				obj[i] = o;
			}
			o = obj[i];
			/*
			 * We can seek around when packing delta chains.
			 * Be extra careful while we don't know where all
			 * the objects start.
			 */
			Bseek(f, o->off, 0);
			if(readpacked(f, o, Cidx) == -1)
				continue;
			sha1((uchar*)o->all, o->size + strlen(o->all) + 1, o->hash.h, nil);
			valid[i] = 1;
			cache(o);
			npct++;
			n++;
			if(objectcrc(f, o) == -1)
				return -1;
		}
		if(n == nvalid){
			sysfatal("fix point reached too early: %d/%d: %r", nvalid, nobj);
			goto error;
		}
		nvalid = n;
	}
	if(interactive)
		fprint(2, "\b\b\b\b100%%\n");
	Bterm(f);

	st = nil;
	qsort(obj, nobj, sizeof(Object*), objcmp);
	if((f = Bopen(idx, OWRITE)) == nil)
		return -1;
	if(hwrite(f, "\xfftOc\x00\x00\x00\x02", 8, &st) != 8)
		goto error;
	/* fanout table */
	c = 0;
	for(i = 0; i < 256; i++){
		while(c < nobj && (obj[c]->hash.h[0] & 0xff) <= i)
			c++;
		PUTBE32(buf, c);
		if(hwrite(f, buf, 4, &st) == -1)
			goto error;
	}
	for(i = 0; i < nobj; i++){
		o = obj[i];
		if(hwrite(f, o->hash.h, sizeof(o->hash.h), &st) == -1)
			goto error;
	}

	for(i = 0; i < nobj; i++){
		PUTBE32(buf, obj[i]->crc);
		if(hwrite(f, buf, 4, &st) == -1)
			goto error;
	}

	nbig = 0;
	for(i = 0; i < nobj; i++){
		if(obj[i]->off < (1ull<<31))
			PUTBE32(buf, obj[i]->off);
		else{
			PUTBE32(buf, (1ull << 31) | nbig);
			nbig++;
		}
		if(hwrite(f, buf, 4, &st) == -1)
			goto error;
	}
	for(i = 0; i < nobj; i++){
		if(obj[i]->off >= (1ull<<31)){
			PUTBE64(buf, obj[i]->off);
			if(hwrite(f, buf, 8, &st) == -1)
				goto error;
		}
	}
	if(hwrite(f, ph.h, sizeof(ph.h), &st) == -1)
		goto error;
	sha1(nil, 0, h.h, st);
	Bwrite(f, h.h, sizeof(h.h));

	free(obj);
	free(valid);
	Bterm(f);
	return 0;

error:
	free(obj);
	free(valid);
	Bterm(f);
	return -1;
}

static int
deltaordercmp(void *pa, void *pb)
{
	Meta *a, *b;
	int cmp;

	a = *(Meta**)pa;
	b = *(Meta**)pb;
	if(a->obj->type != b->obj->type)
		return a->obj->type - b->obj->type;
	cmp = strcmp(a->path, b->path);
	if(cmp != 0)
		return cmp;
	if(a->mtime != b->mtime)
		return a->mtime - b->mtime;
	return memcmp(a->obj->hash.h, b->obj->hash.h, sizeof(a->obj->hash.h));
}

static int
writeordercmp(void *pa, void *pb)
{
	Meta *a, *b, *ahd, *bhd;

	a = *(Meta**)pa;
	b = *(Meta**)pb;
	ahd = (a->head == nil) ? a : a->head;
	bhd = (b->head == nil) ? b : b->head;
	if(ahd->mtime != bhd->mtime)
		return bhd->mtime - ahd->mtime;
	if(ahd != bhd)
		return (uintptr)bhd - (uintptr)ahd;
	if(a->nchain != b->nchain)
		return a->nchain - b->nchain;
	return a->mtime - b->mtime;
}

static void
addmeta(Metavec *v, Objset *has, Object *o, char *path, vlong mtime)
{
	Meta *m;

	if(oshas(has, o->hash))
		return;
	osadd(has, o);
	if(v == nil)
		return;
	m = emalloc(sizeof(Meta));
	m->obj = o;
	m->path = estrdup(path);
	m->mtime = mtime;

	if(v->nmeta == v->metasz){
		v->metasz = 2*v->metasz;
		v->meta = earealloc(v->meta, v->metasz, sizeof(Meta*));
	}
	v->meta[v->nmeta++] = m;
}

static void
freemeta(Meta *m)
{
	free(m->delta);
	free(m->path);
	free(m);
}

static int
loadtree(Metavec *v, Objset *has, Hash tree, char *dpath, vlong mtime)
{
	Object *t, *o;
	Dirent *e;
	char *p;
	int i, k;

	if(oshas(has, tree))
		return 0;
	if((t = readobject(tree)) == nil)
		return -1;
	if(t->type != GTree){
		fprint(2, "load: %H: not tree\n", t->hash);
		unref(t);
		return 0;
	}
	addmeta(v, has, t, dpath, mtime);
	for(i = 0; i < t->tree->nent; i++){
		e = &t->tree->ent[i];
		if(oshas(has, e->h))
			continue;
		if(e->ismod)
			continue;
		k = (e->mode & DMDIR) ? GTree : GBlob;
		o = clearedobject(e->h, k);
		p = smprint("%s/%s", dpath, e->name);
		if(k == GBlob)
			addmeta(v, has, o, p, mtime);
		else if(loadtree(v, has, e->h, p, mtime) == -1){
			free(p);
			return -1;
		}
		free(p);
	}
	unref(t);
	return 0;
}

static int
loadcommit(Metavec *v, Objset *has, Hash h)
{
	Object *c;
	int r;

	if(osfind(has, h))
		return 0;
	if((c = readobject(h)) == nil)
		return -1;
	if(c->type != GCommit){
		fprint(2, "load: %H: not commit\n", c->hash);
		unref(c);
		return 0;
	}
	addmeta(v, has, c, "", c->commit->ctime);
	r = loadtree(v, has, c->commit->tree, "", c->commit->ctime);
	unref(c);
	return r;
}

static int
readmeta(Hash *theirs, int ntheirs, Hash *ours, int nours, Meta ***m)
{
	Object **obj;
	Objset has;
	int i, nobj;
	Metavec v;

	*m = nil;
	osinit(&has);
	v.nmeta = 0;
	v.metasz = 64;
	v.meta = eamalloc(v.metasz, sizeof(Meta*));
	if(findtwixt(theirs, ntheirs, ours, nours, &obj, &nobj) == -1)
		sysfatal("load twixt: %r");

	if(nobj == 0)
		return 0;
	for(i = 0; i < nours; i++)
		if(!hasheq(&ours[i], &Zhash))
			if(loadcommit(nil, &has, ours[i]) == -1)
				goto out;
	for(i = 0; i < nobj; i++)
		if(loadcommit(&v, &has, obj[i]->hash) == -1)
			goto out;
	osclear(&has);
	*m = v.meta;
	return v.nmeta;
out:
	osclear(&has);
	free(v.meta);
	return -1;
}

static int
deltasz(Delta *d, int nd)
{
	int i, sz;
	sz = 32;
	for(i = 0; i < nd; i++)
		sz += d[i].cpy ? 7 : d[i].len + 1;
	return sz;
}

static void
pickdeltas(Meta **meta, int nmeta)
{
	Meta *m, *p;
	Object *o;
	Delta *d;
	int i, j, nd, sz, pct, best;

	pct = 0;
	dprint(1, "picking deltas\n");
	fprint(2, "deltifying %d objects:   0%%", nmeta);
	qsort(meta, nmeta, sizeof(Meta*), deltaordercmp);
	for(i = 0; i < nmeta; i++){
		m = meta[i];
		pct = showprogress((i*100) / nmeta, pct);
		m->delta = nil;
		m->ndelta = 0;
		if(m->obj->type == GCommit || m->obj->type == GTag)
			continue;
		if((o = readobject(m->obj->hash)) == nil)
			sysfatal("readobject %H: %r", m->obj->hash);
		dtinit(&m->dtab, o);
		if(i >= 11)
			dtclear(&meta[i-11]->dtab);
		best = o->size;
		for(j = max(0, i - 10); j < i; j++){
			p = meta[j];
			/* long chains make unpacking slow */
			if(p->nchain >= 128 || p->obj->type != o->type)
				continue;
			d = deltify(o, &p->dtab, &nd);
			sz = deltasz(d, nd);
			if(sz + 32 < best){
				/*
				 * if we already picked a best delta,
				 * replace it.
				 */
				free(m->delta);
				best = sz;
				m->delta = d;
				m->ndelta = nd;
				m->nchain = p->nchain + 1;
				m->prev = p;
				m->head = p->head;
				if(m->head == nil)
					m->head = p;
			}else
				free(d);
		}
		unref(o);
	}
	for(i = max(0, nmeta - 10); i < nmeta; i++)
		dtclear(&meta[i]->dtab);
	fprint(2, "\b\b\b\b100%%\n");
}

static int
compread(void *p, void *dst, int n)
{
	Buf *b;

	b = p;
	if(n > b->sz - b->off)
		n = b->sz - b->off;
	memcpy(dst, b->data + b->off, n);
	b->off += n;
	return n;
}

static int
compwrite(void *p, void *buf, int n)
{
	return hwrite(((Compout *)p)->bfd, buf, n, &((Compout*)p)->st);
}

static int
hcompress(Biobuf *bfd, void *buf, int sz, DigestState **st)
{
	int r;
	Buf b ={
		.off=0,
		.data=buf,
		.sz=sz,
	};
	Compout o = {
		.bfd = bfd,
		.st = *st,
	};

	r = deflatezlib(&o, compwrite, &b, compread, 6, 0);
	*st = o.st;
	return r;
}

static void
append(char **p, int *len, int *sz, void *seg, int nseg)
{
	if(*len + nseg >= *sz){
		while(*len + nseg >= *sz)
			*sz += *sz/2;
		*p = erealloc(*p, *sz);
	}
	memcpy(*p + *len, seg, nseg);
	*len += nseg;
}

static int
encodedelta(Meta *m, Object *o, Object *b, void **pp)
{
	char *p, *bp, buf[16];
	int len, sz, n, i, j;
	Delta *d;

	sz = 128;
	len = 0;
	p = emalloc(sz);

	/* base object size */
	buf[0] = b->size & 0x7f;
	n = b->size >> 7;
	for(i = 1; n > 0; i++){
		buf[i - 1] |= 0x80;
		buf[i] = n & 0x7f;
		n >>= 7;
	}
	append(&p, &len, &sz, buf, i);

	/* target object size */
	buf[0] = o->size & 0x7f;
	n = o->size >> 7;
	for(i = 1; n > 0; i++){
		buf[i - 1] |= 0x80;
		buf[i] = n & 0x7f;
		n >>= 7;
	}
	append(&p, &len, &sz, buf, i);
	for(j = 0; j < m->ndelta; j++){
		d = &m->delta[j];
		if(d->cpy){
			n = d->off;
			bp = buf + 1;
			buf[0] = 0x81;
			buf[1] = 0x00;
			for(i = 0; i < sizeof(buf); i++) {
				buf[0] |= 1<<i;
				*bp++ = n & 0xff;
				n >>= 8;
				if(n == 0)
					break;
			}

			n = d->len;
			if(n != 0x10000) {
				buf[0] |= 0x1<<4;
				for(i = 0; i < sizeof(buf)-4 && n > 0; i++){
					buf[0] |= 1<<(i + 4);
					*bp++ = n & 0xff;
					n >>= 8;
				}
			}
			append(&p, &len, &sz, buf, bp - buf);
		}else{
			n = 0;
			while(n != d->len){
				buf[0] = (d->len - n < 127) ? d->len - n : 127;
				append(&p, &len, &sz, buf, 1);
				append(&p, &len, &sz, o->data + d->off + n, buf[0]);
				n += buf[0];
			}
		}
	}
	*pp = p;
	return len;
}

static int
packhdr(char *hdr, int ty, int len)
{
	int i;

	hdr[0] = ty << 4;
	hdr[0] |= len & 0xf;
	len >>= 4;
	for(i = 1; len != 0; i++){
		hdr[i-1] |= 0x80;
		hdr[i] = len & 0x7f;
		len >>= 7;
	}
	return i;
}

static int
packoff(char *hdr, vlong off)
{
	int i, j;
	char rbuf[8];

	rbuf[0] = off & 0x7f;
	for(i = 1; (off >>= 7) != 0; i++)
		rbuf[i] = (--off & 0x7f)|0x80;

	j = 0;
	while(i > 0)
		hdr[j++] = rbuf[--i];
	return j;
}

static int
genpack(int fd, Meta **meta, int nmeta, Hash *h, int odelta)
{
	int i, nh, nd, res, pct, ret;
	DigestState *st;
	Biobuf *bfd;
	Meta *m;
	Object *o, *po, *b;
	char *p, buf[32];

	st = nil;
	ret = -1;
	pct = 0;
	dprint(1, "generating pack\n");
	if((fd = dup(fd, -1)) == -1)
		return -1;
	if((bfd = Bfdopen(fd, OWRITE)) == nil)
		return -1;
	if(hwrite(bfd, "PACK", 4, &st) == -1)
		return -1;
	PUTBE32(buf, 2);
	if(hwrite(bfd, buf, 4, &st) == -1)
		return -1;
	PUTBE32(buf, nmeta);
	if(hwrite(bfd, buf, 4, &st) == -1)
		return -1;
	qsort(meta, nmeta, sizeof(Meta*), writeordercmp);
	if(interactive)
		fprint(2, "writing %d objects:   0%%", nmeta);
	for(i = 0; i < nmeta; i++){
		pct = showprogress((i*100)/nmeta, pct);
		m = meta[i];
		if((m->off = Boffset(bfd)) == -1)
			goto error;
		if((o = readobject(m->obj->hash)) == nil)
			return -1;
		if(m->delta == nil){
			nh = packhdr(buf, o->type, o->size);
			if(hwrite(bfd, buf, nh, &st) == -1)
				goto error;
			if(hcompress(bfd, o->data, o->size, &st) == -1)
				goto error;
		}else{
			if((b = readobject(m->prev->obj->hash)) == nil)
				goto error;
			nd = encodedelta(m, o, b, &p);
			unref(b);
			if(odelta && m->prev->off != 0){
				nh = 0;
				nh += packhdr(buf, GOdelta, nd);
				nh += packoff(buf+nh, m->off - m->prev->off);
				if(hwrite(bfd, buf, nh, &st) == -1)
					goto error;
			}else{
				nh = packhdr(buf, GRdelta, nd);
				po = m->prev->obj;
				if(hwrite(bfd, buf, nh, &st) == -1)
					goto error;
				if(hwrite(bfd, po->hash.h, sizeof(po->hash.h), &st) == -1)
					goto error;
			}
			res = hcompress(bfd, p, nd, &st);
			free(p);
			if(res == -1)
				goto error;
		}
		unref(o);
	}
	if(interactive)
		fprint(2, "\b\b\b\b100%%\n");
	sha1(nil, 0, h->h, st);
	if(Bwrite(bfd, h->h, sizeof(h->h)) == -1)
		goto error;
	ret = 0;
error:
	if(Bterm(bfd) == -1)
		return -1;
	return ret;
}

int
writepack(int fd, Hash *theirs, int ntheirs, Hash *ours, int nours, Hash *h)
{
	Meta **meta;
	int i, r, nmeta;

	if((nmeta = readmeta(theirs, ntheirs, ours, nours, &meta)) == -1)
		return -1;
	pickdeltas(meta, nmeta);
	r = genpack(fd, meta, nmeta, h, 0);
	for(i = 0; i < nmeta; i++)
		freemeta(meta[i]);
	free(meta);
	return r;
}
