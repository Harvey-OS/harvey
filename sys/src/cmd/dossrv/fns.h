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
#pragma	varargck	argpos	chat	1
int	cistrcmp(const char*, const char*);
int	classifyname(char*);
Xfile	*clean(Xfile*);
int32_t	clust2sect(Dosbpb*, int32_t);
void	dirdump(void*);
int	dosfs(Xfs*);
void	dosptrreloc(Xfile *f, Dosptr *dp, uint32_t addr, uint32_t offset);
int	emptydir(Xfile*);
int	eqqid(Qid, Qid);
int	falloc(Xfs*);
void	ffree(Xfs *xf, int32_t start);
int32_t	fileaddr(Xfile*, int32_t, int);
void	fixname(char*);
void	getdir(Xfs*, Dir*, Dosdir*, int, int);
int32_t	getfat(Xfs*, int);
int	getfile(Xfile*);
void	getname(char*, Dosdir*);
char	*getnamesect(char*, char*, unsigned char*, int*, int*, int);
int32_t	getstart(Xfs *xf, Dosdir *d);
Xfs	*getxfs(char*, char*);
int32_t	gtime(Dosdir *d);
void	io(int srvfd);
int	iscontig(Xfs *xf, Dosdir *d);
int	isroot(uint32_t addr);
int	makecontig(Xfile*, int);
void	mkalias(char*, char*, int);
int	nameok(char*);
void	panic(char*, ...);
#pragma	varargck	argpos	panic	1
void	putdir(Dosdir*, Dir*);
void	putfat(Xfs*, int, uint32_t);
void	putfile(Xfile*);
int	putint32_tname(Xfs *f, Dosptr *ndp, char *name, char sname[13]);
void	putname(char*, Dosdir*);
void	putstart(Xfs *xf, Dosdir *d, int32_t start);
void	puttime(Dosdir*, int32_t);
void	rattach(void);
void	rauth(void);
void	rclone(void);
void	rclunk(void);
void	rcreate(void);
int32_t	readdir(Xfile*, void*, int32_t, int32_t);
int32_t	readfile(Xfile*, void*, int32_t, int32_t);
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
int32_t	sect2clust(Dosbpb*, int32_t);
int	truncfile(Xfile*, int32_t length);
int	utftorunes(Rune*, char*, int);
int	walkup(Xfile*, Dosptr*);
int32_t	writefile(Xfile*, void*, int32_t, int32_t);
char	*xerrstr(int);
Xfile	*xfile(int, int);
int	xfspurge(void);
int putlongname(Xfs *xf, Dosptr *ndp, char *name, char sname[13]);
