/*
 * This file is part of the Harvey operating system.  It is subject to the
 * license terms of the GNU GPL v2 in LICENSE.gpl found in the top-level
 * directory of this distribution and at http://www.gnu.org/licenses/gpl-2.0.txt
 *
 * No part of Harvey operating system, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms
 * contained in the LICENSE.gpl file.
 */


typedef struct Chan Chan;
typedef struct ufsmount ufsmount;
typedef struct vnode vnode;
typedef struct thread thread;
typedef struct inode inode;


/* Wrapper for a UFS mount.  Should support reading from both kernel and user
 * space (eventually)
 */
typedef struct MountPoint {
	ufsmount	*mnt_data;
	Chan		*chan;

	uint64_t	mnt_flag;		/* (i) flags shared with user */
	QLock		mnt_lock;		/* (mnt_mtx) structure lock */
} MountPoint;


// Not sure we even need this - if not we can remove it later.
typedef struct thread {
} thread;


// Not sure we need this either - can be removed if not.
typedef struct Ucred {
} Ucred;


/* Hopefully we can replace this with something already in Harvey - e.g. Biobuf?
 * postponing the decision until later.
 */ 
typedef struct Buf {
} Buf;


typedef struct ComponentName {
	/*
	 * Arguments to lookup.
	 */
	unsigned long cn_nameiop;	/* namei operation */
	uint64_t cn_flags;		/* flags to namei */
	//struct	thread *cn_thread;/* thread requesting lookup */
	struct	ucred *cn_cred;		/* credentials */
	int	cn_lkflags;		/* Lock flags LK_EXCLUSIVE or LK_SHARED */
	/*
	 * Shared between lookup and commit routines.
	 */
	char	*cn_pnbuf;		/* pathname buffer */
	char	*cn_nameptr;		/* pointer to looked up name */
	long	cn_namelen;		/* length of looked up component */
} ComponentName;


/*
 * Vnode types.  VNON means no type.
 */
enum vtype { VNON, VREG, VDIR, VBLK, VCHR, VLNK, VSOCK, VFIFO, VBAD, VMARKER };
typedef enum vtype Vtype;


/*
 * Conversion tables for conversion from vnode types to inode formats
 * and back.
 */
static Vtype iftovt_tab[16] = {
	VNON, VFIFO, VCHR, VNON, VDIR, VNON, VBLK, VNON,
	VREG, VNON, VLNK, VNON, VSOCK, VNON, VNON, VBAD,
};

#define S_IFMT 0170000		/* type of file */
#define	IFTOVT(mode)	(iftovt_tab[((mode) & S_IFMT) >> 12])


/*
 * Vnode flags.
 *	VI flags are protected by interlock and live in v_iflag
 *	VV flags are protected by the vnode lock and live in v_vflag
 *
 *	VI_DOOMED is doubly protected by the interlock and vnode lock.  Both
 *	are required for writing but the status may be checked with either.
 */
//#define	VI_MOUNT	0x0020	/* Mount in progress */
//#define	VI_DOOMED	0x0080	/* This vnode is being recycled */
//#define	VI_FREE		0x0100	/* This vnode is on the freelist */
//#define	VI_ACTIVE	0x0200	/* This vnode is on the active list */
//#define	VI_DOINGINACT	0x0800	/* VOP_INACTIVE is in progress */
//#define	VI_OWEINACT	0x1000	/* Need to call inactive */

#define	VV_ROOT		0x0001	/* root of its filesystem */
//#define	VV_ISTTY	0x0002	/* vnode represents a tty */
//#define	VV_NOSYNC	0x0004	/* unlinked, stop syncing */
//#define	VV_ETERNALDEV	0x0008	/* device that is never destroyed */
//#define	VV_CACHEDLABEL	0x0010	/* Vnode has valid cached MAC label */
//#define	VV_TEXT		0x0020	/* vnode is a pure text prototype */
//#define	VV_COPYONWRITE	0x0040	/* vnode is doing copy-on-write */
//#define	VV_SYSTEM	0x0080	/* vnode being used by kernel */
//#define	VV_PROCDEP	0x0100	/* vnode is process dependent */
//#define	VV_NOKNOTE	0x0200	/* don't activate knotes on this vnode */
//#define	VV_DELETED	0x0400	/* should be removed */
//#define	VV_MD		0x0800	/* vnode backs the md device */
//#define	VV_FORCEINSMQ	0x1000	/* force the insmntque to succeed */

//#define	VMP_TMPMNTFREELIST	0x0001	/* Vnode is on mnt's tmp free list */


/* Harvey equivalent to FreeBSD vnode, but not exactly the same.  Acts as a
 * wrapper for the inode and any associated data.  This is not intended to be
 * support multiple filesystems and should probably be renamed after it works.
 * It may be unnecessary - if so, we should use inodes directly.
 */
typedef struct vnode {
	/*
	 * Fields which define the identity of the vnode.  These fields are
	 * owned by the filesystem (XXX: and vgone() ?)
	 */
	const char 	*v_tag;			/* u type of underlying data */
	inode		*v_data;
	MountPoint	*v_mount;
	
	enum vtype 	v_type;			/* u vnode type */
	unsigned int	v_vflag;		/* v vnode flags */
} vnode;


/*
 * MAXBSIZE -	Filesystems are made out of blocks of at most MAXBSIZE bytes
 *		per block.  MAXBSIZE may be made larger without effecting
 *		any existing filesystems as long as it does not exceed MAXPHYS,
 *		and may be made smaller at the risk of not being able to use
 *		filesystems which require a block size exceeding MAXBSIZE.
 */
#define MAXBSIZE	65536	/* must be power of 2 */


/*
 * Flags to various vnode functions.
 */
#define	FORCECLOSE	0x0002	/* vflush: force file closure */


MountPoint *newufsmount(Chan *c);

vnode* newufsvnode();

void releaseufsmount(MountPoint *mp);
void releaseufsvnode(vnode *vn);

int ffs_mount(MountPoint *mp);
int ffs_unmount(MountPoint *mp, int mntflags);

int ufs_root(MountPoint *mp, int flags, vnode **vpp);
int ufs_lookup(MountPoint *mp);

int getnewvnode(const char *tag, MountPoint *mp, vnode **vpp);
