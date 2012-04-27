/*
 * Copyright (c) 1989, 1993, 1995
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Rick Macklem at The University of Guelph.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
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
 *	@(#)nfs.h	8.4 (Berkeley) 5/1/95
 * $Id: nfs.h,v 1.44 1998/09/07 05:42:15 bde Exp $
 */

#ifndef _9FS_H_
#define _9FS_H_

#ifdef KERNEL
#include "opt_u9fs.h"
#endif

#define U9FS_FABLKSIZE   512
#define U9FS_PORT        17008

/*
 * U9FS mount option flags
 */
#define	U9FSMNT_SOFT		0x00000001  /* soft mount (hard is default) */
#define	U9FSMNT_MAXGRPS		0x00000020  /* set maximum grouplist size */
#define	U9FSMNT_INT		0x00000040  /* allow interrupts on hard mount */
#define	U9FSMNT_KERB		0x00000400  /* Use Kerberos authentication */
#define	U9FSMNT_READAHEAD	0x00002000  /* set read ahead */

struct p9user {
  uid_t p9_uid;
  char p9_name[U9FS_NAMELEN];
};

/*
 * Arguments to mount 9FS
 */
#define U9FS_ARGSVERSION	1	/* change when nfs_args changes */
struct u9fs_args {
	int		version;	/* args structure version number */
	struct sockaddr	*addr;		/* file server address */
	int		addrlen;	/* length of address */
	int		sotype;		/* Socket type */
	int		proto;		/* and Protocol */
	int		fhsize;		/* Size, in bytes, of fh */
	int		flags;		/* flags */
	int		wsize;		/* write size in bytes */
	int		rsize;		/* read size in bytes */
	int		readdirsize;	/* readdir size in bytes */
	char		*hostname;	/* server's name */
        struct sockaddr * authaddr;
        int             authaddrlen;
        int             authsotype;
        int             authsoproto;
        int             nusers;
        char            uname[U9FS_NAMELEN];
        char            key[U9AUTH_DESKEYLEN];
        struct p9user * users;
};

/*
 * The u9fsnode is the u9fs equivalent to ufs's inode. Any similarity
 * is purely coincidental.
 * There is a unique u9fsnode allocated for each active file,
 * each current directory, each mounted-on file, text file, and the root.
 * An u9fsnode is 'named' by its file handle. (nget/u9fs_node.c)
 * If this structure exceeds 256 bytes (it is currently 256 using 4.4BSD-Lite
 * type definitions), file handles of > 32 bytes should probably be split out
 * into a separate MALLOC()'d data structure. (Reduce the size of u9fsfh_t by
 * changing the definition in u9fsproto.h of U9FS_SMALLFH.)
 * NB: Hopefully the current order of the fields is such that everything will
 *     be well aligned and, therefore, tightly packed.
 */
struct u9fsnode {
	LIST_ENTRY(u9fsnode)	n_hash;		/* Hash chain */
	u_quad_t		n_size;		/* Current size of file */
	u_quad_t		n_lrev;		/* Modify rev for lease */
	struct vattr		n_vattr;	/* Vnode attribute cache */
	time_t			n_attrstamp;	/* Attr. cache timestamp */
	u_int32_t		n_mode;		/* ACCESS mode cache */
	uid_t			n_modeuid;	/* credentials having mode */
	time_t			n_modestamp;	/* mode cache timestamp */
	time_t			n_mtime;	/* Prev modify time. */
	time_t			n_ctime;	/* Prev create time. */
	u_short			*n_fid;		/* U9FS FID */
	struct vnode		*n_vnode;	/* associated vnode */
	struct lockf		*n_lockf;	/* Locking record of file */
	int			n_error;	/* Save write error value */
#if 0
	union {
		struct timespec	nf_atim;	/* Special file times */
		u9fsuint64	nd_cookieverf;	/* Cookie verifier (dir only) */
	} n_un1;
	union {
		struct timespec	nf_mtim;
		off_t		nd_direof;	/* Dir. EOF offset cache */
	} n_un2;
	union {
		struct sillyrename *nf_silly;	/* Ptr to silly rename struct */
		LIST_HEAD(, u9fsdmap) nd_cook;	/* cookies */
	} n_un3;
#endif
	short			n_flag;		/* Flag for locking.. */
};

#define n_atim		n_un1.nf_atim
#define n_mtim		n_un2.nf_mtim
#define n_sillyrename	n_un3.nf_silly
#define n_cookieverf	n_un1.nd_cookieverf
#define n_direofoffset	n_un2.nd_direof
#define n_cookies	n_un3.nd_cook

/*
 * Flags for n_flag
 */
#define	NFLUSHWANT	0x0001	/* Want wakeup from a flush in prog. */
#define	NFLUSHINPROG	0x0002	/* Avoid multiple calls to vinvalbuf() */
#define	NMODIFIED	0x0004	/* Might have a modified buffer in bio */
#define	NWRITEERR	0x0008	/* Flag write errors so close will know */
#define	NQU9FSNONCACHE	0x0020	/* Non-cachable lease */
#define	NQU9FSWRITE	0x0040	/* Write lease */
#define	NQU9FSEVICTED	0x0080	/* Has been evicted */
#define	NACC		0x0100	/* Special file accessed */
#define	NUPD		0x0200	/* Special file updated */
#define	NCHG		0x0400	/* Special file times changed */
#define NLOCKED		0x0800  /* node is locked */
#define NWANTED		0x0100  /* someone wants to lock */

/*
 * Convert between u9fsnode pointers and vnode pointers
 */
#define VTOU9FS(vp)	((struct u9fsnode *)(vp)->v_data)
#define U9FSTOV(np)	((struct vnode *)(np)->n_vnode)

/*
 * Mount structure.
 * One allocated on every U9FS mount.
 * Holds U9FS specific information for mount.
 */
struct	u9fsmount {
	int	nm_flag;		/* Flags for soft/hard... */
	int	nm_state;		/* Internal state flags */
	struct	mount *nm_mountp;	/* Vfs structure for this filesystem */
	int	nm_numgrps;		/* Max. size of groupslist */
	u_short	nm_fid;	                /* fid of root dir */
	struct	socket *nm_so;		/* Rpc socket */
	int	nm_sotype;		/* Type of socket */
	int	nm_soproto;		/* and protocol */
	int	nm_soflags;		/* pr_flags for socket protocol */
	struct	sockaddr *nm_nam;	/* Addr of server */
	int	nm_sent;		/* Request send count */
	int	nm_cwnd;		/* Request send window */
	int	nm_rsize;		/* Max size of read rpc */
	int	nm_wsize;		/* Max size of write rpc */
	int	nm_readdirsize;		/* Size of a readdir rpc */
#if 0
	struct vnode *nm_inprog;	/* Vnode in prog by nqu9fs_clientd() */
	uid_t	nm_authuid;		/* Uid for authenticator */
	int	nm_authtype;		/* Authenticator type */
	int	nm_authlen;		/* and length */
	char	*nm_authstr;		/* Authenticator string */
	char	*nm_verfstr;		/* and the verifier */
	int	nm_verflen;
	u_char	nm_verf[U9FSX_V3WRITEVERF]; /* V3 write verifier */
	U9FSKERBKEY_T nm_key;		/* and the session key */
	int	nm_numuids;		/* Number of u9fsuid mappings */
	TAILQ_HEAD(, u9fsuid) nm_uidlruhead; /* Lists of u9fsuid mappings */
	LIST_HEAD(, u9fsuid) nm_uidhashtbl[U9FS_MUIDHASHSIZ];
	TAILQ_HEAD(, buf) nm_bufq;	/* async io buffer queue */
	short	nm_bufqlen;		/* number of buffers in queue */
	short	nm_bufqwant;		/* process wants to add to the queue */
	int	nm_bufqiods;		/* number of iods processing queue */
#endif
	u_int64_t nm_maxfilesize;	/* maximum file size */
};

#ifdef KERNEL

#ifdef MALLOC_DECLARE
MALLOC_DECLARE(M_U9FSHASH);
#endif

/*
 * Convert mount ptr to u9fsmount ptr.
 */
#define VFSTOU9FS(mp)	((struct u9fsmount *)((mp)->mnt_data))

#endif	/* KERNEL */

#endif
