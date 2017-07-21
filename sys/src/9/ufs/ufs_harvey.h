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


// TODO HARVEY Is this really necessary in Harvey?  We only need to distinguish
// between UFS1 and 2.
typedef struct vop_vector {
	//vop_vector *vop_default;
	//int (*vop_open)(vop_open_args *);
	//int (*vop_access)(vop_access_args *);
} vop_vector;


/* Harvey equivalent to FreeBSD vnode, but not exactly the same.  Acts as a
 * wrapper for the inode and any associated data.  This is not intended to be
 * support multiple filesystems and should probably be renamed after it works.
 */
typedef struct vnode {
	/*
	 * Fields which define the identity of the vnode.  These fields are
	 * owned by the filesystem (XXX: and vgone() ?)
	 */
	const char *v_tag;			/* u type of underlying data */
	vop_vector *v_op;			/* u vnode operations vector */
	inode	*v_data;
	//MountPoint	*v_mount;
	enum	vtype v_type;			/* u vnode type */
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

int getnewvnode(const char *tag, MountPoint *mp, vop_vector *vops, vnode **vpp);
