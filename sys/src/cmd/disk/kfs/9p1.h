/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#define DIRREC 116 /* size of a directory ascii record */
#define ERRREC 64  /* size of a error record */
#define MAXMSG 160 /* max header sans data */

typedef struct Oldfcall Oldfcall;

struct Oldfcall {
	char type;
	uint16_t fid;
	short err;
	short tag;
	union {
		struct
		    {
			short uid;		/* T-Userstr */
			short oldtag;		/* T-nFlush */
			Qid9p1 qid;		/* R-Attach, R-Clwalk, R-Walk,
						 * R-Open, R-Create */
			char rauth[AUTHENTLEN]; /* R-attach */
		};
		struct
		    {
			char uname[NAMELEN];    /* T-nAttach */
			char aname[NAMELEN];    /* T-nAttach */
			char ticket[TICKETLEN]; /* T-attach */
			char auth[AUTHENTLEN];  /* T-attach */
		};
		struct
		    {
			char ename[ERRREC];   /* R-nError */
			char chal[CHALLEN];   /* T-session, R-session */
			char authid[NAMELEN]; /* R-session */
			char authdom[DOMLEN]; /* R-session */
		};
		struct
		    {
			char name[NAMELEN]; /* T-Walk, T-Clwalk, T-Create, T-Remove */
			int32_t perm;       /* T-Create */
			uint16_t newfid;    /* T-Clone, T-Clwalk */
			char mode;	  /* T-Create, T-Open */
		};
		struct
		    {
			int32_t offset; /* T-Read, T-Write */
			int32_t count;  /* T-Read, T-Write, R-Read */
			char *data;     /* T-Write, R-Read */
		};
		struct
		    {
			char stat[DIRREC]; /* T-Wstat, R-Stat */
		};
	};
};

/*
 * P9 protocol message types
 */
enum {
	Tnop9p1 = 50,
	Rnop9p1,
	Tosession9p1 = 52,
	Rosession9p1,
	Terror9p1 = 54, /* illegal */
	Rerror9p1,
	Tflush9p1 = 56,
	Rflush9p1,
	Toattach9p1 = 58,
	Roattach9p1,
	Tclone9p1 = 60,
	Rclone9p1,
	Twalk9p1 = 62,
	Rwalk9p1,
	Topen9p1 = 64,
	Ropen9p1,
	Tcreate9p1 = 66,
	Rcreate9p1,
	Tread9p1 = 68,
	Rread9p1,
	Twrite9p1 = 70,
	Rwrite9p1,
	Tclunk9p1 = 72,
	Rclunk9p1,
	Tremove9p1 = 74,
	Rremove9p1,
	Tstat9p1 = 76,
	Rstat9p1,
	Twstat9p1 = 78,
	Rwstat9p1,
	Tclwalk9p1 = 80,
	Rclwalk9p1,
	Tauth9p1 = 82, /* illegal */
	Rauth9p1,      /* illegal */
	Tsession9p1 = 84,
	Rsession9p1,
	Tattach9p1 = 86,
	Rattach9p1,

	MAXSYSCALL
};

int convD2M9p1(Dentry *, char *);
int convM2D9p1(char *, Dentry *);
int convM2S9p1(uint8_t *, Oldfcall *, int);
int convS2M9p1(Oldfcall *, uint8_t *);
void fcall9p1(Chan *, Oldfcall *, Oldfcall *);
int authorize(Chan *, Oldfcall *, Oldfcall *);

void (*call9p1[MAXSYSCALL])(Chan *, Oldfcall *, Oldfcall *);

extern Nvrsafe nvr;
