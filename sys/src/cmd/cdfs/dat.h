/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

enum { Maxtrack = 200,
       Ntrack = Maxtrack + 1,
       BScdrom = 2048, /* mmc data block size */
       BScdda = 2352,
       BScdxa = 2336,
       BSmax = 2352,

       Maxfeatures = 512,

       /* scsi peripheral device types, SPC-3 §6.4.2 */
       TypeDA = 0, /* Direct Access (SBC) */
       TypeSA = 1, /* Sequential Access (SSC) */
       TypeWO = 4, /* Worm (SBC)*/
       TypeCD = 5, /* CD/DVD/BD (MMC) */
       TypeMO = 7, /* rewriteable Magneto-Optical (SBC) */
       TypeMC = 8, /* Medium Changer (SMC) */

       /* MMC device types */
       Mmcnone = 0,
       Mmccd,
       Mmcdvdminus,
       Mmcdvdplus,
       Mmcbd,

       /* disc or track types */
       TypeNone = 0,
       TypeAudio,
       TypeAwritable,
       TypeData,
       TypeDwritable,
       TypeDisk,
       TypeBlank,

       /* disc writability classes */
       Readonly = 0, /* -ROM */
       Write1,       /* -R: write once only */
       Erasewrite,   /* -R[WE]: erase then write */
       Ram,          /* -RAM: read & write unrestricted */

       /* tri-state flags */
       Unset = -1,
       No,
       Yes,

       /* offsets in Pagcapmechsts mode page; see MMC-3 §5.5.10 */
       Capread = 2,
       Capwrite = 3,
       Capmisc = 5,

       /* device capabilities in Pagcapmechsts mode page */
       Capcdr = 1 << 0, /* bytes 2 & 3 */
       Capcdrw = 1 << 1,
       Captestwr = 1 << 2,
       Capdvdrom = 1 << 3,
       Capdvdr = 1 << 4,
       Capdvdram = 1 << 5,
       Capcdda = 1 << 0, /* Capmisc bits */
       Caprw = 1 << 2,

       /* Pagwrparams mode page offsets */
       Wpwrtype = 2, /* write type */
       Wptrkmode,    /* track mode */
       Wpdatblktype,
       Wpsessfmt = 8,
       Wppktsz = 10, /* BE uint32_t: # user data blks/fixed pkt */

       /* Pagwrparams bits */
       Bufe = 1 << 6, /* Wpwrtype: buffer under-run free recording enable */
       /* Wptrkmode */
       Msbits = 3 << 6,     /* multisession field */
       Msnonext = 0 << 6,   /* no next border nor session */
       Mscdnonext = 1 << 6, /* cd special: no next session */
       Msnext = 3 << 6,     /* next session or border allowed */
       Fp = 1 << 5,         /* pay attention to Wppktsz */

       /* close track session cdb bits */
       Closetrack = 1,
       Closesessfinal = 2,  /* close session / finalize disc */
       Closefinaldvdrw = 3, /* dvd-rw special: finalize */
       /* dvd+r dl special: close session, write extended lead-out */
       Closesessextdvdrdl = 4,
       Closefinal30mm = 5,   /* dvd+r special: finalize with ≥30mm radius */
       Closedvdrbdfinal = 6, /* dvd+r, bd-r special: finalize */

       /* read toc format values */
       Tocfmttoc = 0,
       Tocfmtsessnos = 1,
       Tocfmtqleadin = 2,
       Tocfmtqpma = 3,
       Tocfmtatip = 4,
       Tocfmtcdtext = 5,

       /* read toc cdb[1] bit */
       Msfbit = 1 << 1,

       /* write types, MMC-6 §7.5.4.9 */
       Wtpkt = 0, /* a.k.a. incremental */
       Wttrackonce,
       Wtsessonce, /* a.k.a. disc-at-once */
       Wtraw,
       Wtlayerjump,

       /* track modes (determine: are these also track types?) */
       Tmcdda = 0,   /* audio cdda */
       Tm2audio,     /* 2 audio channels */
       Tmunintr = 4, /* data, recorded uninterrupted */
       Tmintr,       /* data, recorded interrupted (dvd default) */

       /* data block types */
       Dbraw = 0,    /* 2352 bytes */
       Db2kdata = 8, /* mode 1: 2K of user data */
       Db2336,       /* mode 2: 2336 bytes of user data */

       /* session formats */
       Sfdata = 0,
       Sfcdi = 0x10,
       Sfcdxa = 0x20,

       /* Cache control bits in mode page 8 byte 2 */
       Ccrcd = 1 << 0, /* read cache disabled */
       Ccmf = 1 << 1,  /* multiplication factor */
       Ccwce = 1 << 2, /* writeback cache enabled */
       Ccsize =
	   1 << 3, /* use `cache segment size', not `# of cache segments' */
       Ccdisc = 1 << 4, /* discontinuity */
       Cccap = 1 << 5,  /* caching analysis permitted */
       Ccabpf = 1 << 6, /* abort pre-fetch */
       Ccic = 1 << 7,   /* initiator control */

       /* drive->cap bits */
       Cwrite = 1 << 0,
       Ccdda = 1 << 1,

       CDNblock = 12,  /* chosen for CD */
       DVDNblock = 16, /* DVD ECC block is 16 sectors */
       BDNblock = 32,  /* BD ECC block (`cluster') is 32 sectors */
       /* BD-R are write-once in increments of 64KB */
       /*
        * make a single transfer fit in a 9P rpc.  if we don't do this,
        * remote access (e.g., via /mnt/term/dev/sd*) fails mysteriously.
        */
       Readblock = 8192 / BScdrom,
};

typedef struct Buf Buf;
typedef struct Dev Dev;
typedef struct Drive Drive;
typedef struct Msf Msf; /* minute, second, frame */
typedef struct Otrack Otrack;
typedef struct Track Track;
typedef uint8_t Tristate;

struct Msf {
	int m;
	int s;
	int f;
};

struct Track {
	/* initialized while obtaining the toc (gettoc) */
	int64_t size; /* total size in bytes */
	int32_t bs;   /* block size in bytes */
	uint32_t beg; /* beginning block number */
	uint32_t end; /* ending block number */
	int type;
	Msf mbeg;
	Msf mend;

	/* initialized by fs */
	char name[32];
	int mode;
	uint32_t mtime;
};

struct DTrack /* not used */
    {
	unsigned char name[32];
	unsigned char beg[4]; /* msf value; only used for audio */
	unsigned char end[4]; /* msf value; only used for audio */
	unsigned char size[8];
	unsigned char magic[4];
};

struct Otrack {
	Track* track;
	Drive* drive;
	int nchange;
	int omode;
	Buf* buf;

	int nref; /* kept by file server */
};

struct Dev {
	Otrack* (*openrd)(Drive* d, int trackno);
	Otrack* (*create)(Drive* d, int bs);
	int32_t (*read)(Otrack* t, void* v, int32_t n, int64_t off);
	int32_t (*write)(Otrack* t, void* v, int32_t n);
	void (*close)(Otrack* t);
	int (*gettoc)(Drive*);
	int (*fixate)(Drive* d);
	char* (*ctl)(Drive* d, int argc, char** argv);
	char* (*setspeed)(Drive* d, int r, int w);
};

struct Drive {
	QLock qlock;
	Scsi scsi;

	int type; /* scsi peripheral device type: Type?? */

	/* disc characteristics */
	int mmctype;   /* cd, dvd, or bd */
	char* dvdtype; /* name of dvd flavour */
	char* laysfx;  /* layer suffix (e.g., -dl) */
	int firsttrack;
	int invistrack;
	int ntrack;
	int nchange;         /* compare with the members in Scsi */
	uint32_t changetime; /* " */
	int relearn;         /* need to re-learn the disc? */
	int nameok;
	int writeok;         /* writable disc? */
	                     /*
	                      * we could combine these attributes into a single variable except
	                      * that we discover them separately sometimes.
	                      */
	Tristate recordable; /* writable by burning? */
	Tristate erasable;   /* writable after erasing? */

	Track track[Ntrack];
	uint32_t end; /* # of blks on current disc */
	uint32_t cap; /* drive capabilities */
	unsigned char blkbuf[BScdda];

	int maxreadspeed;
	int maxwritespeed;
	int readspeed;
	int writespeed;
	Dev;

	unsigned char features[Maxfeatures / 8];

	void* aux; /* kept by driver */
};

struct Buf {
	unsigned char* data; /* buffer */
	int64_t off;         /* data[0] at offset off in file */
	int bs;              /* block size */
	int32_t ndata;       /* no. valid bytes in data */
	int nblock;          /* total buffer size in blocks */
	int omode;           /* OREAD, OWRITE */
	int32_t (*fn)(Buf*, void*, int32_t, uint32_t); /* read, write */

	/* used only by client */
	Otrack* otrack;
};

extern int vflag;
