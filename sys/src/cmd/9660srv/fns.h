/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

void	chat(char*, ...);
void*	ealloc(long);
void	error(char*);
Iobuf*	getbuf(Xdata*, uvlong);
Xdata*	getxdata(char*);
void	iobuf_init(void);
void	nexterror(void);
void	panic(int, char*, ...);
void	purgebuf(Xdata*);
void	putbuf(Iobuf*);
void	refxfs(Xfs*, int);
void	showdir(int, Dir*);
Xfile*	xfile(int, int);
void setnames(Dir*, char*);

#define	waserror()	(++nerr_lab, setjmp(err_lab[nerr_lab-1]))
#define	poperror()	(--nerr_lab)
