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
	AEcmd = 1,
	AEarg,
	AEdev,
	AEcfg,
	AEver,
};

enum {
	Aoetype = 0x88a2,
	Aoesectsz = 512, /* standard sector size */
	Aoever = 1,

	AFerr = 1 << 2,
	AFrsp = 1 << 3,

	AAFwrite = 1,
	AAFext = 1 << 6,
};

typedef struct {
	u8 dst[Eaddrlen];
	u8 src[Eaddrlen];
	u8 type[2];
	u8 verflag;
	u8 error;
	u8 major[2];
	u8 minor;
	u8 cmd;
	u8 tag[4];
	u8 payload[];
} Aoehdr;

#define AOEHDRSZ offsetof(Aoehdr, payload[0])

typedef struct {
	Aoehdr;
	u8 aflag;
	u8 errfeat;
	u8 scnt;
	u8 cmdstat;
	u8 lba[6];
	u8 res[2];
	u8 _payload[];
} Aoeata;

#define AOEATASZ offsetof(Aoeata, payload[0])

typedef struct {
	Aoehdr;
	u8 bufcnt[2];
	u8 fwver[2];
	u8 scnt;
	u8 verccmd;
	u8 cslen[2];
	u8 payload[];
} Aoeqc;

#define AOEQCSZ offsetof(Aoeqc, payload[0])

extern char Echange[];
extern char Enotup[];
