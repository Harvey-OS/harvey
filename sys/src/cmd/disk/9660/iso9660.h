/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

/*
 * iso9660.h
 *
 * Routines and data structures to support reading and writing ISO 9660 CD images.
 * See the ISO 9660 or ECMA 119 standards.
 *
 * Also supports Rock Ridge extensions for int32_t file names and Unix stuff.
 * Also supports Microsoft's Joliet extensions for Unicode and int32_t file names.
 * Also supports El Torito bootable CD spec.
 */

typedef struct Cdimg Cdimg;
typedef struct Cdinfo Cdinfo;
typedef struct Conform Conform;
typedef struct Direc Direc;
typedef struct Dumproot Dumproot;
typedef struct Voldesc Voldesc;
typedef struct XDir XDir;

#ifndef CHLINK
#define CHLINK 0
#endif

struct XDir {
	char	*name;
	char	*uid;
	char	*gid;
	char	*symlink;
	uint32_t   uidno;   /* Numeric uid */
	uint32_t   gidno;   /* Numeric gid */

	uint32_t	mode;
	uint32_t	atime;
	uint32_t	mtime;
	uint32_t   ctime;

        int64_t   length;
};

/*
 * A directory entry in a ISO9660 tree.
 * The extra data (uid, etc.) here is put into the system use areas.
 */
struct Direc {
	char *name;	/* real name */
	char *confname;	/* conformant name */
	char *srcfile;	/* file to copy onto the image */

	uint32_t block;
	uint32_t length;
	int flags;

	char *uid;
	char *gid;
	char *symlink;
	uint32_t mode;
	int32_t atime;
	int32_t ctime;
	int32_t mtime;

	uint32_t uidno;
	uint32_t gidno;

	Direc *child;
	int nchild;
};
enum {  /* Direc flags */
	Dbadname = 1<<0,  /* Non-conformant name     */
};

/*
 * Data found in a volume descriptor.
 */
struct Voldesc {
	char *systemid;
	char *volumeset;
	char *publisher;
	char *preparer;
	char *application;

	/* file names for various parameters */
	char *abstract;
	char *biblio;
	char *notice;

	/* path table */
	uint32_t pathsize;
	uint32_t lpathloc;
	uint32_t mpathloc;

	/* root of file tree */
	Direc root;	
};

/*
 * An ISO9660 CD image.  Various parameters are kept in memory but the
 * real image file is opened for reading and writing on fd.
 *
 * The bio buffers brd and bwr moderate reading and writing to the image.
 * The routines we use are careful to flush one before or after using the other,
 * as necessary.
 */
struct Cdimg {
	char *file;
	int fd;
	uint32_t dumpblock;
	uint32_t nextblock;
	uint32_t iso9660pvd;
	uint32_t jolietsvd;
	uint32_t pathblock;
	uint64_t rrcontin;	/* rock ridge continuation offset */
	uint32_t nulldump;		/* next dump block */
	uint32_t nconform;		/* number of conform entries written already */
	uint64_t bootcatptr;
	uint32_t bootcatblock;
	uint64_t bootimageptr;
	Direc *loaderdirec;
	Direc *bootdirec;
	char *bootimage;
	char *loader;
	
	Biobuf brd;
	Biobuf bwr;

	int flags;

	Voldesc iso;
	Voldesc joliet;
};
enum {	/* Cdimg->flags, Cdinfo->flags */
	CDjoliet = 1<<0,
	CDplan9 = 1<<1,
	CDconform = 1<<2,
	CDrockridge = 1<<3,
	CDnew = 1<<4,
	CDdump = 1<<5,
	CDbootable = 1<<6,
	CDbootnoemu = 1<<7,
	CDpbs= 1<<8,
};

typedef struct Tx Tx;
struct Tx {
	char *bad;	/* atoms */
	char *good;
};

struct Conform {
	Tx *t;
	int nt;	/* delta = 32 */
};

struct Cdinfo {
	int flags;

	char *volumename;

	char *volumeset;
	char *publisher;
	char *preparer;
	char *application;
	char *bootimage;
	char *loader;
};

/*
 * This is a doubly binary tree.
 * We have a tree keyed on the MD5 values
 * as well as a tree keyed on the block numbers.
 */
typedef struct Dump Dump;
typedef struct Dumpdir Dumpdir;

struct Dump {
	Cdimg *cd;
	Dumpdir *md5root;
	Dumpdir *blockroot;
};

struct Dumpdir {
	char *name;
	unsigned char md5[MD5dlen];
	uint32_t block;
	uint32_t length;
	Dumpdir *md5left;
	Dumpdir *md5right;
	Dumpdir *blockleft;
	Dumpdir *blockright;
};

struct Dumproot {
	char *name;
	int nkid;
	Dumproot *kid;
	Direc root;
	Direc jroot;
};

/*
 * ISO9660 on-CD structures.
 */
typedef struct Cdir Cdir;
typedef struct Cpath Cpath;
typedef struct Cvoldesc Cvoldesc;

/* a volume descriptor block */
struct Cvoldesc {
	unsigned char	magic[8];	/* 0x01, "CD001", 0x01, 0x00 */
	unsigned char	systemid[32];	/* system identifier */
	unsigned char	volumeid[32];	/* volume identifier */
	unsigned char	unused[8];	/* character set in secondary desc */
	unsigned char	volsize[8];	/* volume size */
	unsigned char	charset[32];
	unsigned char	volsetsize[4];	/* volume set size = 1 */
	unsigned char	volseqnum[4];	/* volume sequence number = 1 */
	unsigned char	blocksize[4];	/* logical block size */
	unsigned char	pathsize[8];	/* path table size */
	unsigned char	lpathloc[4];	/* Lpath */
	unsigned char	olpathloc[4];	/* optional Lpath */
	unsigned char	mpathloc[4];	/* Mpath */
	unsigned char	ompathloc[4];	/* optional Mpath */
	unsigned char	rootdir[34];	/* directory entry for root */
	unsigned char	volumeset[128];	/* volume set identifier */
	unsigned char	publisher[128];
	unsigned char	preparer[128];	/* data preparer identifier */
	unsigned char	application[128];	/* application identifier */
	unsigned char	notice[37];	/* copyright notice file */
	unsigned char	abstract[37];	/* abstract file */
	unsigned char	biblio[37];	/* bibliographic file */
	unsigned char	cdate[17];	/* creation date */
	unsigned char	mdate[17];	/* modification date */
	unsigned char	xdate[17];	/* expiration date */
	unsigned char	edate[17];	/* effective date */
	unsigned char	fsvers;		/* file system version = 1 */
};

/* a directory entry */
struct Cdir {
	unsigned char	len;
	unsigned char	xlen;
	unsigned char	dloc[8];
	unsigned char	dlen[8];
	unsigned char	date[7];
	unsigned char	flags;
	unsigned char	unitsize;
	unsigned char	gapsize;
	unsigned char	volseqnum[4];
	unsigned char	namelen;
	unsigned char	name[1];	/* chumminess */
};

/* a path table entry */
struct Cpath {
	unsigned char   namelen;
	unsigned char   xlen;
	unsigned char   dloc[4];
	unsigned char   parent[2];
	unsigned char   name[1];        /* chumminess */
};

enum { /* Rockridge flags */
	RR_PX = 1<<0,
	RR_PN = 1<<1,
	RR_SL = 1<<2,
	RR_NM = 1<<3,
	RR_CL = 1<<4,
	RR_PL = 1<<5,
	RR_RE = 1<<6,
	RR_TF = 1<<7,
};

enum { /* CputrripTF type argument */
	TFcreation = 1<<0,
	TFmodify = 1<<1,
	TFaccess = 1<<2,
	TFattributes = 1<<3,
	TFbackup = 1<<4,
	TFexpiration = 1<<5,
	TFeffective = 1<<6,
	TFlongform = 1<<7,
};

enum { /* CputrripNM flag types */
	NMcontinue = 1<<0,
	NMcurrent = 1<<1,
	NMparent = 1<<2,
	NMroot = 1<<3,
	NMvolroot = 1<<4,
	NMhost = 1<<5,
};

/* boot.c */
void Cputbootvol(Cdimg*);
void Cputbootcat(Cdimg*);
void Cupdatebootvol(Cdimg*);
void Cupdatebootcat(Cdimg*);
void Cfillpbs(Cdimg*);
void findbootimage(Cdimg*, Direc*);
void findloader(Cdimg*, Direc*);

/* cdrdwr.c */
Cdimg *createcd(char*, Cdinfo);
Cdimg *opencd(char*, Cdinfo);
void Creadblock(Cdimg*, void*, uint32_t, uint32_t);
uint32_t big(void*, int);
uint32_t little(void*, int);
int parsedir(Cdimg*, Direc*, unsigned char*, int, char *(*)(unsigned char*, int));
void setroot(Cdimg*, uint32_t, uint32_t, uint32_t);
void setvolsize(Cdimg*, uint64_t, uint32_t);
void setpathtable(Cdimg*, uint32_t, uint32_t, uint32_t, uint32_t);
void Cputc(Cdimg*, int);
void Cputnl(Cdimg*, uint64_t, int);
void Cputnm(Cdimg*, uint64_t, int);
void Cputn(Cdimg*, uint64_t, int);
void Crepeat(Cdimg*, int, int);
void Cputs(Cdimg*, char*, int);
void Cwrite(Cdimg*, void*, int);
void Cputr(Cdimg*, Rune);
void Crepeatr(Cdimg*, Rune, int);
void Cputrs(Cdimg*, Rune*, int);
void Cputrscvt(Cdimg*, char*, int);
void Cpadblock(Cdimg*);
void Cputdate(Cdimg*, uint32_t);
void Cputdate1(Cdimg*, uint32_t);
void Cread(Cdimg*, void*, int);
void Cwflush(Cdimg*);
void Cwseek(Cdimg*, int64_t);
uint64_t Cwoffset(Cdimg*);
uint64_t Croffset(Cdimg*);
int Cgetc(Cdimg*);
void Crseek(Cdimg*, int64_t);
char *Crdline(Cdimg*, int);
int Clinelen(Cdimg*);

/* conform.c */
void rdconform(Cdimg*);
char *conform(char*, int);
void wrconform(Cdimg*, int, uint32_t*, uint64_t*);

/* direc.c */
void mkdirec(Direc*, XDir*);
Direc *walkdirec(Direc*, char*);
Direc *adddirec(Direc*, char*, XDir*);
void copydirec(Direc*, Direc*);
void checknames(Direc*, int (*)(char*));
void convertnames(Direc*, char* (*)(char*, char*));
void dsort(Direc*, int (*)(const void*, const void*));
void setparents(Direc*);

/* dump.c */
uint32_t Cputdumpblock(Cdimg*);
int hasdump(Cdimg*);
Dump *dumpcd(Cdimg*, Direc*);
Dumpdir *lookupmd5(Dump*, unsigned char*);
void insertmd5(Dump*, char*, unsigned char*, uint32_t, uint32_t);

Direc readdumpdirs(Cdimg*, XDir*, char*(*)(unsigned char*,int));
char *adddumpdir(Direc*, uint32_t, XDir*);
void copybutname(Direc*, Direc*);

void readkids(Cdimg*, Direc*, char*(*)(unsigned char*,int));
void freekids(Direc*);
void readdumpconform(Cdimg*);
void rmdumpdir(Direc*, char*);

/* ichar.c */
char *isostring(unsigned char*, int);
int isbadiso9660(char*);
int isocmp(const void*, const void*);
int isisofrog(char);
void Cputisopvd(Cdimg*, Cdinfo);

/* jchar.c */
char *jolietstring(unsigned char*, int);
int isbadjoliet(char*);
int jolietcmp(const void*, const void*);
int isjolietfrog(Rune);
void Cputjolietsvd(Cdimg*, Cdinfo);

/* path.c */
void writepathtables(Cdimg*);

/* util.c */
void *emalloc(uint32_t);
void *erealloc(void*, uint32_t);
char *atom(char*);
char *struprcpy(char*, char*);
int chat(char*, ...);

/* unix.c, plan9.c */
void dirtoxdir(XDir*, Dir*);
void fdtruncate(int, uint32_t);
int32_t uidno(char*);
int32_t gidno(char*);

/* rune.c */
Rune *strtorune(Rune*, char*);
Rune *runechr(Rune*, Rune);
int runecmp(Rune*, Rune*);

/* sysuse.c */
int Cputsysuse(Cdimg*, Direc*, int, int, int);

/* write.c */
void writefiles(Dump*, Cdimg*, Direc*);
void writedirs(Cdimg*, Direc*, int(*)(Cdimg*, Direc*, int, int, int));
void writedumpdirs(Cdimg*, Direc*, int(*)(Cdimg*, Direc*, int, int, int));
int Cputisodir(Cdimg*, Direc*, int, int, int);
int Cputjolietdir(Cdimg*, Direc*, int, int, int);
void Cputendvd(Cdimg*);

enum { 
	Blocksize = 2048,
	Ndirblock = 16,		/* directory blocks allocated at once */

	DTdot = 0,
	DTdotdot,
	DTiden,
	DTroot,
	DTrootdot,
};

extern uint32_t now;
extern Conform *map;
extern int chatty;
extern int docolon;
extern int mk9660;
extern int blocksize;
