/*-
 * Copyright (c) 1991, 1993
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
 *	@(#)queue.h	8.5 (Berkeley) 8/20/94
 * $FreeBSD$
 */

#define	TAILQ_HEAD(name, type)						\
struct name {								\
	struct type *tqh_first;	/* first element */			\
	struct type **tqh_last;	/* addr of last next element */		\
}

#define	TAILQ_ENTRY(type)						\
struct {								\
	struct type *tqe_next;	/* next element */			\
	struct type **tqe_prev;	/* address of previous next element */	\
}

#define	LIST_HEAD(name, type)						\
struct name {								\
	struct type *lh_first;	/* first element */			\
}

#define	LIST_ENTRY(type)						\
struct {								\
	struct type *le_next;	/* next element */			\
	struct type **le_prev;	/* address of previous next element */	\
}


typedef	int64_t daddr_t;	/* disk address */
typedef	uint32_t ino_t;		/* inode number */
typedef	uint32_t uid_t;		/* user id */
typedef	uint32_t gid_t;		/* group id */

typedef int64_t off_t;		/* File offset */
typedef int64_t ufs2_daddr_t;

typedef int64_t intmax_t;	/* FIXME: This should probably be moved to <u.h> or replaced with a spatch */


/*
 * User specifiable flags, stored in mnt_flag.
 */
#define	MNT_RDONLY	0x0000000000000001ULL /* read only filesystem */

/*
 * External filesystem command modifier flags.
 * Unmount can use the MNT_FORCE flag.
 * XXX: These are not STATES and really should be somewhere else.
 * XXX: MNT_BYFSID and MNT_NONBUSY collide with MNT_ACLS and MNT_MULTILABEL,
 *      but because MNT_ACLS and MNT_MULTILABEL are only used for mount(2),
 *      and MNT_BYFSID and MNT_NONBUSY are only used for unmount(2),
 *      it's harmless.
 */
#define	MNT_FORCE	0x0000000000080000ULL /* force unmount or readonly */


/*
 * Error codes used by UFS.
 * TODO HARVEY Translate to error strings.
 */
#define	EPERM		1		/* Operation not permitted */
#define	EINVAL		22		/* Invalid argument */
#define	ENOSPC		28		/* No space left on device */
