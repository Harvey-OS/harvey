/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

typedef struct VacFs VacFs;
typedef struct VacDir VacDir;
typedef struct VacFile VacFile;
typedef struct VacDirEnum VacDirEnum;

//#pragma incomplete VacFile
//#pragma incomplete VacDirEnum

/*
 * Mode bits
 */
enum
{
	ModeOtherExec = (1<<0),
	ModeOtherWrite = (1<<1),
	ModeOtherRead = (1<<2),
	ModeGroupExec = (1<<3),
	ModeGroupWrite = (1<<4),
	ModeGroupRead = (1<<5),
	ModeOwnerExec = (1<<6),
	ModeOwnerWrite = (1<<7),
	ModeOwnerRead = (1<<8),
	ModeSticky = (1<<9),
	ModeSetUid = (1<<10),
	ModeSetGid = (1<<11),
	ModeAppend = (1<<12),		/* append only file */
	ModeExclusive = (1<<13),	/* lock file - plan 9 */
	ModeLink = (1<<14),		/* sym link */
	ModeDir	= (1<<15),		/* duplicate of DirEntry */
	ModeHidden = (1<<16),		/* MS-DOS */
	ModeSystem = (1<<17),		/* MS-DOS */
	ModeArchive = (1<<18),		/* MS-DOS */
	ModeTemporary = (1<<19),	/* MS-DOS */
	ModeSnapshot = (1<<20),		/* read only snapshot */
	ModeDevice = (1<<21),		/* Unix device */
	ModeNamedPipe = (1<<22)	/* Unix named pipe */
};

enum
{
	MetaMagic = 0x5656fc79,
	MetaHeaderSize = 12,
	MetaIndexSize = 4,
	IndexEntrySize = 8,
	DirMagic = 0x1c4d9072
};

enum
{
	DirPlan9Entry = 1,	/* not valid in version >= 9 */
	DirNTEntry,		/* not valid in version >= 9 */
	DirQidSpaceEntry,
	DirGenEntry		/* not valid in version >= 9 */
};

struct VacDir
{
	char *elem;		/* path element */
	u32 entry;		/* entry in directory for data */
	u32 gen;		/* generation of data entry */
	u32 mentry;		/* entry in directory for meta */
	u32 mgen;		/* generation of meta entry */
	u64 size;		/* size of file */
	u64 qid;		/* unique file id */

	char *uid;		/* owner id */
	char *gid;		/* group id */
	char *mid;		/* last modified by */
	u32 mtime;		/* last modified time */
	u32 mcount;		/* number of modifications: can wrap! */
	u32 ctime;		/* directory entry last changed */
	u32 atime;		/* last time accessed */
	u32 mode;		/* various mode bits */

	/* plan 9 */
	int plan9;
	u64 p9path;
	u32 p9version;

	/* sub space of qid */
	int qidspace;
	u64 qidoffset;	/* qid offset */
	u64 qidmax;		/* qid maximum */
};

struct VacFs
{
	char	name[128];
	u8	score[VtScoreSize];
	VacFile	*root;
	VtConn	*z;
	int		mode;
	int		bsize;
	u64	qid;
	VtCache	*cache;
};

VacFs	*vacfsopen(VtConn *z, char *file, int mode, int ncache);
VacFs	*vacfsopenscore(VtConn *z, u8 *score, int mode, int ncache);
VacFs	*vacfscreate(VtConn *z, int bsize, int ncache);
void		vacfsclose(VacFs *fs);
int		vacfssync(VacFs *fs);
int		vacfssnapshot(VacFs *fs, char *src, char *dst);
int		vacfsgetscore(VacFs *fs, u8 *score);
int		vacfsgetmaxqid(VacFs*, u64*);
void		vacfsjumpqid(VacFs*, u64);

VacFile *vacfsgetroot(VacFs *fs);
VacFile	*vacfileopen(VacFs *fs, char *path);
VacFile	*vacfilecreate(VacFile *file, char *elem, u32 perm);
VacFile	*vacfilewalk(VacFile *file, char *elem);
int		vacfileremove(VacFile *file);
int		vacfileread(VacFile *file, void *buf, int n, i64 offset);
int		vacfileblockscore(VacFile *file, u32, u8*);
int		vacfilewrite(VacFile *file, void *buf, int n, i64 offset);
u64	vacfilegetid(VacFile *file);
u32	vacfilegetmcount(VacFile *file);
int		vacfileisdir(VacFile *file);
int		vacfileisroot(VacFile *file);
u32	vacfilegetmode(VacFile *file);
int		vacfilegetsize(VacFile *file, u64 *size);
int		vacfilegetdir(VacFile *file, VacDir *dir);
int		vacfilesetdir(VacFile *file, VacDir *dir);
VacFile	*vacfilegetparent(VacFile *file);
int		vacfileflush(VacFile*, int);
VacFile	*vacfileincref(VacFile*);
int		vacfiledecref(VacFile*);
int		vacfilesetsize(VacFile *f, u64 size);

int		vacfilegetentries(VacFile *f, VtEntry *e, VtEntry *me);
int		vacfilesetentries(VacFile *f, VtEntry *e, VtEntry *me);

void		vdcleanup(VacDir *dir);
void		vdcopy(VacDir *dst, VacDir *src);
int		vacfilesetqidspace(VacFile*, u64, u64);
u64	vacfilegetqidoffset(VacFile*);

VacDirEnum	*vdeopen(VacFile*);
int			vderead(VacDirEnum*, VacDir *);
void			vdeclose(VacDirEnum*);
int	vdeunread(VacDirEnum*);

int	vacfiledsize(VacFile *f);
int	sha1matches(VacFile *f, u32 b, u8 *buf, int n);
