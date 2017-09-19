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
 *
 * $FreeBSD$
 */

/*
 * libufs structures.
 */

/*
 * userland ufs disk.
 */
typedef struct Uufsd {
	const char *d_name;	/* disk name */
	int d_ufs;		/* decimal UFS version */
	int d_fd;		/* raw device file descriptor */
	long d_bsize;		/* device bsize */
	ufs2_daddr_t d_sblock;	/* superblock location */
	struct csum *d_sbcsum;	/* Superblock summary info */
	caddr_t d_inoblock;	/* inode block */
	ino_t d_inomin;		/* low inode */
	ino_t d_inomax;		/* high inode */
	union {
		Fs d_fs;	/* filesystem information */
		char d_sb[MAXBSIZE];
				/* superblock as buffer */
	} d_sbunion;
	union {
		Cg d_cg;	/* cylinder group */
		char d_buf[MAXBSIZE];
				/* cylinder group storage */
	} d_cgunion;
	int d_ccg;		/* current cylinder group */
	int d_lcg;		/* last cylinder group (in d_cg) */
	const char *d_error;	/* human readable disk error */
	int d_mine;		/* internal flags */
#define	d_fs	d_sbunion.d_fs
#define	d_sb	d_sbunion.d_sb
#define	d_cg	d_cgunion.d_cg
} Uufsd;


/*
 * libufs prototypes.
 */

/*
 * block.c
 */
int32_t bread(Uufsd *, ufs2_daddr_t, void *, size_t);
int32_t bwrite(Uufsd *, ufs2_daddr_t, const void *, size_t);
void libufserror(Uufsd *u, const char *str);

/*
 * cgroup.c
 */
ufs2_daddr_t cgballoc(Uufsd *);
int cgbfree(Uufsd *, ufs2_daddr_t, long);
ino_t cgialloc(Uufsd *);
int cgread(Uufsd *);
int cgread1(Uufsd *, int);
int cgwrite(Uufsd *);
int cgwrite1(Uufsd *, int);

/*
 * inode.c
 */
int getino(Uufsd *, void **, ino_t, int *);
int putino(Uufsd *);

/*
 * sblock.c
 */
int sbread(Uufsd *);
int sbwrite(Uufsd *, int);

/*
 * type.c
 */
int ufs_disk_close(Uufsd *);
int ufs_disk_fillout(Uufsd *, const char *);
int ufs_disk_fillout_blank(Uufsd *, const char *);
int ufs_disk_write(Uufsd *);

/*
 * ffs_subr.c
 */
void	ffs_clrblock(Fs *, uint8_t *, ufs1_daddr_t);
void	ffs_clusteracct(Fs *, Cg *, ufs1_daddr_t, int);
void	ffs_fragacct(Fs *, int, int32_t [], int);
int	ffs_isblock(Fs *, uint8_t *, ufs1_daddr_t);
int	ffs_isfreeblock(Fs *, uint8_t *, ufs1_daddr_t);
void	ffs_setblock(Fs *, uint8_t *, ufs1_daddr_t);
