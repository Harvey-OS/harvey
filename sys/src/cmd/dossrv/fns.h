/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

int	aliassum(Dosdir*);
void	bootdump32(int, Dosboot32*);
void	bootdump(int, Dosboot*);
void	bootsecdump32(int fd, Xfs *xf, Dosboot32 *b32);
int	cfalloc(Xfile*);
void	chat(char*, ...);
//#pragma	varargck	argpos	chat	1
int	cistrcmp(const char*, const char*);
int	classifyname(char*);
Xfile	*clean(Xfile*);
i32	clust2sect(Dosbpb*, i32);
void	dirdump(void*);
int	dosfs(Xfs*);
void	dosptrreloc(Xfile *f, Dosptr *dp, u32 addr, u32 offset);
int	emptydir(Xfile*);
int	eqqid(Qid, Qid);
int	falloc(Xfs*);
void	ffree(Xfs *xf, i32 start);
i32	fileaddr(Xfile*, i32, int);
void	fixname(char*);
void	getdir(Xfs*, Dir*, Dosdir*, int, int);
i32	getfat(Xfs*, int);
int	getfile(Xfile*);
void	getname(char*, Dosdir*);
char	*getnamesect(char*, char*, unsigned char*, int*, int*, int);
i32	getstart(Xfs *xf, Dosdir *d);
Xfs	*getxfs(char*, char*);
i32	gtime(Dosdir *d);
void	io(int srvfd);
int	iscontig(Xfs *xf, Dosdir *d);
int	isroot(u32 addr);
int	makecontig(Xfile*, int);
void	mkalias(char*, char*, int);
int	nameok(char*);
void	panic(char*, ...);
//#pragma	varargck	argpos	panic	1
void	putdir(Dosdir*, Dir*);
void	putfat(Xfs*, int, u32);
void	putfile(Xfile*);
int	puti32name(Xfs *f, Dosptr *ndp, char *name, char sname[13]);
void	putname(char*, Dosdir*);
void	putstart(Xfs *xf, Dosdir *d, i32 start);
void	puttime(Dosdir*, i32);
void	rattach(void);
void	rauth(void);
void	rclone(void);
void	rclunk(void);
void	rcreate(void);
i32	readdir(Xfile*, void*, i32, i32);
i32	readfile(Xfile*, void*, i32, i32);
void	refxfs(Xfs*, int);
void	rflush(void);
void	rootfile(Xfile*);
void	ropen(void);
void	rread(void);
void	rremove(void);
void	rstat(void);
void	rwalk(void);
void	rwrite(void);
void	rwstat(void);
void	rversion(void);
int	searchdir(Xfile*, char*, Dosptr*, int, int);
i32	sect2clust(Dosbpb*, i32);
int	truncfile(Xfile*, i32 length);
int	utftorunes(Rune*, char*, int);
int	walkup(Xfile*, Dosptr*);
i32	writefile(Xfile*, void*, i32, i32);
char	*xerrstr(int);
Xfile	*xfile(int, int);
int	xfspurge(void);

/* dossubs.c */
int putlongname(Xfs *xf, Dosptr *ndp, char *name, char sname[13]);
