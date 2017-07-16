/*-
 * Copyright (c) 1991, 1993, 1994
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
 *	@(#)ffs_extern.h	8.6 (Berkeley) 3/30/95
 * $FreeBSD$
 */

typedef struct Buf Buf;
//struct cg;
//struct fid;
typedef struct fs fs;
//struct inode;
//struct malloc_type;
typedef struct MountPoint MountPoint;
//struct thread;
//struct sockaddr;
//struct statfs;
typedef struct Ucred Ucred;
typedef struct vnode vnode;
//struct vop_fsync_args;
//struct vop_reallocblks_args;
//struct workhead;

//int	ffs_alloc(struct inode *, ufs2_daddr_t, ufs2_daddr_t, int, int,
//	    struct ucred *, ufs2_daddr_t *);
int	ffs_balloc_ufs1(vnode *a_vp, off_t a_startoffset, int a_size,
            Ucred *a_cred, int a_flags, Buf **a_bpp);
int	ffs_balloc_ufs2(vnode *a_vp, off_t a_startoffset, int a_size,
            Ucred *a_cred, int a_flags, Buf **a_bpp);
int	ffs_blkatoff(vnode *, off_t, char **, Buf **);
/*void	ffs_blkfree(struct ufsmount *, struct fs *, struct vnode *,
	    ufs2_daddr_t, long, ino_t, enum vtype, struct workhead *);
ufs2_daddr_t ffs_blkpref_ufs1(struct inode *, ufs_lbn_t, int, ufs1_daddr_t *);
ufs2_daddr_t ffs_blkpref_ufs2(struct inode *, ufs_lbn_t, int, ufs2_daddr_t *);
int	ffs_checkfreefile(struct fs *, struct vnode *, ino_t);
void	ffs_clrblock(struct fs *, uint8_t *, ufs1_daddr_t);
void	ffs_clusteracct(struct fs *, struct cg *, ufs1_daddr_t, int);
void	ffs_bdflush(struct bufobj *, struct buf *);
int	ffs_copyonwrite(struct vnode *, struct buf *);*/
int	ffs_flushfiles(MountPoint *, int, thread *);
/*void	ffs_fragacct(struct fs *, int, int32_t [], int);
int	ffs_freefile(struct ufsmount *, struct fs *, struct vnode *, ino_t,
	    int, struct workhead *);
void	ffs_fserr(struct fs *, ino_t, char *);
int	ffs_isblock(struct fs *, uint8_t *, ufs1_daddr_t);
int	ffs_isfreeblock(struct fs *, uint8_t *, ufs1_daddr_t);
void	ffs_load_inode(struct buf *, struct inode *, struct fs *, ino_t);
void	ffs_oldfscompat_write(struct fs *, struct ufsmount *);
int	ffs_own_mount(const struct mount *mp);
int	ffs_reallocblks(struct vop_reallocblks_args *);
int	ffs_realloccg(struct inode *, ufs2_daddr_t, ufs2_daddr_t,
	    ufs2_daddr_t, int, int, int, struct ucred *, struct buf **);
int	ffs_reload(struct mount *, struct thread *, int);*/
int	ffs_sbupdate(ufsmount *, int, int);
//void	ffs_setblock(struct fs *, uint8_t *, ufs1_daddr_t);
//int	ffs_snapblkfree(fs *, vnode *, ufs2_daddr_t, long, ino_t, Vtype,
//	workhead *);
//void	ffs_snapremove(struct vnode *vp);
//int	ffs_snapshot(struct mount *mp, char *snapfile);
void	ffs_snapshot_mount(MountPoint *mp);
/*void	ffs_snapshot_unmount(struct mount *mp);
void	process_deferred_inactive(struct mount *mp);
void	ffs_sync_snap(struct mount *, int);
int	ffs_syncvnode(struct vnode *vp, int waitfor, int flags);*/
int	ffs_truncate(vnode *, off_t, int, Ucred *);
int	ffs_update(vnode *, int);
int	ffs_valloc(vnode *, int, Ucred *, vnode **);

int	ffs_vfree(vnode *, ino_t, int);
int	ffs_vget(MountPoint *mp, ino_t ino, int flags, vnode **vpp);
int	ffs_vgetf(MountPoint *, ino_t, int, vnode **, int);
/*void	ffs_susp_initialize(void);
void	ffs_susp_uninitialize(void);

#define	FFSV_FORCEINSMQ	0x0001

#define	FFSR_FORCE	0x0001
#define	FFSR_UNSUSPEND	0x0002

extern struct vop_vector ffs_vnodeops1;
extern struct vop_vector ffs_fifoops1;
extern struct vop_vector ffs_vnodeops2;
extern struct vop_vector ffs_fifoops2;*/

/*
 * Soft update function prototypes.
 */

/*int	softdep_check_suspend(struct mount *, struct vnode *,
	  int, int, int, int);
void	softdep_get_depcounts(struct mount *, int *, int *);
void	softdep_initialize(void);
void	softdep_uninitialize(void);*/
int	softdep_mount(vnode *, MountPoint *, fs *, Ucred *);
/*void	softdep_unmount(struct mount *);
int	softdep_move_dependencies(struct buf *, struct buf *);
int	softdep_flushworklist(struct mount *, int *, struct thread *);
int	softdep_flushfiles(struct mount *, int, struct thread *);
void	softdep_update_inodeblock(struct inode *, struct buf *, int);
void	softdep_load_inodeblock(struct inode *);
void	softdep_freefile(struct vnode *, ino_t, int);
int	softdep_request_cleanup(struct fs *, struct vnode *,
	    struct ucred *, int);
void	softdep_setup_freeblocks(struct inode *, off_t, int);
void	softdep_setup_inomapdep(struct buf *, struct inode *, ino_t, int);
void	softdep_setup_blkmapdep(struct buf *, struct mount *, ufs2_daddr_t,
	    int, int);
void	softdep_setup_allocdirect(struct inode *, ufs_lbn_t, ufs2_daddr_t,
	    ufs2_daddr_t, long, long, struct buf *);
void	softdep_setup_allocext(struct inode *, ufs_lbn_t, ufs2_daddr_t,
	    ufs2_daddr_t, long, long, struct buf *);
void	softdep_setup_allocindir_meta(struct buf *, struct inode *,
	    struct buf *, int, ufs2_daddr_t);
void	softdep_setup_allocindir_page(struct inode *, ufs_lbn_t,
	    struct buf *, int, ufs2_daddr_t, ufs2_daddr_t, struct buf *);
void	softdep_setup_blkfree(struct mount *, struct buf *, ufs2_daddr_t, int,
	    struct workhead *);
void	softdep_setup_inofree(struct mount *, struct buf *, ino_t,
	    struct workhead *);
void	softdep_setup_sbupdate(struct ufsmount *, struct fs *, struct buf *);
void	softdep_fsync_mountdev(struct vnode *);
int	softdep_sync_metadata(struct vnode *);
int	softdep_sync_buf(struct vnode *, struct buf *, int);
int     softdep_fsync(struct vnode *);
int	softdep_prealloc(struct vnode *, int);
int	softdep_journal_lookup(struct mount *, struct vnode **);
void	softdep_journal_freeblocks(struct inode *, struct ucred *, off_t, int);
void	softdep_journal_fsync(struct inode *);
void	softdep_buf_append(struct buf *, struct workhead *);
void	softdep_inode_append(struct inode *, struct ucred *, struct workhead *);
void	softdep_freework(struct workhead *);*/

/*
 * Things to request flushing in softdep_request_cleanup()
 */
#define	FLUSH_INODES		1
#define	FLUSH_INODES_WAIT	2
#define	FLUSH_BLOCKS		3
#define	FLUSH_BLOCKS_WAIT	4
/*
 * Flag to ffs_syncvnode() to request flushing of data only,
 * but skip the ffs_update() on the inode itself. Used to avoid
 * deadlock when flushing snapshot inodes while holding snaplk.
 */
#define	NO_INO_UPDT		0x00000001
/*
 * Request data sync only from ffs_syncvnode(), not touching even more
 * metadata than NO_INO_UPDT.
 */
#define	DATA_ONLY		0x00000002

int	ffs_rdonly(inode *);

#if 0

TAILQ_HEAD(snaphead, inode);

struct snapdata {
	LIST_ENTRY(snapdata) sn_link;
	struct snaphead sn_head;
	daddr_t sn_listsize;
	daddr_t *sn_blklist;
	struct lock sn_lock;
};

#endif // 0
