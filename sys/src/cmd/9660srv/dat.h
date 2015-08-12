/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

typedef struct Ioclust Ioclust;
typedef struct Iobuf Iobuf;
typedef struct Isofile Isofile;
typedef struct Xdata Xdata;
typedef struct Xfile Xfile;
typedef struct Xfs Xfs;
typedef struct Xfsub Xfsub;

#pragma incomplete Isofile

enum {
	Sectorsize = 2048,
	Maxname = 256,
};

struct Iobuf {
	Ioclust *clust;
	int32_t addr;
	unsigned char *iobuf;
};

struct Ioclust {
	int32_t addr; /* in sectors; good to 8TB */
	Xdata *dev;
	Ioclust *next;
	Ioclust *prev;
	int busy;
	int nbuf;
	Iobuf *buf;
	unsigned char *iobuf;
};

struct Xdata {
	Xdata *next;
	char *name; /* of underlying file */
	Qid qid;
	uint16_t type;
	uint16_t fdev;
	int ref; /* attach count */
	int dev; /* for read/write */
};

struct Xfsub {
	void (*reset)(void);
	int (*attach)(Xfile *);
	void (*clone)(Xfile *, Xfile *);
	void (*walkup)(Xfile *);
	void (*walk)(Xfile *, char *);
	void (*open)(Xfile *, int);
	void (*create)(Xfile *, char *, int32_t, int);
	int32_t (*readdir)(Xfile *, unsigned char *, int32_t, int32_t);
	int32_t (*read)(Xfile *, char *, int64_t, int32_t);
	int32_t (*write)(Xfile *, char *, int64_t, int32_t);
	void (*clunk)(Xfile *);
	void (*remove)(Xfile *);
	void (*stat)(Xfile *, Dir *);
	void (*wstat)(Xfile *, Dir *);
};

struct Xfs {
	Xdata *d; /* how to get the bits */
	Xfsub *s; /* how to use them */
	int ref;
	int issusp;      /* follows system use sharing protocol */
	int32_t suspoff; /* if so, offset at which SUSP area begins */
	int isrock;      /* Rock Ridge format */
	int isplan9;     /* has Plan 9-specific directory info */
	Qid rootqid;
	Isofile *ptr; /* private data */
};

struct Xfile {
	Xfile *next; /* in fid hash bucket */
	Xfs *xf;
	int32_t fid;
	uint32_t flags;
	Qid qid;
	int len; /* of private data */
	Isofile *ptr;
};

enum {
	Asis,
	Clean,
	Clunk
};

enum {
	Oread = 1,
	Owrite = 2,
	Orclose = 4,
	Omodes = 3,
};

extern char Enonexist[]; /* file does not exist */
extern char Eperm[];     /* permission denied */
extern char Enofile[];   /* no file system specified */
extern char Eauth[];     /* authentication failed */

extern char *srvname;
extern char *deffile;
extern int chatty;
extern jmp_buf err_lab[];
extern int nerr_lab;
extern char err_msg[];

extern int nojoliet;
extern int noplan9;
extern int norock;
