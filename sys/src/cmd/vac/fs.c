#include "stdinc.h"
#include "vac.h"
#include "dat.h"
#include "fns.h"

static char EBadVacFormat[] = "bad format for vac file";

static VacFS *
vfsAlloc(VtSession *z, int bsize, long ncache)
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
readScore(char* file, uchar score[VtScoreSize])
{
	char buf[44];
	int fd, i, n, c;

	fd = open(file, OREAD);
	if(fd < 0) {
		vtSetError("can't open '%s': %r", file);
		return 0;
	}

	n = readn(fd, buf, sizeof(buf));
	close(fd);
	if(n < sizeof(buf)) {
		vtSetError("read '%s': too short", file);
		return 0;
	}
	if(strncmp(buf, "vac:", 4) != 0) {
		vtSetError("'%s': not a vac file", file);
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
			vtSetError("'%s': bad format for venti score", file);
			return 0;
		}
		if((i & 1) == 0)
			c <<= 4;
	
		score[(i>>1)-2] |= c;
	}
	return 1;
}

VacFS *
vfsOpen(VtSession *z, char *file, int readOnly, long ncache)
{
	VacFS *fs;
	int n;
	VtRoot rt;
	uchar score[VtScoreSize], buf[VtRootSize];
	VacFile *root;

	if(!readScore(file, score))
		return nil;

	n = vtRead(z, score, VtRootType, buf, VtRootSize);
	if(n < 0){
		fprint(2, "cannot read %V: %r\n", score);
		return nil;
	}
	if(n != VtRootSize) {
		vtSetError("vtRead on root too short");
		return nil;
	}

	if(!vtSha1Check(score, buf, VtRootSize)) {
		vtSetError("vtSha1Check failed on root block");	
		return nil;
	}

	if(!vtRootUnpack(&rt, buf))
		return nil;

	if(strcmp(rt.type, "vac") != 0) {
		vtSetError("not a vac root");
		return nil;
	}

	fs = vfsAlloc(z, rt.blockSize, ncache);
	vfsGetScore(fs, score);
	fs->readOnly = readOnly;
	root = vfRoot(fs, rt.score);
	if(root == nil)
		goto Err;
	fs->root = root;

	return fs;
Err:
	if(root)
		vfDecRef(root);
	vfsClose(fs);
	return nil;
}

VacFS *
vacFsCreate(VtSession *z, int bsize, long ncache)
{
	return vfsAlloc(z, bsize, ncache);
}

int
vfsIsReadOnly(VacFS *fs)
{
	return fs->readOnly != 0;
}

VacFile *
vfsGetRoot(VacFS *fs)
{
	return vfIncRef(fs->root);
}

int
vfsGetBlockSize(VacFS *fs)
{
	return fs->bsize;
}

int
vfsGetScore(VacFS *fs, uchar score[VtScoreSize])
{
	memmove(fs->score, score, VtScoreSize);
	return 1;
}

long
vfsGetCacheSize(VacFS *fs)
{
	return cacheGetSize(fs->cache);
}

int
vfsSetCacheSize(VacFS *fs, long size)
{
	return cacheSetSize(fs->cache, size);
}

int
vfsSnapshot(VacFS *fs, char *src, char *dst)
{
	USED(fs);
	USED(src);
	USED(dst);
	return 1;
}

int
vfsSync(VacFS*)
{
	return 1;
}

int
vfsClose(VacFS *fs)
{
	if(fs->root)
		vfDecRef(fs->root);
	fs->root = nil;
	cacheCheck(fs->cache);
	cacheFree(fs->cache);
	memset(fs, 0, sizeof(VacFS));
	vtMemFree(fs);
	return 1;
}


