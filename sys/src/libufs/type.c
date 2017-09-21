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

/* Internally, track the 'name' value, it's ours. */
#define	MINE_NAME	0x01
/* Track if its fd points to a writable device. */
#define	MINE_WRITE	0x02

int
ufs_disk_close(Uufsd *disk)
{
	libufserror(disk, nil);
	close(disk->d_fd);
	if (disk->d_inoblock != nil) {
		free(disk->d_inoblock);
		disk->d_inoblock = nil;
	}
	if (disk->d_mine & MINE_NAME) {
		free((char *)(uintptr_t)disk->d_name);
		disk->d_name = nil;
	}
	if (disk->d_sbcsum != nil) {
		free(disk->d_sbcsum);
		disk->d_sbcsum = nil;
	}
	return (0);
}

int
ufs_disk_write(Uufsd *disk)
{
	libufserror(disk, nil);

	if (disk->d_mine & MINE_WRITE)
		return (0);

	close(disk->d_fd);

	disk->d_fd = open(disk->d_name, ORDWR);
	if (disk->d_fd < 0) {
		libufserror(disk, "failed to open disk for writing");
		return (-1);
	}

	disk->d_mine |= MINE_WRITE;

	return (0);
}
