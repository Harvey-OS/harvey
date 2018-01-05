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
void	addfree(Device, int32_t, Superb*);
int32_t	balloc(Device, int, int32_t);
void	bfree(Device, int32_t, int);
int	byname(void*, void*);
int	byuid(void*, void*);
int	checkname(char*);
int	checktag(Iobuf*, int, int32_t);
void 	cmd_user(void);
char*	cname(char*);
int	con_attach(int, char*, char*);
int	con_clone(int, int);
int	con_create(int, char*, int, int, int32_t, int);
int	con_open(int, int);
int	con_path(int, char*);
int	con_read(int, char*, int32_t, int);
int	con_remove(int);
int	con_stat(int, char*);
int	con_swap(int, int);
int	con_clri(int);
int	con_session(void);
int	con_walk(int, char*);
int	con_write(int, char*, int32_t, int);
int	con_wstat(int, char*);
void	cprint(char*, ...);
void	datestr(char*, int32_t);
void	dbufread(Iobuf*, Dentry*, int32_t);
Qid	dentryqid(Dentry*);
int	devcmp(Device, Device);
Iobuf*	dnodebuf(Iobuf*, Dentry*, int32_t, int);
Iobuf*	dnodebuf1(Iobuf*, Dentry*, int32_t, int);
void	dofilter(Filter*);
int	doremove(File *, int);
void	dtrunc(Iobuf*, Dentry*);
void	exit(void);
Float	famd(Float, int, int, int);
int	fchar(void);
uint32_t	fdf(Float, int);
void	fileinit(Chan*);
void	sublockinit(void);
File*	filep(Chan*, int, int);
int	fname(char*);
void	formatinit(void);
void	freefp(File*);
void	freewp(Wpath*);
Filsys*	fsstr(char*);
Iobuf*	getbuf(Device, int32_t, int);
Dentry*	getdir(Iobuf*, int);
int32_t	getraddr(Device);
Wpath*	getwp(Wpath*);
void	hexdump(void*, int);
int	iaccess(File*, Dentry*, int);
int32_t	indfetch(Iobuf*, Dentry*, int32_t, int32_t , int, int);
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
int	prime(int32_t);
void	putbuf(Iobuf*);
void	putwp(Wpath*);
int32_t	qidpathgen(Device*);
void	rootream(Device, int32_t);
void	settag(Iobuf*, int, int32_t);
void serve(Chan*);
void	serve9p1(Chan*, uint8_t*, int);
void	serve9p2(Chan*, uint8_t*, int);
void	strrand(void*, int);
int	strtouid(char*);
int	strtouid1(char*);
int	superok(Device, int32_t, int);
void	superream(Device, int32_t);
void	sync(char*);
int	syncblock(void);
int	Tfmt(Fmt*);
Tlock*	tlocked(Iobuf*, Dentry*);
void	uidtostr(char*,int);
void	uidtostr1(char*,int);



typedef struct Oldfcall Oldfcall;	/* needed for pragma */
