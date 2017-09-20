/*
 * Copyright (c) 2002 Juli Mallett.  All rights reserved.
 *
 * This software was written by Juli Mallett <jmallett@FreeBSD.org> for the
 * FreeBSD project.  Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that the following
 * conditions are met:
 *
 * 1. Redistribution of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistribution in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <u.h>
#include <libc.h>

#include <ufs/ufsdat.h>
#include <ffs/fs.h>
#include <ufs/libufs.h>

static int superblocks[] = SBLOCKSEARCH;

#define HOWMANY(x, y)	(((x)+((y)-1))/(y))

int
sbread(Uufsd *disk)
{
	uint8_t block[MAXBSIZE];
	Fs *fs;
	int sb, superblock;
	int i, size, blks;
	uint8_t *space;

	libufserror(disk, nil);

	fs = &disk->d_fs;
	superblock = superblocks[0];

	for (sb = 0; (superblock = superblocks[sb]) != -1; sb++) {
		if (bread(disk, superblock, disk->d_sb, SBLOCKSIZE) == -1) {
			libufserror(disk, "non-existent or truncated superblock");
			return (-1);
		}
		if (fs->fs_magic == FS_UFS1_MAGIC)
			disk->d_ufs = 1;
		if (fs->fs_magic == FS_UFS2_MAGIC &&
		    fs->fs_sblockloc == superblock)
			disk->d_ufs = 2;
		if (fs->fs_bsize <= MAXBSIZE &&
		    (size_t)fs->fs_bsize >= sizeof(*fs)) {
			if (disk->d_ufs)
				break;
		}
		disk->d_ufs = 0;
	}
	if (superblock == -1 || disk->d_ufs == 0) {
		libufserror(disk, "no usable known superblock found");
		return (-1);
	}
	disk->d_bsize = fs->fs_fsize / fsbtodb(fs, 1);
	disk->d_sblock = superblock / disk->d_bsize;
	/*
	 * Read in the superblock summary information.
	 */
	size = fs->fs_cssize;
	blks = HOWMANY(size, fs->fs_fsize);
	size += fs->fs_ncg * sizeof(int32_t);
	space = malloc(size);
	if (space == nil) {
		libufserror(disk, "failed to allocate space for summary information");
		return (-1);
	}
	fs->fs_csp = (struct csum *)space;
	for (i = 0; i < blks; i += fs->fs_frag) {
		size = fs->fs_bsize;
		if (i + fs->fs_frag > blks)
			size = (blks - i) * fs->fs_fsize;
		if (bread(disk, fsbtodb(fs, fs->fs_csaddr + i), block, size)
		    == -1) {
			libufserror(disk, "Failed to read sb summary information");
			free(fs->fs_csp);
			return (-1);
		}
		memcpy(space, block, size);
		space += size;
	}
	fs->fs_maxcluster = (int32_t *)space;
	disk->d_sbcsum = fs->fs_csp;

	return (0);
}

int
sbwrite(Uufsd *disk, int all)
{
	Fs *fs;
	int blks, size;
	uint8_t *space;
	unsigned i;

	libufserror(disk, nil);

	fs = &disk->d_fs;

	if (!disk->d_sblock) {
		disk->d_sblock = disk->d_fs.fs_sblockloc / disk->d_bsize;
	}

	if (bwrite(disk, disk->d_sblock, fs, SBLOCKSIZE) == -1) {
		libufserror(disk, "failed to write superblock");
		return (-1);
	}
	/*
	 * Write superblock summary information.
	 */
	blks = HOWMANY(fs->fs_cssize, fs->fs_fsize);
	space = (uint8_t *)disk->d_sbcsum;
	for (i = 0; i < blks; i += fs->fs_frag) {
		size = fs->fs_bsize;
		if (i + fs->fs_frag > blks)
			size = (blks - i) * fs->fs_fsize;
		if (bwrite(disk, fsbtodb(fs, fs->fs_csaddr + i), space, size)
		    == -1) {
			libufserror(disk, "Failed to write sb summary information");
			return (-1);
		}
		space += size;
	}
	if (all) {
		for (i = 0; i < fs->fs_ncg; i++)
			if (bwrite(disk, fsbtodb(fs, cgsblock(fs, i)),
			    fs, SBLOCKSIZE) == -1) {
				libufserror(disk, "failed to update a superblock");
				return (-1);
			}
	}
	return (0);
}
