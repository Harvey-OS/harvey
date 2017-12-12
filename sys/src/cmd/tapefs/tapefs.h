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
#define	b8byte(x)	(((int64_t)b4byte(x)<<32) | (uint32_t)b4byte((x)+4))
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
	int16_t	busy;
	int16_t	open;
	int16_t	rclose;
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
	int32_t	perm;
	char	*name;
	uint32_t	atime;
	uint32_t	mtime;
	char	*user;
	char	*group;
	int64_t addr;
	void *data;
	int64_t	ndata;
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
	int64_t	addr;
	void	*data;
	int64_t	size;
	int	mode;
	int	uid;
	int	gid;
	int32_t	mdate;
} Fileinf;

extern	uint32_t	path;		/* incremented for each new file */
extern	Ram	*ram;
extern	char	*user;
extern	Idmap	*uidmap;
extern	Idmap	*gidmap;
extern	int	replete;
extern	int	blocksize;
void	error(char*);
void	*erealloc(void*, uint32_t);
void	*emalloc(uint32_t);
char	*estrdup(char*);
void	populate(char *);
void	dotrunc(Ram*);
void	docreate(Ram*);
char	*doread(Ram*, int64_t, int32_t);
void	dowrite(Ram*, char*, int32_t, int32_t);
int	dopermw(Ram*);
Idmap	*getpass(char*);
char	*mapid(Idmap*,int);
Ram	*poppath(Fileinf fi, int new);
Ram	*popfile(Ram *dir, Fileinf fi);
void	popdir(Ram*);
Ram	*lookup(Ram*, char*);
