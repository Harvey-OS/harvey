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
	uint8_t	dst[Eaddrlen];
	uint8_t	src[Eaddrlen];
	uint8_t	type[2];
	uint8_t	verflag;
	uint8_t	error;
	uint8_t	major[2];
	uint8_t	minor;
	uint8_t	cmd;
	uint8_t	tag[4];
	uint8_t	payload[];
} Aoehdr;

#define AOEHDRSZ	offsetof(Aoehdr, payload[0])

typedef struct {
	Aoehdr;
	uint8_t	aflag;
	uint8_t	errfeat;
	uint8_t	scnt;
	uint8_t	cmdstat;
	uint8_t	lba[6];
	uint8_t	res[2];
	uint8_t	_payload[];
} Aoeata;

#define AOEATASZ	offsetof(Aoeata, _payload[0])

typedef struct {
	Aoehdr;
	uint8_t	bufcnt[2];
	uint8_t	fwver[2];
	uint8_t	scnt;
	uint8_t	verccmd;
	uint8_t	cslen[2];
	uint8_t	__payload[];
} Aoeqc;

#define AOEQCSZ		offsetof(Aoeqc, __payload[0])

extern char Echange[];
extern char Enotup[];
