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
	uchar	dst[Eaddrlen];
	uchar	src[Eaddrlen];
	uchar	type[2];
	uchar	verflag;
	uchar	error;
	uchar	major[2];
	uchar	minor;
	uchar	cmd;
	uchar	tag[4];
	uchar	payload[];
} Aoehdr;

#define AOEHDRSZ	offsetof(Aoehdr, payload[0])

typedef struct {
	Aoehdr;
	uchar	aflag;
	uchar	errfeat;
	uchar	scnt;
	uchar	cmdstat;
	uchar	lba[6];
	uchar	res[2];
	uchar	payload[];
} Aoeata;

#define AOEATASZ	offsetof(Aoeata, payload[0])

typedef struct {
	Aoehdr;
	uchar	bufcnt[2];
	uchar	fwver[2];
	uchar	scnt;
	uchar	verccmd;
	uchar	cslen[2];
	uchar	payload[];
} Aoeqc;

#define AOEQCSZ		offsetof(Aoeqc, payload[0])
