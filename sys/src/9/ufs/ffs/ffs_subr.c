/*-
 * Copyright (c) 1982, 1986, 1989, 1993
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
 *
 *	@(#)ffs_subr.c	8.5 (Berkeley) 3/21/95
 */

#include "u.h"
#include "../../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"

#include <ufs/freebsd_util.h>
#include "ufs_harvey.h"

#include "ufs/quota.h"
#include <ffs/fs.h>
#include "ufs/ufsmount.h"
#include "ufs/inode.h"
#include "ufs/dinode.h"

/*
 * Return buffer with the contents of block "offset" from the beginning of
 * directory "ip".  If "res" is non-zero, fill it in with a pointer to the
 * remaining space in the directory.
 */
int
ffs_blkatoff(vnode *vp, off_t offset, char **res, void **bpp)
{
	inode *ip;
	Fs *fs;
	void *bp;
	ufs_lbn_t lbn;
	int bsize, error;

	ip = VTOI(vp);
	fs = ITOFS(ip);
	lbn = lblkno(fs, offset);
	bsize = blksize(fs, ip, lbn);

	*bpp = nil;
	error = bread(vp->mount, lbn, bsize, /*NOCRED,*/ &bp); 
	if (error) {
		free(bp);
		return (error);
	}
	if (res)
		*res = (char *)bp + blkoff(fs, offset);
	*bpp = bp;
	return (0);
}

/*
 * Load up the contents of an inode and copy the appropriate pieces
 * to the incore copy.
 */
void
ffs_load_inode(void *buf, inode *ip, Fs *fs, ino_t ino)
{
	*ip->i_din2 = *((ufs2_dinode *)buf + ino_to_fsbo(fs, ino));
	ip->i_mode = ip->i_din2->di_mode;
	ip->i_nlink = ip->i_din2->di_nlink;
	ip->i_size = ip->i_din2->di_size;
	ip->i_flags = ip->i_din2->di_flags;
	ip->i_gen = ip->i_din2->di_gen;
	ip->i_uid = ip->i_din2->di_uid;
	ip->i_gid = ip->i_din2->di_gid;
}
