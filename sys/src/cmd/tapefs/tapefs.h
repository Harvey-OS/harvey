/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#define	g2byte(x)	(((x)[1]<<8) + (x)[0])		/* little-endian */
#define	g3byte(x)	(((x)[2]<<16) + ((x)[1]<<8) + (x)[0])
#define	g4byte(x)	(((x)[3]<<24) + ((x)[2]<<16) + ((x)[1]<<8) + (x)[0])

/* big endian */
#define	b4byte(x)	(((x)[0]<<24) + ((x)[1]<<16) + ((x)[2]<<8) + (x)[3])
#define	b8byte(x)	(((i64)b4byte(x)<<32) | (u32)b4byte((x)+4))
enum
{
	OPERM	= 0x3,		/* mask of all permission types in open mode */
	Nram	= 512,
	Maxbuf	= 8192,		/* max buffer size */
};

typedef struct Fid Fid;
typedef struct Ram Ram;

struct Fid
{
	i16	busy;
	i16	open;
	i16	rclose;
	int	fid;
	Fid	*next;
	char	*user;
	Ram	*ram;
};

struct Ram
{
	char	busy;
	char	open;
	char	replete;
	Ram	*parent;	/* parent directory */
	Ram	*child;		/* first member of directory */
	Ram	*next;		/* next member of file's directory */
	Qid	qid;
	i32	perm;
	char	*name;
	u32	atime;
	u32	mtime;
	char	*user;
	char	*group;
	i64 addr;
	void *data;
	i64	ndata;
};

enum
{
	Pexec =		1,
	Pwrite = 	2,
	Pread = 	4,
	Pother = 	1,
	Pgroup = 	8,
	Powner =	64,
};

typedef struct idmap {
	char	*name;
	int	id;
} Idmap;

typedef struct fileinf {
	char	*name;
	i64	addr;
	void	*data;
	i64	size;
	int	mode;
	int	uid;
	int	gid;
	i32	mdate;
} Fileinf;

extern	u32	path;		/* incremented for each new file */
extern	Ram	*ram;
extern	char	*user;
extern	Idmap	*uidmap;
extern	Idmap	*gidmap;
extern	int	replete;
extern	int	blocksize;
void	error(char*);
void	*erealloc(void*, u32);
void	*emalloc(u32);
char	*estrdup(char*);
void	populate(char *);
void	dotrunc(Ram*);
void	docreate(Ram*);
char	*doread(Ram*, i64, i32);
void	dowrite(Ram*, char*, i32, i32);
int	dopermw(Ram*);
Idmap	*getpass(char*);
char	*mapid(Idmap*,int);
Ram	*poppath(Fileinf fi, int new);
Ram	*popfile(Ram *dir, Fileinf fi);
void	popdir(Ram*);
Ram	*lookup(Ram*, char*);
