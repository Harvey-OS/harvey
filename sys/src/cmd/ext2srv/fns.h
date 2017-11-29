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
int32_t	writefile(Xfile*, void*, int64_t, int32_t);
char *	xerrstr(int);
Xfile *	xfile(Fid*, int);
int	xfspurge(void);

int ext2fs(Xfs *);
int get_inode( Xfile *, uint);
char *getname(Xfile *, char *);
int get_file(Xfile *, char *);
int bmap( Xfile *f, int block );
int ffz(int);
int32_t	readdir(Xfile*, void*, int64_t, int32_t);
int32_t	readfile(Xfile*, void*, int64_t, int32_t);
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
void free_block( Xfs *, uint32_t);
void free_inode( Xfs *, int);
int empty_dir(Xfile *);
int truncfile(Xfile *);
int dowstat(Xfile *, Dir *);
int32_t getmode(Xfile *);
Ext2 getext2(Xfs *, char, int);
void CleanSuper(Xfs *);

/* Iobuf operations */

Iobuf *getbuf(Xfs *, int32_t addr);
void putbuf(Iobuf *);
void purgebuf(Xfs *);
void iobuf_init(void);
int xread(Xfs *, Iobuf *, int32_t);
void syncbuf(void);
void xwrite(Iobuf *);
void dirtybuf(Iobuf *);

void mchat(char *fmt, ...);
void dumpbuf(void);

void gidfile(char*);
void uidfile(char*);
