#include "stdinc.h"
#include "vac.h"
#include "dat.h"
#include "fns.h"

static char EBadVacFormat[] = "bad format for vac file";

static VacFS *
vacFSAlloc(VtSession *z, int bsize, long ncache)
{
	VacFS *fs;

	fs = vtMemAllocZ(sizeof(VacFS));
	fs->ref = 1;
	fs->z = z;
	fs->bsize = bsize;
	fs->cache = cacheAlloc(z, bsize, ncache);
	return fs;
}

static int
readScore(int fd, uchar score[VtScoreSize])
{
	char buf[44];
	int i, n, c;

	n = readn(fd, buf, sizeof(buf));
	if(n < sizeof(buf)) {
		vtSetError("short read");
		return 0;
	}
	if(strncmp(buf, "vac:", 4) != 0) {
		vtSetError("not a vac file");
		return 0;
	}
	memset(score, 0, VtScoreSize);
	for(i=4; i<sizeof(buf); i++) {
		if(buf[i] >= '0' && buf[i] <= '9')
			c = buf[i] - '0';
		else if(buf[i] >= 'a' && buf[i] <= 'f')
			c = buf[i] - 'a' + 10;
		else if(buf[i] >= 'A' && buf[i] <= 'F')
			c = buf[i] - 'A' + 10;
		else {
			vtSetError("bad format for venti score");
			return 0;
		}
		if((i & 1) == 0)
			c <<= 4;
	
		score[(i>>1)-2] |= c;
	}
	return 1;
}

VacFS *
vacFSOpen(VtSession *z, char *file, int readOnly, long ncache)
{
	VacFS *fs;
	int n, fd;
	VtRootLump rt;
	uchar score[VtScoreSize];
	int bsize;
	VacFile *root;

	fd = open(file, OREAD);
	if(fd < 0) {
		vtOSError();
		return nil;
	}

	if(!readScore(fd, score)) {
		close(fd);
		return nil;
	}
	close(fd);

	n = vtRead(z, score, VtRootType, (uchar*)&rt, VtRootSize);
	if(n < 0)
		return nil;
	if(n != VtRootSize) {
		vtSetError("vtRead on root too short");
		return nil;
	}

	if(!vtSha1Check(score, (uchar*)&rt, VtRootSize)) {
		vtSetError("vtSha1Check failed on root block");	
		return nil;
	}

	rt.name[sizeof(rt.name)-1] = 0;
	rt.type[sizeof(rt.type)-1] = 0;
	bsize = (rt.blockSize[0]<<8) | rt.blockSize[1];
if(0) fprint(2, "name=%s type=%s size=%d\n", rt.name, rt.type, bsize);

	if(((rt.version[0]<<8)|rt.version[1]) != VtRootVersion2) {
		vtSetError("unknown root block version");
		return nil;
	}
		

	if(strcmp(rt.type, "vac") != 0) {
		vtSetError("not a vac root");
		return nil;
	}

	fs = vacFSAlloc(z, bsize, ncache);
	memmove(fs->score, score, VtScoreSize);
	fs->readOnly = readOnly;
	root = vacFileRoot(fs, rt.score);
	if(root == nil)
		goto Err;
	fs->root = root;

	return fs;
Err:
	if(root)
		vacFileDecRef(root);
	vacFSClose(fs);
	return nil;
}

VacFS *
vacFsCreate(VtSession *z, int bsize, long ncache)
{
	VacFS *fs;

	fs = vacFSAlloc(z, bsize, ncache);
	return fs;
}

int
vacFSGetBlockSize(VacFS *fs)
{
	return fs->bsize;
}

int
vacFSGetScore(VacFS *fs, uchar score[VtScoreSize])
{
	memmove(fs, score, VtScoreSize);
	return 1;
}

long
vacFSGetCacheSize(VacFS *fs)
{
	return cacheGetSize(fs->cache);
}

int
vacFSSetCacheSize(VacFS *fs, long size)
{
	return cacheSetSize(fs->cache, size);
}

int
vacFSSnapshot(VacFS *fs, char *src, char *dst)
{
	USED(fs);
	USED(src);
	USED(dst);
	return 1;
}

int
vacFSSync(VacFS*)
{
	return 1;
}

int
vacFSClose(VacFS *fs)
{
	if(fs->root)
		vacFileDecRef(fs->root);
	fs->root = nil;
	cacheCheck(fs->cache);
	cacheFree(fs->cache);
	memset(fs, 0, sizeof(VacFS));
	vtMemFree(fs);
	return 1;
}


