/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include	"all.h"

char	*errstr9p[MAXERR] =
{
	[Ebadspc] =	"attach -- bad specifier",
	[Efid] =	"unknown fid",
	[Echar] =	"bad character in directory name",
	[Eopen] =	"read/write -- on non open fid",
	[Ecount] =	"read/write -- count too big",
	[Ealloc] =	"phase error -- directory entry not allocated",
	[Eqid] =	"phase error -- qid does not match",
	[Eaccess] =	"access permission denied",
	[Eentry] =	"directory entry not found",
	[Emode] =	"open/create -- unknown mode",
	[Edir1] =	"walk -- in a non-directory",
	[Edir2] =	"create -- in a non-directory",
	[Ephase] =	"phase error -- cannot happen",
	[Eexist] =	"create/wstat -- file exists",
	[Edot] =	"create/wstat -- . and .. illegal names",
	[Eempty] =	"remove -- directory not empty",
	[Ebadu] =	"attach -- unknown user or failed authentication",
	[Enoattach] =	"attach -- system maintenance",
	[Ewstatb] =	"wstat -- unknown bits in qid.type/mode",
	[Ewstatd] =	"wstat -- attempt to change directory",
	[Ewstatg] =	"wstat -- not in group",
	[Ewstatl] =	"wstat -- attempt to make length negative",
	[Ewstatm] =	"wstat -- attempt to change muid",
	[Ewstato] =	"wstat -- not owner or group leader",
	[Ewstatp] =	"wstat -- attempt to change qid.path",
	[Ewstatq] =	"wstat -- qid.type/dir.mode mismatch",
	[Ewstatu] =	"wstat -- not owner",
	[Ewstatv] =	"wstat -- attempt to change qid.vers",
	[Ename] =	"create/wstat -- bad character in file name",
	[Ewalk] =	"walk -- too many (system wide)",
	[Eronly] =	"file system read only",
	[Efull] =	"file system full",
	[Eoffset] =	"read/write -- offset negative",
	[Elocked] =	"open/create -- file is locked",
	[Ebroken] =	"read/write -- lock is broken",
	[Eauth] =	"attach -- authentication failed",
	[Eauth2] =	"read/write -- authentication unimplemented",
	[Etoolong] =	"name too long",
	[Efidinuse] =	"fid in use",
	[Econvert] =	"protocol botch",
	[Eversion] =	"version conversion",
	[Eauthnone] =	"auth -- user 'none' requires no authentication",
	[Eauthdisabled] = "auth -- authentication disabled",	/* development */
	[Eauthfile] =	"auth -- out of auth files",
	[Eedge] =	"at the bleeding edge",		/* development */
};

char*	wormscode[0x80] =
{
	[0x00] =	"no sense",
	[0x01] =	"invalid command",
	[0x02] =	"recovered error",
	[0x03] =	"illegal request",
	[0x06] =	"unit attention",
	[0x07] =	"parity error",
	[0x08] =	"message reject error",
	[0x0a] =	"copy aborted",
	[0x0b] =	"initiator detected error",
	[0x0c] =	"select re-select failed",
	[0x0e] =	"miscompare",

	[0x10] =	"ecc trouble occurred",
	[0x11] =	"time out error",
	[0x12] =	"controller error",
	[0x13] =	"sony i/f II hardware/firmware error",
	[0x14] =	"scsi hardware/firmware error",
	[0x15] =	"rom version unmatched error",
	[0x16] =	"logical block address out of range",

	[0x20] =	"command not terminated",
	[0x21] =	"drive interface parity error",
	[0x22] =	"loading trouble",
	[0x23] =	"focus trouble",
	[0x24] =	"tracking trouble",
	[0x25] =	"spindle trouble",
	[0x26] =	"slide trouble",
	[0x27] =	"skew trouble",
	[0x28] =	"head lead out",
	[0x29] =	"write modulation trouble",
	[0x2a] =	"under laser power",
	[0x2b] =	"over laser power",
	[0x2f] =	"drive error",

	[0x30] =	"drive power off",
	[0x31] =	"no disk in drive",
	[0x32] =	"drive not ready",
	[0x38] =	"disk already exists in drive",
	[0x39] =	"no disk in shelf",
	[0x3a] =	"disk already exists in shelf",

	[0x40] =	"write warning",
	[0x41] =	"write error",
	[0x42] =	"disk error",
	[0x43] =	"cannot read disk ID",
	[0x44] =	"write protect error 1",
	[0x45] =	"write protect error 2",
	[0x46] =	"disk warning",
	[0x47] =	"alternation trouble",

	[0x50] =	"specified address not found",
	[0x51] =	"address block not found",
	[0x52] =	"all address could not be read",
	[0x53] =	"data could not be read",
	[0x54] =	"uncorrectable read error",
	[0x55] =	"tracking error",
	[0x56] =	"write servo error",
	[0x57] =	"write monitor error",
	[0x58] =	"write verify error",

	[0x60] =	"no data in specified address",
	[0x61] =	"blank check failed",
	[0x62] =	"controller diagnostics failed",
	[0x63] =	"drive diagnostice failed",
	[0x64] =	"diagnostice aborted",
	[0x67] =	"juke diagnostice failed",
	[0x68] =	"z-axis servo failed",
	[0x69] =	"roter servo error",
	[0x6a] =	"hook servo error",
	[0x6b] =	"I/O self error",
	[0x6c] =	"drive 0 error",
	[0x6d] =	"drive 1 error",
	[0x6e] =	"shelf error",
	[0x6f] =	"carrier error",

	[0x70] =	"rob made me do it",
	[0x71] =	"out of range",
};

char*	tagnames[] =
{
	[Tbuck] =	"Tbuck",
	[Tdir] =	"Tdir",
	[Tfile] =	"Tfile",
	[Tfree] =	"Tfree",
	[Tind1] =	"Tind1",
	[Tind2] =	"Tind2",
#ifndef COMPAT32
	[Tind3] =	"Tind3",
	[Tind4] =	"Tind4",
	/* add more Tind tags here ... */
#endif
	[Tnone] =	"Tnone",
	[Tsuper] =	"Tsuper",
	[Tvirgo] =	"Tvirgo",
	[Tcache] =	"Tcache",
};
