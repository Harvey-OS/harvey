/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

uint	convM2Dold(uchar*, uint, Dir*, char*);
uint	convD2Mold(Dir*, uchar*, uint);
uint	sizeD2Mold(Dir*);
uint	convM2Sold(uchar*, uint, Fcall*);
uint	convS2Mold(Fcall*, uchar*, uint);
uint	oldhdrsize(uchar);
uint	iosize(uchar*);

enum
{
	oldTnop =		50,
	oldRnop,
	oldTosession =	52,	/* illegal */
	oldRosession,		/* illegal */
	oldTerror =	54,	/* illegal */
	oldRerror,
	oldTflush =	56,
	oldRflush,
	oldToattach =	58,	/* illegal */
	oldRoattach,		/* illegal */
	oldTclone =	60,
	oldRclone,
	oldTwalk =		62,
	oldRwalk,
	oldTopen =		64,
	oldRopen,
	oldTcreate =	66,
	oldRcreate,
	oldTread =		68,
	oldRread,
	oldTwrite =	70,
	oldRwrite,
	oldTclunk =	72,
	oldRclunk,
	oldTremove =	74,
	oldRremove,
	oldTstat =		76,
	oldRstat,
	oldTwstat =	78,
	oldRwstat,
	oldTclwalk =	80,
	oldRclwalk,
	oldTauth =		82,	/* illegal */
	oldRauth,			/* illegal */
	oldTsession =	84,
	oldRsession,
	oldTattach =	86,
	oldRattach,
	oldTmax
};
