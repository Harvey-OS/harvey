/*
 * Copyright (c) 2002 Networks Associates Technology, Inc.
 * All rights reserved.
 *
 * This software was developed for the FreeBSD Project by Marshall
 * Kirk McKusick and Network Associates Laboratories, the Security
 * Research Division of Network Associates, Inc. under DARPA/SPAWAR
 * contract N66001-01-C-8035 ("CBOSS"), as part of the DARPA CHATS
 * research program.
 *
 * Copyright (c) 1980, 1989, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <u.h>
#include <libc.h>

#include <ufs/ufsdat.h>
#include <ufs/dir.h>
#include <ffs/fs.h>
#include <ufs/dinode.h>
#include <ufs/libufs.h>
#include "newfs.h"


/*
 * make file system for cylinder-group style file systems
 */
#define UMASK		0755
#define POWEROF2(num)	(((num) & ((num) - 1)) == 0)

#define HOWMANY(x, y)	(((x)+((y)-1))/(y))
#define ROUNDUP(x, y)	(HOWMANY((x), (y))*(y))

#define MAXPHYS		(128 * 1024)	/* max raw I/O transfer size */

#define CHAR_BIT 8

/*
 * The size of a cylinder group is calculated by CGSIZE. The maximum size
 * is limited by the fact that cylinder groups are at most one block.
 * Its size is derived from the size of the maps maintained in the
 * cylinder group and the (struct cg) size.
 */
#define	CGSIZE(fs) \
    /* base cg */	(sizeof(Cg) + sizeof(int32_t) + \
    /* old btotoff */	(fs)->fs_old_cpg * sizeof(int32_t) + \
    /* old boff */	(fs)->fs_old_cpg * sizeof(uint16_t) + \
    /* inode map */	HOWMANY((fs)->fs_ipg, NBBY) + \
    /* block map */	HOWMANY((fs)->fs_fpg, NBBY) +\
    /* if present */	((fs)->fs_contigsumsize <= 0 ? 0 : \
    /* cluster sum */	(fs)->fs_contigsumsize * sizeof(int32_t) + \
    /* cluster map */	HOWMANY(fragstoblks(fs, (fs)->fs_fpg), NBBY)))

static struct	csum *fscs;
#define	sblock	disk.d_fs
#define	acg	disk.d_cg

union dinode {
	struct ufs1_dinode dp1;
	struct ufs2_dinode dp2;
};

static caddr_t iobuf;
static long iobufsize;
static ufs2_daddr_t alloc(int size, int mode);
static void clrblock(Fs *, unsigned char *, int);
static void fsinit(int32_t);
static int ilog2(int);
static void initcg(int, int32_t);
static int isblock(Fs *, unsigned char *, int);
static void iput(union dinode *, ino_t);
static int makedir(Direct *, int);
static void setblock(Fs *, unsigned char *, int);
static void wtfs(ufs2_daddr_t, int, char *);
static uint32_t newfs_random();

static int
do_sbwrite(Uufsd *disk)
{
	if (!disk->d_sblock)
		disk->d_sblock = disk->d_fs.fs_sblockloc / disk->d_bsize;
	return (pwrite(disk->d_fd, &disk->d_fs, SBLOCKSIZE,
		(off_t)((disk->d_sblock) * disk->d_bsize)));
}

void
mkfs(char *filename)
{
	int fragsperinode, optimalfpg, origdensity, minfpg, lastminfpg;
	long i, j, csfrags;
	uint cg;
	int32_t utime;
	int64_t sizepb;
	int width = 60;
	ino_t maxinum;
	int minfragsperinode;	/* minimum ratio of frags to inodes */
	char tmpbuf[100];	/* XXX this will break in about 2,500 years */

	/*
	 * Our blocks == sector size, and the version of UFS we are using is
	 * specified by Oflag.
	 */
	disk.d_bsize = sectorsize;
	disk.d_ufs = 2;			// Only UFS2
	if (regressiontest)
		utime = 1000000000;
	else
		time(&utime);
	sblock.fs_old_flags = FS_FLAGS_UPDATED;
	sblock.fs_flags = 0;
	if (enablesu)
		sblock.fs_flags |= FS_DOSOFTDEP;
	if (addvolumelabel)
		strlcpy((char*)sblock.fs_volname, volumelabel, MAXVOLLEN);
	if (enablemultilabel)
		sblock.fs_flags |= FS_MULTILABEL;
	if (enabletrim)
		sblock.fs_flags |= FS_TRIM;
	/*
	 * Validate the given file system size.
	 * Verify that its last block can actually be accessed.
	 * Convert to file system fragment sized units.
	 */
	if (fssize <= 0) {
		fprint(2, "preposterous file system size %lld\n", fssize);
		exits("preposterous file system size");
	}
	wtfs(fssize - (realsectorsize / DEV_BSIZE), realsectorsize,
	    (char *)&sblock);
	/*
	 * collect and verify the file system density info
	 */
	sblock.fs_avgfilesize = avgfilesize;
	sblock.fs_avgfpdir = avgfilesperdir;
	if (sblock.fs_avgfilesize <= 0) {
		fprint(2, "illegal expected average file size %d\n",
		    sblock.fs_avgfilesize);
		exits("illegal expected average file size");
	}
	if (sblock.fs_avgfpdir <= 0) {
		fprint(2, "illegal expected number of files per directory %d\n",
		    sblock.fs_avgfpdir);
		exits("illegal expected number of files per directory");
	}

restart:
	/*
	 * collect and verify the block and fragment sizes
	 */
	sblock.fs_bsize = bsize;
	sblock.fs_fsize = fsize;
	if (!POWEROF2(sblock.fs_bsize)) {
		fprint(2, "block size must be a power of 2, not %d\n",
		    sblock.fs_bsize);
		exits("block size not a power of 2");
	}
	if (!POWEROF2(sblock.fs_fsize)) {
		fprint(2, "fragment size must be a power of 2, not %d\n",
		    sblock.fs_fsize);
		exits("fragment size not a power of 2");
	}
	if (sblock.fs_fsize < sectorsize) {
		fprint(2, "increasing fragment size from %d to sector size (%d)\n",
		    sblock.fs_fsize, sectorsize);
		sblock.fs_fsize = sectorsize;
	}
	if (sblock.fs_bsize > MAXBSIZE) {
		fprint(2, "decreasing block size from %d to maximum (%d)\n",
		    sblock.fs_bsize, MAXBSIZE);
		sblock.fs_bsize = MAXBSIZE;
	}
	if (sblock.fs_bsize < MINBSIZE) {
		fprint(2, "increasing block size from %d to minimum (%d)\n",
		    sblock.fs_bsize, MINBSIZE);
		sblock.fs_bsize = MINBSIZE;
	}
	if (sblock.fs_fsize > MAXBSIZE) {
		fprint(2, "decreasing fragment size from %d to maximum (%d)\n",
		    sblock.fs_fsize, MAXBSIZE);
		sblock.fs_fsize = MAXBSIZE;
	}
	if (sblock.fs_bsize < sblock.fs_fsize) {
		fprint(2, "increasing block size from %d to fragment size (%d)\n",
		    sblock.fs_bsize, sblock.fs_fsize);
		sblock.fs_bsize = sblock.fs_fsize;
	}
	if (sblock.fs_fsize * MAXFRAG < sblock.fs_bsize) {
		fprint(2,
		"increasing fragment size from %d to block size / %d (%d)\n",
		    sblock.fs_fsize, MAXFRAG, sblock.fs_bsize / MAXFRAG);
		sblock.fs_fsize = sblock.fs_bsize / MAXFRAG;
	}
	if (maxbsize == 0)
		maxbsize = bsize;
	if (maxbsize < bsize || !POWEROF2(maxbsize)) {
		sblock.fs_maxbsize = sblock.fs_bsize;
		fprint(2, "Extent size set to %d\n", sblock.fs_maxbsize);
	} else if (sblock.fs_maxbsize > FS_MAXCONTIG * sblock.fs_bsize) {
		sblock.fs_maxbsize = FS_MAXCONTIG * sblock.fs_bsize;
		fprint(2, "Extent size reduced to %d\n", sblock.fs_maxbsize);
	} else {
		sblock.fs_maxbsize = maxbsize;
	}
	/*
	 * Maxcontig sets the default for the maximum number of blocks
	 * that may be allocated sequentially. With file system clustering
	 * it is possible to allocate contiguous blocks up to the maximum
	 * transfer size permitted by the controller or buffering.
	 */
	if (maxcontig == 0)
		maxcontig = MAX(1, MAXPHYS / bsize);
	sblock.fs_maxcontig = maxcontig;
	if (sblock.fs_maxcontig < sblock.fs_maxbsize / sblock.fs_bsize) {
		sblock.fs_maxcontig = sblock.fs_maxbsize / sblock.fs_bsize;
		fprint(2, "Maxcontig raised to %d\n", sblock.fs_maxbsize);
	}
	if (sblock.fs_maxcontig > 1)
		sblock.fs_contigsumsize = MIN(sblock.fs_maxcontig,FS_MAXCONTIG);
	sblock.fs_bmask = ~(sblock.fs_bsize - 1);
	sblock.fs_fmask = ~(sblock.fs_fsize - 1);
	sblock.fs_qbmask = ~sblock.fs_bmask;
	sblock.fs_qfmask = ~sblock.fs_fmask;
	sblock.fs_bshift = ilog2(sblock.fs_bsize);
	sblock.fs_fshift = ilog2(sblock.fs_fsize);
	sblock.fs_frag = numfrags(&sblock, sblock.fs_bsize);
	sblock.fs_fragshift = ilog2(sblock.fs_frag);
	if (sblock.fs_frag > MAXFRAG) {
		fprint(2, "fragment size %d is still too small (can't happen)\n",
		    sblock.fs_bsize / MAXFRAG);
		exits("fragment size too small");
	}
	sblock.fs_fsbtodb = ilog2(sblock.fs_fsize / sectorsize);
	sblock.fs_size = fssize = dbtofsb(&sblock, fssize);

	/*
	 * Before the filesystem is finally initialized, mark it
	 * as incompletely initialized.
	 */
	sblock.fs_magic = FS_BAD_MAGIC;

	sblock.fs_sblockloc = SBLOCK_UFS2;
	sblock.fs_nindir = sblock.fs_bsize / sizeof(ufs2_daddr_t);
	sblock.fs_inopb = sblock.fs_bsize / sizeof(struct ufs2_dinode);
	sblock.fs_maxsymlinklen = ((UFS_NDADDR + UFS_NIADDR) * sizeof(ufs2_daddr_t));
	sblock.fs_sblkno =
	    ROUNDUP(HOWMANY(sblock.fs_sblockloc + SBLOCKSIZE, sblock.fs_fsize),
		sblock.fs_frag);
	sblock.fs_cblkno = sblock.fs_sblkno +
	    ROUNDUP(HOWMANY(SBLOCKSIZE, sblock.fs_fsize), sblock.fs_frag);
	sblock.fs_iblkno = sblock.fs_cblkno + sblock.fs_frag;
	sblock.fs_maxfilesize = sblock.fs_bsize * UFS_NDADDR - 1;
	for (sizepb = sblock.fs_bsize, i = 0; i < UFS_NIADDR; i++) {
		sizepb *= NINDIR(&sblock);
		sblock.fs_maxfilesize += sizepb;
	}

	/*
	 * It's impossible to create a snapshot in case that fs_maxfilesize
	 * is smaller than the fssize.
	 */
	if (sblock.fs_maxfilesize < fssize) {
		fprint(2, "WARNING: You will be unable to create snapshots on this "
		      "file system.  Correct by using a larger blocksize.");
	}

	/*
	 * Calculate the number of blocks to put into each cylinder group.
	 *
	 * This algorithm selects the number of blocks per cylinder
	 * group. The first goal is to have at least enough data blocks
	 * in each cylinder group to meet the density requirement. Once
	 * this goal is achieved we try to expand to have at least
	 * MINCYLGRPS cylinder groups. Once this goal is achieved, we
	 * pack as many blocks into each cylinder group map as will fit.
	 *
	 * We start by calculating the smallest number of blocks that we
	 * can put into each cylinder group. If this is too big, we reduce
	 * the density until it fits.
	 */
	maxinum = (((int64_t)(1)) << 32) - INOPB(&sblock);
	minfragsperinode = 1 + fssize / maxinum;
	if (density == 0) {
		density = MAX(NFPI, minfragsperinode) * fsize;
	} else if (density < minfragsperinode * fsize) {
		origdensity = density;
		density = minfragsperinode * fsize;
		fprint(2, "density increased from %d to %d\n",
		    origdensity, density);
	}
	origdensity = density;
	for (;;) {
		fragsperinode = MAX(numfrags(&sblock, density), 1);
		if (fragsperinode < minfragsperinode) {
			bsize <<= 1;
			fsize <<= 1;
			fprint(2, "Block size too small for a file system %s %d\n",
			     "of this size. Increasing blocksize to", bsize);
			goto restart;
		}
		minfpg = fragsperinode * INOPB(&sblock);
		if (minfpg > sblock.fs_size)
			minfpg = sblock.fs_size;
		sblock.fs_ipg = INOPB(&sblock);
		sblock.fs_fpg = ROUNDUP(sblock.fs_iblkno +
		    sblock.fs_ipg / INOPF(&sblock), sblock.fs_frag);
		if (sblock.fs_fpg < minfpg)
			sblock.fs_fpg = minfpg;
		sblock.fs_ipg = ROUNDUP(HOWMANY(sblock.fs_fpg, fragsperinode),
		    INOPB(&sblock));
		sblock.fs_fpg = ROUNDUP(sblock.fs_iblkno +
		    sblock.fs_ipg / INOPF(&sblock), sblock.fs_frag);
		if (sblock.fs_fpg < minfpg)
			sblock.fs_fpg = minfpg;
		sblock.fs_ipg = ROUNDUP(HOWMANY(sblock.fs_fpg, fragsperinode),
		    INOPB(&sblock));
		if (CGSIZE(&sblock) < (unsigned long)sblock.fs_bsize)
			break;
		density -= sblock.fs_fsize;
	}
	if (density != origdensity)
		fprint(2, "density reduced from %d to %d\n", origdensity, density);
	/*
	 * Start packing more blocks into the cylinder group until
	 * it cannot grow any larger, the number of cylinder groups
	 * drops below MINCYLGRPS, or we reach the size requested.
	 * For UFS1 inodes per cylinder group are stored in an int16_t
	 * so fs_ipg is limited to 2^15 - 1.
	 */
	for ( ; sblock.fs_fpg < maxblkspercg; sblock.fs_fpg += sblock.fs_frag) {
		sblock.fs_ipg = ROUNDUP(HOWMANY(sblock.fs_fpg, fragsperinode),
		    INOPB(&sblock));
		if (sblock.fs_size / sblock.fs_fpg < MINCYLGRPS)
			break;
		if (CGSIZE(&sblock) < (unsigned long)sblock.fs_bsize)
			continue;
		if (CGSIZE(&sblock) == (unsigned long)sblock.fs_bsize)
			break;
		sblock.fs_fpg -= sblock.fs_frag;
		sblock.fs_ipg = ROUNDUP(HOWMANY(sblock.fs_fpg, fragsperinode),
		    INOPB(&sblock));
		break;
	}
	/*
	 * Check to be sure that the last cylinder group has enough blocks
	 * to be viable. If it is too small, reduce the number of blocks
	 * per cylinder group which will have the effect of moving more
	 * blocks into the last cylinder group.
	 */
	optimalfpg = sblock.fs_fpg;
	for (;;) {
		sblock.fs_ncg = HOWMANY(sblock.fs_size, sblock.fs_fpg);
		lastminfpg = ROUNDUP(sblock.fs_iblkno +
		    sblock.fs_ipg / INOPF(&sblock), sblock.fs_frag);
		if (sblock.fs_size < lastminfpg) {
			fprint(2, "Filesystem size %lld < minimum size of %d\n",
			    sblock.fs_size, lastminfpg);
			exits("filesystem size less than minimum size");
		}
		if (sblock.fs_size % sblock.fs_fpg >= lastminfpg ||
		    sblock.fs_size % sblock.fs_fpg == 0)
			break;
		sblock.fs_fpg -= sblock.fs_frag;
		sblock.fs_ipg = ROUNDUP(HOWMANY(sblock.fs_fpg, fragsperinode),
		    INOPB(&sblock));
	}
	if (optimalfpg != sblock.fs_fpg)
		fprint(2, "Reduced frags per cylinder group from %d to %d %s\n",
		   optimalfpg, sblock.fs_fpg, "to enlarge last cyl group");
	sblock.fs_cgsize = fragroundup(&sblock, CGSIZE(&sblock));
	sblock.fs_dblkno = sblock.fs_iblkno + sblock.fs_ipg / INOPF(&sblock);
	/*
	 * fill in remaining fields of the super block
	 */
	sblock.fs_csaddr = cgdmin(&sblock, 0);
	sblock.fs_cssize =
	    fragroundup(&sblock, sblock.fs_ncg * sizeof(struct csum));
	fscs = (struct csum *)calloc(1, sblock.fs_cssize);
	if (fscs == nil) {
		fprint(2, "calloc failed");
		exits("calloc failed");
	}
	sblock.fs_sbsize = fragroundup(&sblock, sizeof(Fs));
	if (sblock.fs_sbsize > SBLOCKSIZE)
		sblock.fs_sbsize = SBLOCKSIZE;
	sblock.fs_minfree = minfree;
	if (metaspace > 0 && metaspace < sblock.fs_fpg / 2)
		sblock.fs_metaspace = blknum(&sblock, metaspace);
	else if (metaspace != -1)
		/* reserve half of minfree for metadata blocks */
		sblock.fs_metaspace = blknum(&sblock,
		    (sblock.fs_fpg * minfree) / 200);
	if (maxbpg == 0)
		sblock.fs_maxbpg = MAXBLKPG(sblock.fs_bsize);
	else
		sblock.fs_maxbpg = maxbpg;
	sblock.fs_optim = opt;
	sblock.fs_cgrotor = 0;
	sblock.fs_pendingblocks = 0;
	sblock.fs_pendinginodes = 0;
	sblock.fs_fmod = 0;
	sblock.fs_ronly = 0;
	sblock.fs_state = 0;
	sblock.fs_clean = 1;
	sblock.fs_id[0] = (long)utime;
	sblock.fs_id[1] = newfs_random();
	sblock.fs_fsmnt[0] = '\0';
	csfrags = HOWMANY(sblock.fs_cssize, sblock.fs_fsize);
	sblock.fs_dsize = sblock.fs_size - sblock.fs_sblkno -
	    sblock.fs_ncg * (sblock.fs_dblkno - sblock.fs_sblkno);
	sblock.fs_cstotal.cs_nbfree =
	    fragstoblks(&sblock, sblock.fs_dsize) -
	    HOWMANY(csfrags, sblock.fs_frag);
	sblock.fs_cstotal.cs_nffree =
	    fragnum(&sblock, sblock.fs_size) +
	    (fragnum(&sblock, csfrags) > 0 ?
	     sblock.fs_frag - fragnum(&sblock, csfrags) : 0);
	sblock.fs_cstotal.cs_nifree =
	    sblock.fs_ncg * sblock.fs_ipg - UFS_ROOTINO;
	sblock.fs_cstotal.cs_ndir = 0;
	sblock.fs_dsize -= csfrags;
	sblock.fs_time = utime;

	/*
	 * Dump out summary information about file system.
	 */
#define B2MBFACTOR (1 / (1024.0 * 1024.0))
	print("%s: %.1fMB (%lld sectors) block size %d, fragment size %d\n",
	    filename, (float)sblock.fs_size * sblock.fs_fsize * B2MBFACTOR,
	    fsbtodb(&sblock, sblock.fs_size), sblock.fs_bsize,
	    sblock.fs_fsize);
	print("\tusing %d cylinder groups of %.2fMB, %d blks, %d inodes.\n",
	    sblock.fs_ncg, (float)sblock.fs_fpg * sblock.fs_fsize * B2MBFACTOR,
	    sblock.fs_fpg / sblock.fs_frag, sblock.fs_ipg);
	if (sblock.fs_flags & FS_DOSOFTDEP)
		fprint(2, "\twith soft updates\n");
#undef B2MBFACTOR

	if (erasecontents && createfs) {
		fprint(2, "Erasing sectors [%jd...%jd]\n", 
		    sblock.fs_sblockloc / disk.d_bsize,
		    fsbtodb(&sblock, sblock.fs_size) - 1);
		berase(&disk, sblock.fs_sblockloc / disk.d_bsize,
		    sblock.fs_size * sblock.fs_fsize - sblock.fs_sblockloc);
	}

	if (createfs)
		do_sbwrite(&disk);
	if (exitflag == 1) {
		fprint(2, "** Exiting on exitflag 1\n");
		exits("xflag1");
	}
	if (exitflag == 2)
		fprint(2, "** Leaving BAD MAGIC on exitflag 2\n");
	else
		sblock.fs_magic = FS_UFS2_MAGIC;

	/*
	 * Now build the cylinders group blocks and
	 * then print out indices of cylinder groups.
	 */
	print("super-block backups (for fsck_ffs -b #) at:\n");
	i = 0;
	/*
	 * allocate space for superblock, cylinder group map, and
	 * two sets of inode blocks.
	 */
	if (sblock.fs_bsize < SBLOCKSIZE)
		iobufsize = SBLOCKSIZE + 3 * sblock.fs_bsize;
	else
		iobufsize = 4 * sblock.fs_bsize;
	if ((iobuf = calloc(1, iobufsize)) == 0) {
		fprint(2, "Cannot allocate I/O buffer\n");
		exits("cannot allocate io buffer");
	}
	/*
	 * Make a copy of the superblock into the buffer that we will be
	 * writing out in each cylinder group.
	 */
	memcpy(iobuf, (char *)&sblock, SBLOCKSIZE);
	for (cg = 0; cg < sblock.fs_ncg; cg++) {
		initcg(cg, utime);
		j = snprint(tmpbuf, sizeof(tmpbuf), " %lld%s",
		    fsbtodb(&sblock, cgsblock(&sblock, cg)),
		    cg < (sblock.fs_ncg-1) ? "," : "");
		if (j < 0)
			tmpbuf[j = 0] = '\0';
		if (i + j >= width) {
			print("\n");
			i = 0;
		}
		i += j;
		print("%s", tmpbuf);
	}
	print("\n");
	if (!createfs)
		exits(nil);

	/*
	 * Now construct the initial file system,
	 * then write out the super-block.
	 */
	fsinit(utime);
	if (exitflag == 3) {
		fprint(2, "** Exiting on exitflag 3\n");
		exits("xflag3");
	}

	if (createfs) {
		do_sbwrite(&disk);
	}
	for (i = 0; i < sblock.fs_cssize; i += sblock.fs_bsize)
		wtfs(fsbtodb(&sblock, sblock.fs_csaddr + numfrags(&sblock, i)),
			MIN(sblock.fs_cssize - i, sblock.fs_bsize),
			((char *)fscs) + i);
}

/*
 * Initialize a cylinder group.
 */
void
initcg(int cylno, int32_t utime)
{
	long blkno, start;
	uint i, d, dlower, dupper;
	ufs2_daddr_t cbase, dmax;
	ufs2_dinode *dp2;
	csum *cs;

	/*
	 * Determine block bounds for cylinder group.
	 * Allow space for super block summary information in first
	 * cylinder group.
	 */
	cbase = cgbase(&sblock, cylno);
	dmax = cbase + sblock.fs_fpg;
	if (dmax > sblock.fs_size)
		dmax = sblock.fs_size;
	dlower = cgsblock(&sblock, cylno) - cbase;
	dupper = cgdmin(&sblock, cylno) - cbase;
	if (cylno == 0)
		dupper += HOWMANY(sblock.fs_cssize, sblock.fs_fsize);
	cs = &fscs[cylno];
	memset(&acg, 0, sblock.fs_cgsize);
	acg.cg_time = utime;
	acg.cg_magic = CG_MAGIC;
	acg.cg_cgx = cylno;
	acg.cg_niblk = sblock.fs_ipg;
	acg.cg_initediblk = MIN(sblock.fs_ipg, 2 * INOPB(&sblock));
	acg.cg_ndblk = dmax - cbase;
	if (sblock.fs_contigsumsize > 0)
		acg.cg_nclusterblks = acg.cg_ndblk / sblock.fs_frag;
	start = &acg.cg_space[0] - (uint8_t *)(&acg.cg_firstfield);
	acg.cg_iusedoff = start;
	acg.cg_freeoff = acg.cg_iusedoff + HOWMANY(sblock.fs_ipg, CHAR_BIT);
	acg.cg_nextfreeoff = acg.cg_freeoff + HOWMANY(sblock.fs_fpg, CHAR_BIT);
	if (sblock.fs_contigsumsize > 0) {
		acg.cg_clustersumoff =
		    ROUNDUP(acg.cg_nextfreeoff, sizeof(uint32_t));
		acg.cg_clustersumoff -= sizeof(uint32_t);
		acg.cg_clusteroff = acg.cg_clustersumoff +
		    (sblock.fs_contigsumsize + 1) * sizeof(uint32_t);
		acg.cg_nextfreeoff = acg.cg_clusteroff +
		    HOWMANY(fragstoblks(&sblock, sblock.fs_fpg), CHAR_BIT);
	}
	if (acg.cg_nextfreeoff > (unsigned)sblock.fs_cgsize) {
		fprint(2, "Panic: cylinder group too big\n");
		exits("cylinder group too big");
	}
	acg.cg_cs.cs_nifree += sblock.fs_ipg;
	if (cylno == 0)
		for (i = 0; i < (long)UFS_ROOTINO; i++) {
			setbit(cg_inosused(&acg), i);
			acg.cg_cs.cs_nifree--;
		}
	if (cylno > 0) {
		/*
		 * In cylno 0, beginning space is reserved
		 * for boot and super blocks.
		 */
		for (d = 0; d < dlower; d += sblock.fs_frag) {
			blkno = d / sblock.fs_frag;
			setblock(&sblock, cg_blksfree(&acg), blkno);
			if (sblock.fs_contigsumsize > 0)
				setbit(cg_clustersfree(&acg), blkno);
			acg.cg_cs.cs_nbfree++;
		}
	}
	if ((i = dupper % sblock.fs_frag)) {
		acg.cg_frsum[sblock.fs_frag - i]++;
		for (d = dupper + sblock.fs_frag - i; dupper < d; dupper++) {
			setbit(cg_blksfree(&acg), dupper);
			acg.cg_cs.cs_nffree++;
		}
	}
	for (d = dupper; d + sblock.fs_frag <= acg.cg_ndblk;
	     d += sblock.fs_frag) {
		blkno = d / sblock.fs_frag;
		setblock(&sblock, cg_blksfree(&acg), blkno);
		if (sblock.fs_contigsumsize > 0)
			setbit(cg_clustersfree(&acg), blkno);
		acg.cg_cs.cs_nbfree++;
	}
	if (d < acg.cg_ndblk) {
		acg.cg_frsum[acg.cg_ndblk - d]++;
		for (; d < acg.cg_ndblk; d++) {
			setbit(cg_blksfree(&acg), d);
			acg.cg_cs.cs_nffree++;
		}
	}
	if (sblock.fs_contigsumsize > 0) {
		int32_t *sump = cg_clustersum(&acg);
		uint8_t *mapp = cg_clustersfree(&acg);
		int map = *mapp++;
		int bit = 1;
		int run = 0;

		for (i = 0; i < acg.cg_nclusterblks; i++) {
			if ((map & bit) != 0)
				run++;
			else if (run != 0) {
				if (run > sblock.fs_contigsumsize)
					run = sblock.fs_contigsumsize;
				sump[run]++;
				run = 0;
			}
			if ((i & (CHAR_BIT - 1)) != CHAR_BIT - 1)
				bit <<= 1;
			else {
				map = *mapp++;
				bit = 1;
			}
		}
		if (run != 0) {
			if (run > sblock.fs_contigsumsize)
				run = sblock.fs_contigsumsize;
			sump[run]++;
		}
	}
	*cs = acg.cg_cs;
	/*
	 * Write out the duplicate super block, the cylinder group map
	 * and two blocks worth of inodes in a single write.
	 */
	start = MAX(sblock.fs_bsize, SBLOCKSIZE);
	memcpy(&iobuf[start], (char *)&acg, sblock.fs_cgsize);
	start += sblock.fs_bsize;
	dp2 = (struct ufs2_dinode *)(&iobuf[start]);
	for (i = 0; i < acg.cg_initediblk; i++) {
		dp2->di_gen = newfs_random();
		dp2++;
	}
	wtfs(fsbtodb(&sblock, cgsblock(&sblock, cylno)), iobufsize, iobuf);
}

/*
 * initialize the file system
 */
#define ROOTLINKCNT 3

static Direct root_dir[] = {
	{ UFS_ROOTINO, sizeof(Direct), DT_DIR, 1, "." },
	{ UFS_ROOTINO, sizeof(Direct), DT_DIR, 2, ".." },
	{ UFS_ROOTINO + 1, sizeof(Direct), DT_DIR, 5, ".snap" },
};

#define SNAPLINKCNT 2

static Direct snap_dir[] = {
	{ UFS_ROOTINO + 1, sizeof(Direct), DT_DIR, 1, "." },
	{ UFS_ROOTINO, sizeof(Direct), DT_DIR, 2, ".." },
};

void
fsinit(int32_t utime)
{
	// TODO HARVEY Set group
	union dinode node;
	//struct group *grp;
	//gid_t gid;
	int entries;

	memset(&node, 0, sizeof node);
	/*if ((grp = getgrnam("operator")) != nil) {
		gid = grp->gr_gid;
	} else {
		fprint(2, "Cannot retrieve operator gid, using gid 0.");
		gid = 0;
	}*/
	entries = createsnapdir ? ROOTLINKCNT : ROOTLINKCNT - 1;

	/*
	 * initialize the node
	 */
	node.dp2.di_atime = utime;
	node.dp2.di_mtime = utime;
	node.dp2.di_ctime = utime;
	node.dp2.di_birthtime = utime;
	/*
	 * create the root directory
	 */
	node.dp2.di_mode = IFDIR | UMASK;
	node.dp2.di_nlink = entries;
	node.dp2.di_size = makedir(root_dir, entries);
	node.dp2.di_db[0] = alloc(sblock.fs_fsize, node.dp2.di_mode);
	node.dp2.di_blocks = btodb(fragroundup(&sblock, node.dp2.di_size));
	wtfs(fsbtodb(&sblock, node.dp2.di_db[0]), sblock.fs_fsize, iobuf);
	iput(&node, UFS_ROOTINO);
	if (createsnapdir) {
		/*
		 * create the .snap directory
		 */
		node.dp2.di_mode |= 020;
		//node.dp2.di_gid = gid;
		node.dp2.di_nlink = SNAPLINKCNT;
		node.dp2.di_size = makedir(snap_dir, SNAPLINKCNT);
		node.dp2.di_db[0] = alloc(sblock.fs_fsize, node.dp2.di_mode);
		node.dp2.di_blocks =
		    btodb(fragroundup(&sblock, node.dp2.di_size));
			wtfs(fsbtodb(&sblock, node.dp2.di_db[0]), 
			    sblock.fs_fsize, iobuf);
		iput(&node, UFS_ROOTINO + 1);
	}
}

/*
 * construct a set of directory entries in "iobuf".
 * return size of directory.
 */
int
makedir(Direct *protodir, int entries)
{
	char *cp;
	int i, spcleft;

	spcleft = DIRBLKSIZ;
	memset(iobuf, 0, DIRBLKSIZ);
	for (cp = iobuf, i = 0; i < entries - 1; i++) {
		protodir[i].d_reclen = DIRSIZ(&protodir[i]);
		memmove(cp, &protodir[i], protodir[i].d_reclen);
		cp += protodir[i].d_reclen;
		spcleft -= protodir[i].d_reclen;
	}
	protodir[i].d_reclen = spcleft;
	memmove(cp, &protodir[i], DIRSIZ(&protodir[i]));
	return (DIRBLKSIZ);
}

/*
 * allocate a block or frag
 */
ufs2_daddr_t
alloc(int size, int mode)
{
	int i, blkno, frag;
	uint d;

	bread(&disk, fsbtodb(&sblock, cgtod(&sblock, 0)), (char *)&acg,
	    sblock.fs_cgsize);
	if (acg.cg_magic != CG_MAGIC) {
		fprint(2, "cg 0: bad magic number\n");
		exits("bad cg magic number");
	}
	if (acg.cg_cs.cs_nbfree == 0) {
		fprint(2, "first cylinder group ran out of space\n");
		exits("first cylinder group ran out of space");
	}
	for (d = 0; d < acg.cg_ndblk; d += sblock.fs_frag)
		if (isblock(&sblock, cg_blksfree(&acg), d / sblock.fs_frag))
			goto goth;
	fprint(2, "internal error: can't find block in cyl 0\n");
	exits("cannot find block in cyl 0");
goth:
	blkno = fragstoblks(&sblock, d);
	clrblock(&sblock, cg_blksfree(&acg), blkno);
	if (sblock.fs_contigsumsize > 0)
		clrbit(cg_clustersfree(&acg), blkno);
	acg.cg_cs.cs_nbfree--;
	sblock.fs_cstotal.cs_nbfree--;
	fscs[0].cs_nbfree--;
	if (mode & IFDIR) {
		acg.cg_cs.cs_ndir++;
		sblock.fs_cstotal.cs_ndir++;
		fscs[0].cs_ndir++;
	}
	if (size != sblock.fs_bsize) {
		frag = HOWMANY(size, sblock.fs_fsize);
		fscs[0].cs_nffree += sblock.fs_frag - frag;
		sblock.fs_cstotal.cs_nffree += sblock.fs_frag - frag;
		acg.cg_cs.cs_nffree += sblock.fs_frag - frag;
		acg.cg_frsum[sblock.fs_frag - frag]++;
		for (i = frag; i < sblock.fs_frag; i++)
			setbit(cg_blksfree(&acg), d + i);
	}
	/* XXX cgwrite(&disk, 0)??? */
	wtfs(fsbtodb(&sblock, cgtod(&sblock, 0)), sblock.fs_cgsize,
	    (char *)&acg);
	return ((ufs2_daddr_t)d);
}

/*
 * Allocate an inode on the disk
 */
void
iput(union dinode *ip, ino_t ino)
{
	ufs2_daddr_t d;

	bread(&disk, fsbtodb(&sblock, cgtod(&sblock, 0)), (char *)&acg,
	    sblock.fs_cgsize);
	if (acg.cg_magic != CG_MAGIC) {
		fprint(2, "cg 0: bad magic number\n");
		exits("bad cg magic number");
	}
	acg.cg_cs.cs_nifree--;
	setbit(cg_inosused(&acg), ino);
	wtfs(fsbtodb(&sblock, cgtod(&sblock, 0)), sblock.fs_cgsize,
	    (char *)&acg);
	sblock.fs_cstotal.cs_nifree--;
	fscs[0].cs_nifree--;
	if (ino >= (unsigned long)sblock.fs_ipg * sblock.fs_ncg) {
		fprint(2, "fsinit: inode value out of range (%llu).\n",
		    (uint64_t)ino);
		exits("inode value out of range");
	}
	d = fsbtodb(&sblock, ino_to_fsba(&sblock, ino));
	bread(&disk, d, (char *)iobuf, sblock.fs_bsize);
	if (sblock.fs_magic == FS_UFS1_MAGIC)
		((struct ufs1_dinode *)iobuf)[ino_to_fsbo(&sblock, ino)] =
		    ip->dp1;
	else
		((struct ufs2_dinode *)iobuf)[ino_to_fsbo(&sblock, ino)] =
		    ip->dp2;
	wtfs(d, sblock.fs_bsize, (char *)iobuf);
}

/*
 * possibly write to disk
 */
static void
wtfs(ufs2_daddr_t bno, int size, char *bf)
{
	if (!createfs)
		return;
	if (bwrite(&disk, bno, bf, size) < 0) {
		fprint(2, "wtfs: %d bytes at sector %lld", size, bno);
		exits("bwrite failed");
	}
}

/*
 * check if a block is available
 */
static int
isblock(Fs *fs, unsigned char *cp, int h)
{
	unsigned char mask;

	switch (fs->fs_frag) {
	case 8:
		return (cp[h] == 0xff);
	case 4:
		mask = 0x0f << ((h & 0x1) << 2);
		return ((cp[h >> 1] & mask) == mask);
	case 2:
		mask = 0x03 << ((h & 0x3) << 1);
		return ((cp[h >> 2] & mask) == mask);
	case 1:
		mask = 0x01 << (h & 0x7);
		return ((cp[h >> 3] & mask) == mask);
	default:
		fprint(2, "isblock bad fs_frag %d\n", fs->fs_frag);
		return (0);
	}
}

/*
 * take a block out of the map
 */
static void
clrblock(Fs *fs, unsigned char *cp, int h)
{
	switch ((fs)->fs_frag) {
	case 8:
		cp[h] = 0;
		return;
	case 4:
		cp[h >> 1] &= ~(0x0f << ((h & 0x1) << 2));
		return;
	case 2:
		cp[h >> 2] &= ~(0x03 << ((h & 0x3) << 1));
		return;
	case 1:
		cp[h >> 3] &= ~(0x01 << (h & 0x7));
		return;
	default:
		fprint(2, "clrblock bad fs_frag %d\n", fs->fs_frag);
		return;
	}
}

/*
 * put a block into the map
 */
static void
setblock(Fs *fs, unsigned char *cp, int h)
{
	switch (fs->fs_frag) {
	case 8:
		cp[h] = 0xff;
		return;
	case 4:
		cp[h >> 1] |= (0x0f << ((h & 0x1) << 2));
		return;
	case 2:
		cp[h >> 2] |= (0x03 << ((h & 0x3) << 1));
		return;
	case 1:
		cp[h >> 3] |= (0x01 << (h & 0x7));
		return;
	default:
		fprint(2, "setblock bad fs_frag %d\n", fs->fs_frag);
		return;
	}
}

static int
ilog2(int val)
{
	unsigned int n;

	for (n = 0; n < sizeof(n) * CHAR_BIT; n++)
		if (1 << n == val)
			return (n);
	fprint(2, "ilog2: %d is not a power of 2\n", val);
	exits("ilog2: not a power of 2");
	return 0;
}

/*
 * For the regression test, return predictable random values.
 * Otherwise use a true random number generator.
 */
static uint32_t
newfs_random()
{
	static int nextnum = 1;

	if (regressiontest)
		return (nextnum++);
	return (lrand());
}
