/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include <u.h>
#include <libc.h>
#include <bio.h>
#include <disk.h>
#include "scsireq.h"

extern Biobuf bout;

i32
SRcdpause(ScsiReq *rp, int resume)
{
	u8 cmd[10];

	memset(cmd, 0, sizeof(cmd));
	cmd[0] = ScmdCDpause;
	cmd[8] = resume;
	rp->cmd.p = cmd;
	rp->cmd.count = sizeof(cmd);
	rp->data.p = cmd;
	rp->data.count = 0;
	rp->data.write = 1;
	return SRrequest(rp);
}

i32
SRcdstop(ScsiReq *rp)
{
	u8 cmd[10];

	memset(cmd, 0, sizeof(cmd));
	cmd[0] = ScmdCDstop;
	rp->cmd.p = cmd;
	rp->cmd.count = sizeof(cmd);
	rp->data.p = cmd;
	rp->data.count = 0;
	rp->data.write = 1;
	return SRrequest(rp);
}

static i32
_SRcdplay(ScsiReq *rp, i32 lba, i32 length)
{
	u8 cmd[12];

	memset(cmd, 0, sizeof(cmd));
	cmd[0] = ScmdCDplay;
	cmd[2] = lba>>24;
	cmd[3] = lba>>16;
	cmd[4] = lba>>8;
	cmd[5] = lba;
	cmd[6] = length>>24;
	cmd[7] = length>>16;
	cmd[8] = length>>8;
	cmd[9] = length;
	rp->cmd.p = cmd;
	rp->cmd.count = sizeof(cmd);
	rp->data.p = cmd;
	rp->data.count = 0;
	rp->data.write = 1;

	return SRrequest(rp);
}

static struct {
	int	trackno;
	i32	lba;
	i32	length;
} tracks[100];
static int ntracks;

i32
SRcdplay(ScsiReq *rp, int raw, i32 start, i32 length)
{
	u8 d[100*8+4], *p;
	int lba, n, tdl;

	if(raw || start == 0)
		return _SRcdplay(rp, start, length);

	ntracks = 0;
	if(SRTOC(rp, d, sizeof(d), 0, 0) == -1){
		if(rp->status == STok)
			Bprint(&bout, "\t(probably empty)\n");
		return -1;
	}
	tdl = (d[0]<<8)|d[1];
	for(p = &d[4], n = tdl-2; n; n -= 8, p += 8){
		tracks[ntracks].trackno = p[2];
		lba = (p[4]<<24)|(p[5]<<16)|(p[6]<<8)|p[7];
		tracks[ntracks].lba = lba;
		if(ntracks > 0)
			tracks[ntracks-1].length = lba-tracks[ntracks-1].lba;
		ntracks++;
	}
	if(ntracks > 0)
		tracks[ntracks-1].length = 0xFFFFFFFF;

	for(n = 0; n < ntracks; n++){
		if(start != tracks[n].trackno)
			continue;
		return _SRcdplay(rp, tracks[n].lba, tracks[n].length);
	}

	return -1;
}

i32
SRcdload(ScsiReq *rp, int load, int slot)
{
	u8 cmd[12];

	memset(cmd, 0, sizeof(cmd));
	cmd[0] = ScmdCDload;
	if(load)
		cmd[4] = 0x03;
	else
		cmd[4] = 0x02;
	cmd[8] = slot;
	rp->cmd.p = cmd;
	rp->cmd.count = sizeof(cmd);
	rp->data.p = cmd;
	rp->data.count = 0;
	rp->data.write = 1;
	return SRrequest(rp);
}

i32
SRcdstatus(ScsiReq *rp, u8 *list, int nbytes)
{
	u8 cmd[12];

	memset(cmd, 0, sizeof(cmd));
	cmd[0] = ScmdCDstatus;
	cmd[8] = nbytes>>8;
	cmd[9] = nbytes;
	rp->cmd.p = cmd;
	rp->cmd.count = sizeof(cmd);
	rp->data.p = list;
	rp->data.count = nbytes;
	rp->data.write = 0;
	return SRrequest(rp);
}

i32
SRgetconf(ScsiReq *rp, u8 *list, int nbytes)
{
	u8 cmd[10];

	memset(cmd, 0, sizeof(cmd));
	cmd[0] = Scmdgetconf;
	cmd[7] = nbytes>>8;
	cmd[8] = nbytes;
	rp->cmd.p = cmd;
	rp->cmd.count = sizeof(cmd);
	rp->data.p = list;
	rp->data.count = nbytes;
	rp->data.write = 0;
	return SRrequest(rp);
}
