/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include "headers.h"

SmbGlobals smbglobals = {
	.maxreceive = 0x1ffff,
	.unicode = 1,
	.nativeos = "Plan 9 4th edition",
	.serverinfo = {
		.nativelanman = "Aquarela",
		.vmaj = 0,
		.vmin = 5,
		.stype = SV_TYPE_SERVER,
	},
	.mailslotbrowse = "/MAILSLOT/BROWSE",
	.pipelanman = "/PIPE/LANMAN",
	.l2sectorsize = 9,
	.l2allocationsize = 14,
	.convertspace = 0,
	.log = {
		.fd = -1,
		.print = 0,
		.poolparanoia = 1,
	},
};

void
smbglobalsguess(int client)
{
	if (smbglobals.serverinfo.name == 0)
		smbglobals.serverinfo.name = sysname();
	if (smbglobals.nbname[0] == 0)
		nbmknamefromstring(smbglobals.nbname, smbglobals.serverinfo.name);
	if (smbglobals.accountname == nil)
		smbglobals.accountname = strdup(getuser());
	if (smbglobals.primarydomain == nil)
		smbglobals.primarydomain = "PLAN9";
	if (smbglobals.serverinfo.remark == nil)
		smbglobals.serverinfo.remark = "This is a default server comment";
	if (smbglobals.log.fd < 0)
		if (client){
			smbglobals.log.fd = create("client.log", OWRITE|OTRUNC, 0666);
		}
		else{
			if (access("/sys/log/aquarela", 2) == 0)
				smbglobals.log.fd = open("/sys/log/aquarela", OWRITE);
		}
}
