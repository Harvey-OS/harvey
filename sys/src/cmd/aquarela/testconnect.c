#include "headers.h"

void
threadmain(int argc, char *argv[])
{
	SmbClient *c;
	char *errmsg;
	if (argc != 2 && argc != 3) {
		print("usage: testconnect to [share]\n");
		exits("args");
	}
	smbglobalsguess(1);
	errmsg = nil;
	c = smbconnect(argv[1], argc == 3 ? argv[2] : nil, &errmsg);
	if (c) {
		int i, rv;
		int entries;
		SmbRapServerInfo1 *si = nil;
		SmbFindFileBothDirectoryInfo ip[10];
		char *errmsg;
		ushort sid, searchcount, endofsearch;
		errmsg = nil;
		rv = smbnetserverenum2(c, SV_TYPE_SERVER, "PLAN9", &entries, &si, &errmsg);
		if (rv < 0)
			print("error: %s\n", errmsg);
		else if (rv > 0)
			print("error code %d\n", rv);
		else
			for (i = 0; i < entries; i++)
				print("%s: %d.%d 0x%.8lux %s\n", si[i].name, si[i].vmaj, si[i].vmin, si[i].type, si[i].remark);
		free(si);
		if (rv == 0) {
			rv = smbnetserverenum2(c, SV_TYPE_DOMAIN_ENUM, nil, &entries, &si, &errmsg);
			if (rv < 0)
				print("error: %s\n", errmsg);
			else if (rv > 0)
				print("error code %d\n", rv);
			else
				for (i = 0; i < entries; i++)
					print("%s: %d.%d 0x%.8lux %s\n", si[i].name, si[i].vmaj, si[i].vmin, si[i].type, si[i].remark);
			free(si);
		}
		rv = smbclienttrans2findfirst2(c, nelem(ip), "\\LICENSE",
			&sid, &searchcount, &endofsearch, ip, &errmsg);
		if (rv) {
			print("sid 0x%.4ux\n", sid);
			print("searchcount 0x%.4ux\n", searchcount);
			print("endofsearch 0x%.4ux\n", endofsearch);
		}
		else
			print("search failed %s\n", errmsg);
		smbclientfree(c);
	}
	else
		print("failed to connect: %s\n", errmsg);
}
