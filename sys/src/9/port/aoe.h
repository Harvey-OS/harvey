/* 
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

/*
 * ATA-over-Ethernet (AoE) protocol
 */
enum {
	ACata,
	ACconfig,
};

enum {
	AQCread,
	AQCtest,
	AQCprefix,
	AQCset,
	AQCfset,
};

enum {
	AEcmd	= 1,
	AEarg,
	AEdev,
	AEcfg,
	AEver,
};

enum {
	Aoetype	= 0x88a2,
	Aoesectsz = 512,			/* standard sector size */
	Aoever	= 1,

	AFerr	= 1<<2,
	AFrsp	= 1<<3,

	AAFwrite= 1,
	AAFext	= 1<<6,
};

typedef struct {
	unsigned char	dst[Eaddrlen];
	unsigned char	src[Eaddrlen];
	unsigned char	type[2];
	unsigned char	verflag;
	unsigned char	error;
	unsigned char	major[2];
	unsigned char	minor;
	unsigned char	cmd;
	unsigned char	tag[4];
	unsigned char	payload[];
} Aoehdr;

#define AOEHDRSZ	offsetof(Aoehdr, payload[0])

typedef struct {
	Aoehdr;
	unsigned char	aflag;
	unsigned char	errfeat;
	unsigned char	scnt;
	unsigned char	cmdstat;
	unsigned char	lba[6];
	unsigned char	res[2];
	unsigned char	payload[];
} Aoeata;

#define AOEATASZ	offsetof(Aoeata, payload[0])

typedef struct {
	Aoehdr;
	unsigned char	bufcnt[2];
	unsigned char	fwver[2];
	unsigned char	scnt;
	unsigned char	verccmd;
	unsigned char	cslen[2];
	unsigned char	payload[];
} Aoeqc;

#define AOEQCSZ		offsetof(Aoeqc, payload[0])

extern char Echange[];
extern char Enotup[];
