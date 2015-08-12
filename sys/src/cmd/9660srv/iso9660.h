/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#define VOLDESC 16 /* sector number */

/*
 * L means little-endian, M means big-endian, and LM means little-endian
 * then again big-endian.
 */
typedef unsigned char Byte2L[2];
typedef unsigned char Byte2M[2];
typedef unsigned char Byte4LM[4];
typedef unsigned char Byte4L[4];
typedef unsigned char Byte4M[4];
typedef unsigned char Byte8LM[8];
typedef union Drec Drec;
typedef union Voldesc Voldesc;

enum {
	Boot = 0,
	Primary = 1,
	Supplementary = 2,
	Partition = 3,
	Terminator = 255
};

union Voldesc {/* volume descriptor */
	unsigned char byte[Sectorsize];
	union {/* for CD001, the ECMA standard */
		struct
		    {
			unsigned char type;
			unsigned char stdid[5];
			unsigned char version;
			unsigned char unused;
			unsigned char sysid[32];
			unsigned char bootid[32];
			unsigned char data[1977];
		} boot;
		struct
		    {
			unsigned char type;
			unsigned char stdid[5];
			unsigned char version;
			unsigned char flags;
			unsigned char sysid[32];
			unsigned char volid[32];
			Byte8LM partloc;
			Byte8LM size;
			unsigned char escapes[32];
			Byte4LM vsetsize;
			Byte4LM vseqno;
			Byte4LM blksize;
			Byte8LM ptabsize;
			Byte4L lptable;
			Byte4L optlptable;
			Byte4M mptable;
			Byte4M optmptable;
			unsigned char rootdir[34];
			unsigned char volsetid[128];
			unsigned char pubid[128];
			unsigned char prepid[128];
			unsigned char appid[128];
			unsigned char copyright[37];
			unsigned char abstract[37];
			unsigned char bibliography[37];
			unsigned char cdate[17];
			unsigned char mdate[17];
			unsigned char expdate[17];
			unsigned char effdate[17];
			unsigned char fsversion;
			unsigned char unused3[1];
			unsigned char appuse[512];
			unsigned char unused4[653];
		} desc;
	} z;
	union {/* for CDROM, the `High Sierra' standard */
		struct
		    {
			Byte8LM number;
			unsigned char type;
			unsigned char stdid[5];
			unsigned char version;
			unsigned char flags;
			unsigned char sysid[32];
			unsigned char volid[32];
			Byte8LM partloc;
			Byte8LM size;
			unsigned char escapes[32];
			Byte4LM vsetsize;
			Byte4LM vseqno;
			Byte4LM blksize;
			unsigned char quux[40];
			unsigned char rootdir[34];
			unsigned char volsetid[128];
			unsigned char pubid[128];
			unsigned char prepid[128];
			unsigned char appid[128];
			unsigned char copyright[32];
			unsigned char abstract[32];
			unsigned char cdate[16];
			unsigned char mdate[16];
			unsigned char expdate[16];
			unsigned char effdate[16];
			unsigned char fsversion;
		} desc;
	} r;
};

union Drec {
	struct
	    {
		unsigned char reclen;
		unsigned char attrlen;
		Byte8LM addr;
		Byte8LM size;
		unsigned char date[6];
		unsigned char tzone;    /* flags in high sierra */
		unsigned char flags;    /* ? in high sierra */
		unsigned char unitsize; /* ? in high sierra */
		unsigned char gapsize;  /* ? in high sierra */
		Byte4LM vseqno;		/* ? in high sierra */
		unsigned char namelen;
		unsigned char name[1];
	};
	struct
	    {
		unsigned char r_pad[24];
		unsigned char r_flags;
	};
};

struct Isofile {
	short fmt; /* 'z' if iso, 'r' if high sierra */
	short blksize;
	int64_t offset;  /* true offset when reading directory */
	int32_t odelta;  /* true size of directory just read */
	int64_t doffset; /* plan9 offset when reading directory */
	Drec d;
};
