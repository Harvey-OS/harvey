/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

void	chat(char*, ...);
Xfile *	clean(Xfile*);
void	dirdump(void*);
int	dosfs(Xfs*);
int	emptydir(Xfile*);
int	falloc(Xfs*);
int	fileaddr(Xfile*, int, int);
int	getfat(Xfs*, int);
int	getfile(Xfile*);
Xfs *	getxfs(char*);
void	panic(char*, ...);
void	putfat(Xfs*, int, int);
void	putfile(Xfile*);
void	refxfs(Xfs*, int);
i32	writefile(Xfile*, void*, i64, i32);
char *	xerrstr(int);
Xfile *	xfile(Fid*, int);
int	xfspurge(void);

int ext2fs(Xfs *);
int get_inode( Xfile *, u32);
char *getname(Xfile *, char *);
int get_file(Xfile *, char *);
int bmap( Xfile *f, int block );
int ffz(int);
i32	readdir(Xfile*, void*, i64, i32);
i32	readfile(Xfile*, void*, i64, i32);
void dostat(Qid, Xfile *, Dir *);
int new_block( Xfile *, int);
int test_bit(int, void *);
int set_bit(int, void *);
int  clear_bit(int , void *);
void *memscan(void *, int, int);
int find_first_zero_bit(void *, int);
int find_next_zero_bit(void *, int, int);
int block_getblk(Xfile *, int, int);
int inode_getblk(Xfile *, int);
int getblk(Xfile *, int);
int  new_inode(Xfile *, int);
int add_entry(Xfile *, char *, int);
int create_file(Xfile *, char *, int);
int create_dir(Xfile *, char *, int);
int unlink(Xfile *);
int  delete_entry(Xfs *, Inode *, int);
int  free_block_inode(Xfile *);
void free_block( Xfs *, u32);
void free_inode( Xfs *, int);
int empty_dir(Xfile *);
int truncfile(Xfile *);
int dowstat(Xfile *, Dir *);
i32 getmode(Xfile *);
Ext2 getext2(Xfs *, char, int);
void CleanSuper(Xfs *);

/* Iobuf operations */

Iobuf *getbuf(Xfs *, i32 addr);
void putbuf(Iobuf *);
void purgebuf(Xfs *);
void iobuf_init(void);
int xread(Xfs *, Iobuf *, i32);
void syncbuf(void);
void xwrite(Iobuf *);
void dirtybuf(Iobuf *);

void mchat(char *fmt, ...);
void dumpbuf(void);

void gidfile(char*);
void uidfile(char*);
