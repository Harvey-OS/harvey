/*
 * This file is part of the Harvey operating system.  It is subject to the
 * license terms of the GNU GPL v2 in LICENSE.gpl found in the top-level
 * directory of this distribution and at http://www.gnu.org/licenses/gpl-2.0.txt
 *
 * No part of Harvey operating system, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms
 * contained in the LICENSE.gpl file.
 */

// Functions to be exposed outside the UFS code (e.g. to devufs)

typedef struct Chan Chan;
typedef struct ufsmount ufsmount;
typedef struct vnode vnode;
typedef struct inode inode;


/*
 * Vnode types.  VNON means no type.
 */
enum vtype { VNON, VREG, VDIR, VLNK, VBAD, VMARKER };
typedef enum vtype Vtype;


// Not sure we even need this - if not we can remove it later.
typedef struct thread {
} thread;

// Not sure we need this either - can be removed if not.
typedef struct Ucred {
} Ucred;


typedef struct Buf {
	vnode*	vnode;
	void*	data;
	size_t	bcount;		/* Requested size of buffer */
	int64_t	offset;		/* Offset into file. */
} Buf;


typedef struct Uio {
	int64_t	offset;		/* offset in target object */
	int64_t	resid;		/* remaining bytes to process */

	Block*	dest;
} Uio;


/*
 * filesystem statistics
 */
typedef struct StatFs {
	uint64_t f_iosize;		/* optimal transfer block size */
} StatFs;


/* Wrapper for a UFS mount.  Should support reading from both kernel and user
 * space (eventually)
 */
typedef struct MountPoint {
	ufsmount	*mnt_data;
	Chan		*chan;
	int		id;
	StatFs		mnt_stat;		/* cache of filesystem stats */
	int		mnt_maxsymlinklen;	/* max size of short symlink */
	int		mnt_iosize_max;		/* max size for clusters, etc */

	uint64_t	mnt_flag;	/* (i) flags shared with user */
	QLock		mnt_lock;	/* (mnt_mtx) structure lock */

	QLock		vnodes_lock;	/* lock on vnodes in use & freelist */
	vnode		*vnodes;	/* vnode cache */
	vnode		*free_vnodes;	/* vnode freelist */
} MountPoint;


typedef struct ComponentName {
	/*
	 * Arguments to lookup.
	 */
	unsigned long cn_nameiop;	/* namei operation */
	uint64_t cn_flags;		/* flags to namei */
	//struct	thread *cn_thread;/* thread requesting lookup */
	struct	Ucred *cn_cred;		/* credentials */
	int	cn_lkflags;		/* Lock flags LK_EXCLUSIVE or LK_SHARED */
	/*
	 * Shared between lookup and commit routines.
	 */
	char	*cn_pnbuf;		/* pathname buffer */
	char	*cn_nameptr;		/* pointer to looked up name */
	long	cn_namelen;		/* length of looked up component */
} ComponentName;


/* Harvey equivalent to FreeBSD vnode, but not exactly the same.  Acts as a
 * wrapper for the inode and any associated data.  This is not intended to be
 * support multiple filesystems and should probably be renamed after it works.
 * It may be unnecessary - if so, we should use inodes directly.
 */
typedef struct vnode {
	vnode		*next;
	vnode		*prev;

	Ref		ref;		/* Refcount */
	RWlock		vnlock;		/* u pointer to vnode lock */

	/*
	 * Fields which define the identity of the vnode.  These fields are
	 * owned by the filesystem (XXX: and vgone() ?)
	 */
	inode		*data;
	MountPoint	*mount;
	
	enum vtype 	type;		/* u vnode type */
	unsigned int	vflag;		/* v vnode flags */
} vnode;


#define	MAXUFSNAMLEN	255


typedef struct Dirent {
	uint32_t	fileno;			// file number of entry
	uint16_t	reclen;			// length of this record
	uint8_t		type; 			// file type, see below
	uint8_t		namlen;			// length of string in name
	char		name[MAXUFSNAMLEN + 1];	// name must be no longer than this
} Dirent;


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


/*
 * Flags to various vnode functions.
 */
#define	FORCECLOSE	0x0002	/* vflush: force file closure */

/*
 * namei operations
 */
#define	LOOKUP		0	/* perform name lookup only */
#define	CREATE		1	/* setup for file creation */
#define	DELETE		2	/* setup for file deletion */
#define	RENAME		3	/* setup for file renaming */
#define	OPMASK		3	/* mask for operation */

/*
 * Operations for lockmgr().
 */
#define	LK_EXCLUSIVE	0x080000
#define	LK_SHARED	0x200000
