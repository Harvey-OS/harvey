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
SReinitialise(ScsiReq *rp)
{
	uint8_t cmd[6];

	memset(cmd, 0, sizeof(cmd));
	cmd[0] = ScmdEInitialise;
	rp->cmd.p = cmd;
	rp->cmd.count = sizeof(cmd);
	rp->data.p = cmd;
	rp->data.count = 0;
	rp->data.write = 1;
	return SRrequest(rp);
}

int32_t
SRmmove(ScsiReq *rp, int transport, int source, int destination, int invert)
{
	uint8_t cmd[12];

	memset(cmd, 0, sizeof(cmd));
	cmd[0] = ScmdMMove;
	cmd[2] = transport >> 8;
	cmd[3] = transport;
	cmd[4] = source >> 8;
	cmd[5] = source;
	cmd[6] = destination >> 8;
	cmd[7] = destination;
	cmd[10] = invert & 0x01;
	rp->cmd.p = cmd;
	rp->cmd.count = sizeof(cmd);
	rp->data.p = cmd;
	rp->data.count = 0;
	rp->data.write = 1;
	return SRrequest(rp);
}

int32_t
SRestatus(ScsiReq *rp, uint8_t type, uint8_t *list, int nbytes)
{
	uint8_t cmd[12];

	memset(cmd, 0, sizeof(cmd));
	cmd[0] = ScmdEStatus;
	cmd[1] = type & 0x07;
	cmd[4] = 0xFF;
	cmd[5] = 0xFF;
	cmd[7] = nbytes >> 16;
	cmd[8] = nbytes >> 8;
	cmd[9] = nbytes;
	rp->cmd.p = cmd;
	rp->cmd.count = sizeof(cmd);
	rp->data.p = list;
	rp->data.count = nbytes;
	rp->data.write = 0;
	return SRrequest(rp);
}
