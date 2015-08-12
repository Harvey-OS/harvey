/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

typedef struct MLock MLock;
typedef struct Iosect Iosect;
typedef struct Iotrack Iotrack;
typedef struct Track Track;
typedef struct Xfs Xfs;

struct MLock {
	char key;
};

struct Iosect {
	Iosect *next;
	short flags;
	MLock lock;
	Iotrack *t;
	unsigned char *iobuf;
};

struct Iotrack {
	short flags;
	Xfs *xf;
	int32_t addr;
	Iotrack *next; /* in lru list */
	Iotrack *prev;
	Iotrack *hnext; /* in hash list */
	Iotrack *hprev;
	MLock lock;
	int ref;
	Track *tp;
};

enum {
	Sectorsize = 512,
	Sect2trk = 9,
	Trksize = Sectorsize * Sect2trk
};

struct Track {
	Iosect *p[Sect2trk];
	unsigned char buf[Sect2trk][Sectorsize];
};

#define BMOD (1 << 0)
#define BIMM (1 << 1)
#define BSTALE (1 << 2)

Iosect *getiosect(Xfs *, int32_t, int);
Iosect *getosect(Xfs *, int32_t);
Iosect *getsect(Xfs *, int32_t);
Iosect *newsect(void);
Iotrack *getiotrack(Xfs *, int32_t);
int canmlock(MLock *);
int devcheck(Xfs *);
int devread(Xfs *, int32_t, void *, int32_t);
int devwrite(Xfs *, int32_t, void *, int32_t);
int tread(Iotrack *);
int twrite(Iotrack *);
void freesect(Iosect *);
void iotrack_init(void);
void mlock(MLock *);
void purgebuf(Xfs *);
void purgetrack(Iotrack *);
void putsect(Iosect *);
void sync(void);
void unmlock(MLock *);
