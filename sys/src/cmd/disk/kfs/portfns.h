/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

void	accessdir(Iobuf*, Dentry*, int);
void	authfree(File*);
void	addfree(Device, i32, Superb*);
i32	balloc(Device, int, i32);
void	bfree(Device, i32, int);
int	byname(void*, void*);
int	byuid(void*, void*);
int	checkname(char*);
int	checktag(Iobuf*, int, i32);
void 	cmd_user(void);
char*	cname(char*);
int	con_attach(int, char*, char*);
int	con_clone(int, int);
int	con_create(int, char*, int, int, i32, int);
int	con_open(int, int);
int	con_path(int, char*);
int	con_read(int, char*, i32, int);
int	con_remove(int);
int	con_stat(int, char*);
int	con_swap(int, int);
int	con_clri(int);
int	con_session(void);
int	con_walk(int, char*);
int	con_write(int, char*, i32, int);
int	con_wstat(int, char*);
void	cprint(char*, ...);
void	datestr(char*, i32);
void	dbufread(Iobuf*, Dentry*, i32);
Qid	dentryqid(Dentry*);
int	devcmp(Device, Device);
Iobuf*	dnodebuf(Iobuf*, Dentry*, i32, int);
Iobuf*	dnodebuf1(Iobuf*, Dentry*, i32, int);
void	dofilter(Filter*);
int	doremove(File *, int);
void	dtrunc(Iobuf*, Dentry*);
void	exit(void);
Float	famd(Float, int, int, int);
int	fchar(void);
u32	fdf(Float, int);
void	fileinit(Chan*);
void	sublockinit(void);
File*	filep(Chan*, int, int);
int	fname(char*);
void	formatinit(void);
void	freefp(File*);
void	freewp(Wpath*);
Filsys*	fsstr(char*);
Iobuf*	getbuf(Device, i32, int);
Dentry*	getdir(Iobuf*, int);
i32	getraddr(Device);
Wpath*	getwp(Wpath*);
void	hexdump(void*, int);
int	iaccess(File*, Dentry*, int);
i32	indfetch(Iobuf*, Dentry*, i32, i32 , int, int);
int	ingroup(int, int);
void	iobufinit(void);
int	leadgroup(int, int);
void	mkchallenge(Chan*);
void	mkqid(Qid*, Dentry*, int);
int	mkqidcmp(Qid*, Dentry*);
void	mkqid9p1(Qid9p1*, Qid*);
void	mkqid9p2(Qid*, Qid9p1*, int);
int	netserve(char*);
File*	newfp(Chan*);
Qid	newqid(Device);
void	newstart(void);
Wpath*	newwp(void);
int	oconvD2M(Dentry*, void*);
int	oconvM2D(void*, Dentry*);
int	ofcallfmt(Fmt*);
void	panic(char*, ...);
int	prime(i32);
void	putbuf(Iobuf*);
void	putwp(Wpath*);
i32	qidpathgen(Device*);
void	rootream(Device, i32);
void	settag(Iobuf*, int, i32);
void serve(Chan*);
void	serve9p1(Chan*, u8*, int);
void	serve9p2(Chan*, u8*, int);
void	strrand(void*, int);
int	strtouid(char*);
int	strtouid1(char*);
int	superok(Device, i32, int);
void	superream(Device, i32);
void	sync(char*);
int	syncblock(void);
int	Tfmt(Fmt*);
Tlock*	tlocked(Iobuf*, Dentry*);
void	uidtostr(char*,int);
void	uidtostr1(char*,int);



typedef struct Oldfcall Oldfcall;	/* needed for pragma */
