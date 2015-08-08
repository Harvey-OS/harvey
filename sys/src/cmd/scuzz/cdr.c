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
#include <disk.h>
#include "scsireq.h"

int32_t
SRblank(ScsiReq* rp, uint8_t type, uint8_t track)
{
	uint8_t cmd[12];

	memset(cmd, 0, sizeof(cmd));
	cmd[0] = ScmdBlank;
	cmd[1] = type;
	cmd[2] = track >> 24;
	cmd[3] = track >> 16;
	cmd[4] = track >> 8;
	cmd[5] = track;
	rp->cmd.p = cmd;
	rp->cmd.count = sizeof(cmd);
	rp->data.p = cmd;
	rp->data.count = 0;
	rp->data.write = 1;
	return SRrequest(rp);
}

int32_t
SRsynccache(ScsiReq* rp)
{
	uint8_t cmd[10];

	memset(cmd, 0, sizeof(cmd));
	cmd[0] = ScmdSynccache;
	rp->cmd.p = cmd;
	rp->cmd.count = sizeof(cmd);
	rp->data.p = cmd;
	rp->data.count = 0;
	rp->data.write = 1;
	return SRrequest(rp);
}

int32_t
SRTOC(ScsiReq* rp, void* data, int nbytes, uint8_t format, uint8_t track)
{
	uint8_t cmd[10];

	memset(cmd, 0, sizeof(cmd));
	cmd[0] = ScmdRTOC;
	cmd[2] = format;
	cmd[6] = track;
	cmd[7] = nbytes >> 8;
	cmd[8] = nbytes;
	rp->cmd.p = cmd;
	rp->cmd.count = sizeof(cmd);
	rp->data.p = data;
	rp->data.count = nbytes;
	rp->data.write = 0;
	return SRrequest(rp);
}

int32_t
SRrdiscinfo(ScsiReq* rp, void* data, int nbytes)
{
	uint8_t cmd[10];

	memset(cmd, 0, sizeof(cmd));
	cmd[0] = ScmdRdiscinfo;
	cmd[7] = nbytes >> 8;
	cmd[8] = nbytes;
	rp->cmd.p = cmd;
	rp->cmd.count = sizeof(cmd);
	rp->data.p = data;
	rp->data.count = nbytes;
	rp->data.write = 0;
	return SRrequest(rp);
}

int32_t
SRrtrackinfo(ScsiReq* rp, void* data, int nbytes, int track)
{
	uint8_t cmd[10];

	memset(cmd, 0, sizeof(cmd));
	cmd[0] = ScmdRtrackinfo;
	cmd[1] = 0x01;
	cmd[2] = track >> 24;
	cmd[3] = track >> 16;
	cmd[4] = track >> 8;
	cmd[5] = track;
	cmd[7] = nbytes >> 8;
	cmd[8] = nbytes;
	rp->cmd.p = cmd;
	rp->cmd.count = sizeof(cmd);
	rp->data.p = data;
	rp->data.count = nbytes;
	rp->data.write = 0;
	return SRrequest(rp);
}

int32_t
SRfwaddr(ScsiReq* rp, uint8_t track, uint8_t mode, uint8_t npa, uint8_t* data)
{
	uint8_t cmd[10];

	memset(cmd, 0, sizeof(cmd));
	cmd[0] = ScmdFwaddr;
	cmd[2] = track;
	cmd[3] = mode;
	cmd[7] = npa;
	rp->cmd.p = cmd;
	rp->cmd.count = sizeof(cmd);
	rp->data.p = data;
	rp->data.count = MaxDirData;
	rp->data.write = 0;
	return SRrequest(rp);
}

int32_t
SRtreserve(ScsiReq* rp, int32_t nbytes)
{
	uint8_t cmd[10];
	int32_t n;

	if((nbytes % rp->lbsize)) {
		rp->status = Status_BADARG;
		return -1;
	}
	memset(cmd, 0, sizeof(cmd));
	cmd[0] = ScmdTreserve;
	n = nbytes / rp->lbsize;
	cmd[5] = n >> 24;
	cmd[6] = n >> 16;
	cmd[7] = n >> 8;
	cmd[8] = n;
	rp->cmd.p = cmd;
	rp->cmd.count = sizeof(cmd);
	rp->data.p = cmd;
	rp->data.count = 0;
	rp->data.write = 1;
	return SRrequest(rp);
}

int32_t
SRtinfo(ScsiReq* rp, uint8_t track, uint8_t* data)
{
	uint8_t cmd[10];

	memset(cmd, 0, sizeof(cmd));
	cmd[0] = ScmdTinfo;
	cmd[5] = track;
	cmd[8] = MaxDirData;
	rp->cmd.p = cmd;
	rp->cmd.count = sizeof(cmd);
	rp->data.p = data;
	rp->data.count = MaxDirData;
	rp->data.write = 0;
	return SRrequest(rp);
}

int32_t
SRwtrack(ScsiReq* rp, void* buf, int32_t nbytes, uint8_t track, uint8_t mode)
{
	uint8_t cmd[10];
	int32_t m, n;

	if((nbytes % rp->lbsize) || nbytes > maxiosize) {
		rp->status = Status_BADARG;
		return -1;
	}
	memset(cmd, 0, sizeof(cmd));
	cmd[0] = ScmdTwrite;
	cmd[5] = track;
	cmd[6] = mode;
	n = nbytes / rp->lbsize;
	cmd[7] = n >> 8;
	cmd[8] = n;
	rp->cmd.p = cmd;
	rp->cmd.count = sizeof(cmd);
	rp->data.p = buf;
	rp->data.count = nbytes;
	rp->data.write = 1;
	m = SRrequest(rp);
	if(m < 0)
		return -1;
	rp->offset += n;
	return m;
}

int32_t
SRmload(ScsiReq* rp, uint8_t code)
{
	uint8_t cmd[12];

	memset(cmd, 0, sizeof(cmd));
	cmd[0] = ScmdMload;
	cmd[8] = code;
	rp->cmd.p = cmd;
	rp->cmd.count = sizeof(cmd);
	rp->data.p = cmd;
	rp->data.count = 0;
	rp->data.write = 1;
	return SRrequest(rp);
}

int32_t
SRfixation(ScsiReq* rp, uint8_t type)
{
	uint8_t cmd[10];

	memset(cmd, 0, sizeof(cmd));
	cmd[0] = ScmdFixation;
	cmd[8] = type;
	rp->cmd.p = cmd;
	rp->cmd.count = sizeof(cmd);
	rp->data.p = cmd;
	rp->data.count = 0;
	rp->data.write = 1;
	return SRrequest(rp);
}
