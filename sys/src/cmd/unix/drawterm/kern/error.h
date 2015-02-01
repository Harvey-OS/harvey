/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

extern char Enoerror[];		/* no error */
extern char Emount[];		/* inconsistent mount */
extern char Eunmount[];		/* not mounted */
extern char Eunion[];		/* not in union */
extern char Emountrpc[];	/* mount rpc error */
extern char Eshutdown[];	/* device shut down */
extern char Enocreate[];	/* mounted directory forbids creation */
extern char Enonexist[];	/* file does not exist */
extern char Eexist[];		/* file already exists */
extern char Ebadsharp[];	/* unknown device in # filename */
extern char Enotdir[];		/* not a directory */
extern char Eisdir[];		/* file is a directory */
extern char Ebadchar[];		/* bad character in file name */
extern char Efilename[];	/* file name syntax */
extern char Eperm[];		/* permission denied */
extern char Ebadusefd[];	/* inappropriate use of fd */
extern char Ebadarg[];		/* bad arg in system call */
extern char Einuse[];		/* device or object already in use */
extern char Eio[];		/* i/o error */
extern char Etoobig[];		/* read or write too large */
extern char Etoosmall[];	/* read or write too small */
extern char Enoport[];		/* network port not available */
extern char Ehungup[];		/* i/o on hungup channel */
extern char Ebadctl[];		/* bad process or channel control request */
extern char Enodev[];		/* no free devices */
extern char Eprocdied[];	/* process exited */
extern char Enochild[];		/* no living children */
extern char Eioload[];		/* i/o error in demand load */
extern char Enovmem[];		/* virtual memory allocation failed */
extern char Ebadfd[];		/* fd out of range or not open */
extern char Enofd[];		/* no free file descriptors */
extern char Eisstream[];	/* seek on a stream */
extern char Ebadexec[];		/* exec header invalid */
extern char Etimedout[];	/* connection timed out */
extern char Econrefused[];	/* connection refused */
extern char Econinuse[];	/* connection in use */
extern char Eintr[];		/* interrupted */
extern char Enomem[];		/* kernel allocate failed */
extern char Enoswap[];		/* swap space full */
extern char Esoverlap[];	/* segments overlap */
extern char Emouseset[];	/* mouse type already set */
extern char Eshort[];		/* i/o count too small */
extern char Egreg[];		/* ken has left the building */
extern char Ebadspec[];		/* bad attach specifier */
extern char Enoreg[];		/* process has no saved registers */
extern char Enoattach[];	/* mount/attach disallowed */
extern char Eshortstat[];	/* stat buffer too small */
extern char Ebadstat[];		/* malformed stat buffer */
extern char Enegoff[];		/* negative i/o offset */
extern char Ecmdargs[];		/* wrong #args in control message */
