#include "stdinc.h"
#include <bio.h>

typedef struct Source Source;

struct Source
{
	ulong gen;
	int psize;
	int dsize;
	int dir;
	int active;
	int depth;
	uvlong size;
	uchar score[VtScoreSize];
	int reserved;
};

int bsize;
Biobuf *bout;
VtRootLump root;
int ver;
int cmp;
int all;
int find;
uchar fscore[VtScoreSize];
int dirSize;
void (*parse)(Source*, uchar*);
VtSession *z;

int vtGetUint16(uchar *p);
ulong vtGetUint32(uchar *p);
uvlong vtGetUint48(uchar *p);
void usage(void);
int parseScore(uchar *score, char *buf, int n);
void readRoot(VtRootLump*, uchar *score, char *file);
void parse1(Source*, uchar*);
void parse2(Source*, uchar*);
int dumpDir(Source*, int indent);

void
main(int argc, char *argv[])
{
	char *host = nil;
	uchar score[VtScoreSize];
	Source source;
	uchar buf[VtMaxLumpSize];
	char *p;
	int n;

	ARGBEGIN{
	case 'h':
		host = ARGF();
		break;
	case 'c':
		cmp++;
		break;
	case 'f':
		find++;
		p = ARGF();
		if(p == nil || !parseScore(fscore, p, strlen(p)))
			usage();
		break;
	case 'a':
		all = 1;
		break;
	}ARGEND

	vtAttach();

	bout = vtMemAllocZ(sizeof(Biobuf));
	Binit(bout, 1, OWRITE);

	if(argc > 1)
		usage();

	vtAttach();

	fmtinstall('V', vtScoreFmt);
	fmtinstall('R', vtErrFmt);

	z = vtDial(host);
	if(z == nil)
		vtFatal("could not connect to server: %s", vtGetError());

	if(!vtConnect(z, 0))
		sysfatal("vtConnect: %r");

	readRoot(&root, score, argv[0]);
	ver = vtGetUint16(root.version);
	bsize = vtGetUint16(root.blockSize);
	if(!find) {
		Bprint(bout, "score: %V\n", score);
		Bprint(bout, "version: %d\n", ver);
		Bprint(bout, "name: %s\n", root.name);
		Bprint(bout, "type: %s\n", root.type);
		Bprint(bout, "bsize: %d\n", bsize);
		Bprint(bout, "prev: %V\n", root.prev);
	}

	switch(ver) {
	default:
		sysfatal("unknown version");
	case VtRootVersion1:
		dirSize = VtDirEntrySize1;
		parse = parse1;
		break;
	case VtRootVersion2:
		dirSize = VtDirEntrySize2;
		parse = parse2;
		break;
	}

	n = vtRead(z, root.score, VtDirType, buf, bsize);
	if(n < 0)
		sysfatal("could not read root dir");

	/* fake up top level source */
	memset(&source, 0, sizeof(source));
	memmove(source.score, root.score, VtScoreSize);
	source.psize = bsize;
	source.dsize = bsize;
	source.dir = 1;
	source.active = 1;
	source.depth = 0;
	source.size = n;

	dumpDir(&source, 0);

	Bterm(bout);

	vtClose(z);
	vtDetach();
	exits(0);
}

void
sourcePrint(Source *s, int indent, int entry)
{
	int i;
	uvlong size;
	int ne;

	for(i=0; i<indent; i++)
		Bprint(bout, " ");
	Bprint(bout, "%4d", entry);
	if(s->active) {
		/* dir size in directory entries */
		if(s->dir) {
			ne = s->dsize/dirSize;
			size = ne*(s->size/s->dsize) + (s->size%s->dsize)/dirSize;
		} else 
			size = s->size;
		if(cmp) {
			Bprint(bout, ": gen: %lud size: %llud",
				s->gen, size);
			if(!s->dir)
				Bprint(bout, ": %V", s->score);
		} else {
			Bprint(bout, ": gen: %lud psize: %d dsize: %d",
				s->gen, s->psize, s->dsize);
			Bprint(bout, " depth: %d size: %llud: %V",
				s->depth, size, s->score);
		}
		
		if(s->reserved)
			Bprint(bout, ": reserved not emtpy");
	}
	Bprint(bout, "\n");
}

void
parse1(Source *s, uchar *p)
{
	VtDirEntry1 *dir;
	int i;

	memset(s, 0, sizeof(*s));
	dir = (VtDirEntry1*)p;
	if(!(dir->flag & VtDirEntryActive)) {
		for(i = 0; i < VtDirEntrySize1; i++) {
			if(p[i]) {
				s->reserved++;
				break;
			}
		}
		return;
	}

	s->active = 1;
	s->psize = vtGetUint16(dir->psize);
	s->dsize = vtGetUint16(dir->dsize);
	s->size = vtGetUint48(dir->size);
	memmove(s->score, dir->score, VtScoreSize);
	if(dir->flag & VtDirEntryDir)
		s->dir = 1;
	s->depth = (dir->flag & VtDirEntryDepthMask) >> VtDirEntryDepthShift;

	if(dir->flag & ~0x1f)
		s->reserved++;
}

void
parse2(Source *s, uchar *p)
{
	VtDirEntry2 *dir;
	int i;

	memset(s, 0, sizeof(*s));
	dir = (VtDirEntry2*)p;
	if(!(dir->flag & VtDirEntryActive)) {
		/* check everything but gen is zero */
		for(i = 4; i < VtDirEntrySize2; i++) {
			if(p[i]) {
				s->reserved++;
				break;
			}
		}
		return;
	}

	s->active = 1;
	s->gen = vtGetUint32(dir->gen);
	s->psize = vtGetUint16(dir->psize);
	s->dsize = vtGetUint16(dir->dsize);
	s->size = vtGetUint48(dir->size);
	memmove(s->score, dir->score, VtScoreSize);
	if(dir->flag & VtDirEntryDir)
		s->dir = 1;
	s->depth = (dir->flag & VtDirEntryDepthMask) >> VtDirEntryDepthShift;

	if(dir->flag & ~0x1f)
		s->reserved++;
	for(i = 0; i < sizeof(dir->reserved); i++) {
		if(dir->reserved[i]) {
			s->reserved++;
			break;
		}
	}
}

int
sourceRead(Source *s, ulong block, uchar *p, int n)
{
	uchar buf[VtMaxLumpSize];
	uchar score[VtScoreSize];
	int i, nn, np, type;
	int elem[VtPointerDepth];

	memmove(score, s->score, VtScoreSize);

	np = s->psize/VtScoreSize;
	for(i=0; i<s->depth; i++) {
		elem[i] = block % np;
		block /= np;
	}
	assert(block == 0);

	for(i=s->depth-1; i>=0; i--) {
		nn = vtRead(z, score, VtPointerType0+i, buf, s->psize);
		if(nn < 0)
			return -1;

		if(!vtSha1Check(score, buf, nn)) {
			vtSetError("vtSha1Check failed on root block");
			return -1;
		}

		if((elem[i]+1)*VtScoreSize > nn)
			return 0;
		memmove(score, buf + elem[i]*VtScoreSize, VtScoreSize);
	}

	if(s->dir)
		type = VtDirType;
	else
		type = VtDataType;

	nn = vtRead(z, score, type, p, n);
	if(nn < 0)
		return -1;

	if(!vtSha1Check(score, p, nn)) {
		vtSetError("vtSha1Check failed on root block");
		return -1;
	}
	
	return nn;
}

void
dumpFileContents(Source *s)
{
	int nb, lb, i, n;
	uchar buf[VtMaxLumpSize];

	nb = (s->size + s->dsize - 1)/s->dsize;
	lb = s->size%s->dsize;
	for(i=0; i<nb; i++) {
		memset(buf, 0, s->dsize);
		n = sourceRead(s, i, buf, s->dsize);
		if(n < 0) {	
			fprint(2, "could not read block: %d: %s\n", i, vtGetError());
			continue;
		}
		if(i < nb-1)
			Bwrite(bout, buf, s->dsize);
		else
			Bwrite(bout, buf, lb);
	}
}

void
dumpFile(Source *s, int indent)
{
	int nb, i, j, n;
	uchar buf[VtMaxLumpSize];
	uchar score[VtScoreSize];

	nb = (s->size + s->dsize - 1)/s->dsize;
	for(i=0; i<nb; i++) {
		memset(buf, 0, s->dsize);
		n = sourceRead(s, i, buf, s->dsize);
		if(n < 0) {	
			fprint(2, "could not read block: %d: %s\n", i, vtGetError());
			continue;
		}
		for(j=0; j<indent; j++)
			Bprint(bout, " ");
		vtSha1(score, buf, n);		
		Bprint(bout, "%4d: size: %ud: %V\n", i, n, score);
	}
}

int
dumpDir(Source *s, int indent)
{
	int pb, ne, nb, i, j, n, entry;
	uchar buf[VtMaxLumpSize];
	Source ss;

	pb = s->dsize/dirSize;
	ne = pb*(s->size/s->dsize) + (s->size%s->dsize)/dirSize;
	nb = (s->size + s->dsize - 1)/s->dsize;
	for(i=0; i<nb; i++) {
		memset(buf, 0, s->dsize);
		n = sourceRead(s, i, buf, s->dsize);
		if(n < 0) {	
			fprint(2, "could not read block: %d: %s\n", i, vtGetError());
			continue;
		}
		for(j=0; j<pb; j++) {
			entry = i*pb + j;
			if(entry >= ne)
				break;
			parse(&ss, buf + j * dirSize);

			if(!find)
				sourcePrint(&ss, indent, entry);
			else if(memcmp(ss.score, fscore, VtScoreSize) == 0) {
				dumpFileContents(&ss);
				return 0;
			}

			if(ss.dir) {
				if(!dumpDir(&ss, indent+1))
					return 0;
			} else if(all)
				dumpFile(&ss, indent+1);
		}
	}
	return 1;
}

void
usage(void)
{
	fprint(2, "%s: [file]\n", argv0);
	exits("usage");
}

int
parseScore(uchar *score, char *buf, int n)
{
	int i, c;

	memset(score, 0, VtScoreSize);

	if(n < VtScoreSize*2)
		return 0;
	for(i=0; i<VtScoreSize*2; i++) {
		if(buf[i] >= '0' && buf[i] <= '9')
			c = buf[i] - '0';
		else if(buf[i] >= 'a' && buf[i] <= 'f')
			c = buf[i] - 'a' + 10;
		else if(buf[i] >= 'A' && buf[i] <= 'F')
			c = buf[i] - 'A' + 10;
		else {
			return 0;
		}

		if((i & 1) == 0)
			c <<= 4;
	
		score[i>>1] |= c;
	}
	return 1;
}

void
readRoot(VtRootLump *root, uchar *score, char *file)
{
	int fd;
	char buf[100];
	int i, n, nn;

	if(file == 0)
		fd = 0;
	else {
		fd = open(file, OREAD);
		if(fd < 0)
			sysfatal("could not open file: %s: %r\n", file);
	}
	n = readn(fd, buf, sizeof(buf)-1);
	if(n < 0)
		sysfatal("read failed: %r\n");
	buf[n] = 0;
	close(fd);

	for(i=0; i<n; i++) {
		if(!parseScore(score, buf+i, n-i))
			continue;
		nn = vtRead(z, score, VtRootType, (uchar*)root, VtRootSize);
		if(nn >= 0) {
			if(nn != VtRootSize)
				sysfatal("vtRead on root too short");
	
			if(!vtSha1Check(score, (uchar*)root, VtRootSize))
				sysfatal("vtSha1Check failed on root block");
			root->name[sizeof(root->name)-1] = 0;
			root->type[sizeof(root->type)-1] = 0;
			return;
		}
	}

	sysfatal("could not find root");
}
