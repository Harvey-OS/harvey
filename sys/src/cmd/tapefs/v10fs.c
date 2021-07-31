/*
 * 10th edition 4K file system
 */
#include <u.h>
#include <libc.h>
#include <auth.h>
#include <fcall.h>
#include "tapefs.h"

/*
 * v10 disk inode
 */
#define	VNADDR	13
#define	VFMT	0160000
#define	VIFREG	0100000
#define	VIFDIR	0040000
#define	VIFCHR	0120000
#define	VIFBLK	0160000
#define	VMODE	0777
#define	VSUPERB	1
#define	VROOT		2	/* root inode */
#define	VNAMELEN	14
#define	BLSIZE	4096
#define	LINOPB	(BLSIZE/sizeof(struct v10dinode))
#define	LNINDIR	(BLSIZE/sizeof(unsigned long))

struct v10dinode {
	unsigned char flags[2];
	unsigned char nlinks[2];
	unsigned char uid[2];
	unsigned char gid[2];
	unsigned char size[4];
	unsigned char addr[40];
	unsigned char atime[4];
	unsigned char mtime[4];
	unsigned char ctime[4];
};

struct	v10dir {
	uchar	ino[2];
	char	name[VNAMELEN];
};

int	tapefile;
Fileinf	iget(int ino);
long	bmap(Ram *r, long bno);
void	getblk(Ram *r, long bno, char *buf);

void
populate(char *name)
{
	Fileinf f;

	replete = 0;
	tapefile = open(name, OREAD);
	if (tapefile<0)
		error("Can't open argument file");
	f = iget(VROOT);
	ram->perm = f.mode;
	ram->mtime = f.mdate;
	ram->addr = f.addr;
	ram->data = f.data;
	ram->ndata = f.size;
}

void
popdir(Ram *r)
{
	int i, ino;
	char *cp;
	struct v10dir *dp;
	Fileinf f;
	char name[VNAMELEN+1];

	cp = 0;
	for (i=0; i<r->ndata; i+=sizeof(struct v10dir)) {
		if (i%BLSIZE==0)
			cp = doread(r, i, BLSIZE);
		dp = (struct v10dir *)(cp+i%BLSIZE);
		ino = g2byte(dp->ino);
		if (strcmp(dp->name, ".")==0 || strcmp(dp->name, "..")==0)
			continue;
		if (ino==0)
			continue;
		f = iget(ino);
		strncpy(name, dp->name, VNAMELEN);
		name[VNAMELEN+1] = '\0';
		f.name = name;
		popfile(r, f);
	}
	r->replete = 1;
}

void
dotrunc(Ram *r)
{
	USED(r);
}

void
docreate(Ram *r)
{
	USED(r);
}

char *
doread(Ram *r, vlong off, long cnt)
{
	static char buf[Maxbuf+BLSIZE];
	int bno, i;

	bno = off/BLSIZE;
	off -= bno*BLSIZE;
	if (cnt>Maxbuf)
		error("count too large");
	if (off)
		cnt += off;
	i = 0;
	while (cnt>0) {
		getblk(r, bno, &buf[i*BLSIZE]);
		cnt -= BLSIZE;
		bno++;
		i++;
	}
	return buf+off;
}

void
dowrite(Ram *r, char *buf, long off, long cnt)
{
	USED(r); USED(buf); USED(off); USED(cnt);
}

int
dopermw(Ram *r)
{
	USED(r);
	return 0;
}

/*
 * fetch an i-node
 * -- no sanity check for now
 * -- magic inode-to-disk-block stuff here
 */

Fileinf
iget(int ino)
{
	char buf[BLSIZE];
	struct v10dinode *dp;
	long flags, i;
	Fileinf f;

	seek(tapefile, BLSIZE*((ino-1)/LINOPB + VSUPERB + 1), 0);
	if (read(tapefile, buf, BLSIZE) != BLSIZE)
		error("Can't read inode");
	dp = ((struct v10dinode *)buf) + ((ino-1)%LINOPB);
	flags = g2byte(dp->flags);
	f.size = g4byte(dp->size);
	if ((flags&VFMT)==VIFCHR || (flags&VFMT)==VIFBLK)
		f.size = 0;
	f.data = emalloc(VNADDR*sizeof(long));
	for (i = 0; i < VNADDR; i++)
		((long*)f.data)[i] = g3byte(dp->addr+3*i);
	f.mode = flags & VMODE;
	if ((flags&VFMT)==VIFDIR)
		f.mode |= DMDIR;
	f.uid = g2byte(dp->uid);
	f.gid = g2byte(dp->gid);	
	f.mdate = g4byte(dp->mtime);
	return f;
}

void
getblk(Ram *r, long bno, char *buf)
{
	long dbno;

	if ((dbno = bmap(r, bno)) == 0) {
		memset(buf, 0, BLSIZE);
		return;
	}
	seek(tapefile, dbno*BLSIZE, 0);
	if (read(tapefile, buf, BLSIZE) != BLSIZE)
		error("bad read");
}

/*
 * logical to physical block
 * only singly-indirect files for now
 */

long
bmap(Ram *r, long bno)
{
	unsigned char indbuf[LNINDIR][sizeof(long)];

	if (bno < VNADDR-3)
		return ((long*)r->data)[bno];
	if (bno < VNADDR*LNINDIR) {
		seek(tapefile, ((long *)r->data)[(bno-(VNADDR-3))/LNINDIR+(VNADDR-3)]*BLSIZE, 0);
		if (read(tapefile, (char *)indbuf, BLSIZE) != BLSIZE)
			return 0;
		return ((indbuf[(bno-(VNADDR-3))%LNINDIR][2]<<16) + (indbuf[(bno-(VNADDR-3))%LNINDIR][1]<<8)
			+ indbuf[(bno-(VNADDR-3))%LNINDIR][0]);
	}
	return 0;
}
