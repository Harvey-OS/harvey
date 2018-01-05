/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

typedef struct Usbfs Usbfs;
typedef struct Fid Fid;

enum
{
	Hdrsize	= 128,		/* plenty of room for headers */
	Msgsize	= 8216,		/* our preferred iounit (also devmnt's) */
	Bufsize	= Hdrsize + Msgsize,
	Namesz = 40,
	Errmax = 128,
	ONONE = ~0,		/* omode in Fid when not open */
};

struct Fid
{
	int	fid;
	Qid	qid;
	int	omode;
	Fid*	next;
	void*	aux;
};

struct Usbfs
{
	char	name[Namesz];
	uint64_t	qid;
	Dev*	dev;
	void*	aux;

	int	(*walk)(Usbfs *fs, Fid *f, char *name);
	void	(*clone)(Usbfs *fs, Fid *of, Fid *nf);
	void	(*clunk)(Usbfs *fs, Fid *f);
	int	(*open)(Usbfs *fs, Fid *f, int mode);
	int32_t	(*read)(Usbfs *fs, Fid *f, void *data, int32_t count, int64_t offset);
	int32_t	(*write)(Usbfs *fs, Fid*f, void *data, int32_t count, int64_t offset);
	int	(*stat)(Usbfs *fs, Qid q, Dir *d);
	void	(*end)(Usbfs *fs);
};

typedef int (*Dirgen)(Usbfs*, Qid, int, Dir*, void*);

int32_t	usbreadbuf(void *data, int32_t count, int64_t offset, void *buf, int32_t n);
void	usbfsadd(Usbfs *dfs);
void	usbfsdel(Usbfs *dfs);
int	usbdirread(Usbfs*f, Qid q, char *data, int32_t cnt, int64_t off, Dirgen gen, void *arg);
void	usbfsinit(char* srv, char *mnt, Usbfs *f, int flag);

void	usbfsdirdump(void);

extern char Enotfound[];
extern char Etoosmall[];
extern char Eio[];
extern char Eperm[];
extern char Ebadcall[];
extern char Ebadfid[];
extern char Einuse[];
extern char Eisopen[];
extern char Ebadctl[];

extern Usbfs usbdirfs;
extern int usbfsdebug;
