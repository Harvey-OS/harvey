#include <u.h>
#include <libc.h>
#include <auth.h>
#include <fcall.h>
#include <thread.h>
#include <9p.h>
#include "cifs.h"

static char *period(long sec);

int
shareinfo(Fmt *f)
{
	int i, j, n;
	char *type;
	Shareinfo2 si2;
	Share *sp, *sip;

	if((n = RAPshareenum(Sess, &Ipc, &sip)) < 1){
		fmtprint(f, "can't enumerate shares: %r\n");
		return 0;
	}

	for(i = 0; i < n; i++){
		fmtprint(f, "%-13q ", sip[i].name);

		sp = &sip[i];
		for(j = 0; j < Nshares; j++)
			if(strcmp(Shares[j].name, sip[i].name) == 0){
				sp = &Shares[j];
				break;
			}
		if(j >= Nshares)
			sp->tid = Ipc.tid;

		if(RAPshareinfo(Sess, sp, sp->name, &si2) != -1){
			switch(si2.type){
			case STYPE_DISKTREE:	type = "disk"; break;
			case STYPE_PRINTQ:	type = "printq"; break;
			case STYPE_DEVICE:	type = "device"; break;
			case STYPE_IPC:		type = "ipc"; break;
			case STYPE_SPECIAL:	type = "special"; break;
			case STYPE_TEMP:	type = "temp"; break;
			default:		type = "unknown"; break;
			}

			fmtprint(f, "%-8s %5d/%-5d %s", type,
				si2.activeusrs, si2.maxusrs, si2.comment);
			free(si2.name);
			free(si2.comment);
			free(si2.path);
			free(si2.passwd);
		}
		fmtprint(f, "\n");

	}
	free(sip);
	return 0;
}

int
openfileinfo(Fmt *f)
{
	int got,  i;
	Fileinfo *fi;

	fi = nil;
	if((got = RAPFileenum2(Sess, &Ipc, "", "", &fi)) == -1){
		fmtprint(f, "RAPfileenum: %r\n");
		return 0;
	}

	for(i = 0; i < got; i++){
		fmtprint(f, "%c%c%c %-4d %-24q %q ",
			(fi[i].perms & 1)? 'r': '-',
			(fi[i].perms & 2)? 'w': '-',
			(fi[i].perms & 4)? 'c': '-',
			fi[i].locks, fi[i].user, fi[i].path);
		free(fi[i].path);
		free(fi[i].user);
	}
	free(fi);
	return 0;
}

int
conninfo(Fmt *f)
{
	int i;
	typedef struct {
		int	val;
		char	*name;
	} Tab;
	static Tab captab[] = {
		{ 1,		"raw-mode" },
		{ 2,		"mpx-mode" },
		{ 4,		"unicode" },
		{ 8,		"large-files" },
		{ 0x10,		"NT-smbs" },
		{ 0x20,		"rpc-remote-APIs" },
		{ 0x40,		"status32" },
		{ 0x80,		"l2-oplocks" },
		{ 0x100,	"lock-read" },
		{ 0x200,	"NT-find" },
		{ 0x1000,	"Dfs" },
		{ 0x2000,	"info-passthru" },
		{ 0x4000,	"large-readx" },
		{ 0x8000,	"large-writex" },
		{ 0x800000,	"Unix" },
		{ 0x20000000,	"bulk-transfer" },
		{ 0x40000000,	"compressed" },
		{ 0x80000000,	"extended-security" },
	};
	static Tab sectab[] = {
		{ 1,		"user-auth" },
		{ 2,		"challange-response" },
		{ 4,		"signing-available" },
		{ 8,		"signing-required" },
	};

	fmtprint(f, "%q %q %q %q %+ldsec %dmtu %s\n",
		Sess->auth->user, Sess->cname,
		Sess->auth->windom, Sess->remos,
		Sess->slip, Sess->mtu, Sess->isguest? "as guest": "");

	fmtprint(f, "caps: ");
	for(i = 0; i < nelem(captab); i++)
		if(Sess->caps & captab[i].val)
			fmtprint(f, "%s ", captab[i].name);
	fmtprint(f, "\n");

	fmtprint(f, "security: ");
	for(i = 0; i < nelem(sectab); i++)
		if(Sess->secmode & sectab[i].val)
			fmtprint(f, "%s ", sectab[i].name);
	fmtprint(f, "\n");

	if(Sess->nbt)
		fmtprint(f, "transport: cifs over netbios\n");
	else
		fmtprint(f, "transport: cifs\n");
	return 0;
}

int
sessioninfo(Fmt *f)
{
	int got,  i;
	Sessinfo *si;

	si = nil;
	if((got = RAPsessionenum(Sess, &Ipc, &si)) == -1){
		fmtprint(f, "RAPsessionenum: %r\n");
		return 0;
	}

	for(i = 0; i < got; i++){
		fmtprint(f, "%-24q %-24q ", si[i].user, si[i].wrkstn);
		fmtprint(f, "%12s ", period(si[i].sesstime));
		fmtprint(f, "%12s\n", period(si[i].idletime));
		free(si[i].wrkstn);
		free(si[i].user);
	}
	free(si);
	return 0;
}

/*
 * We request the domain referral for "" which gives the
 * list of all the trusted domains in the clients forest, and
 * other trusted forests.
 *
 * We then sumbit each of these names in turn which gives the
 * names of the domain controllers for that domain.
 *
 * We get a DNS domain name for each domain controller as well as a
 * netbios name.  I THINK I am correct in saying that a name
 * containing a dot ('.') must be a DNS name, as the NetBios
 * name munging cannot encode one.  Thus names which contain no
 * dots must be netbios names.
 *
 */
static void
dfsredir(Fmt *f, char *path, int depth)
{
	Refer *re, retab[128];
	int n, used, flags;

	n = T2getdfsreferral(Sess, &Ipc, path, &flags, &used, retab, nelem(retab));
	if(n == -1)
		return;
	for(re = retab; re < retab+n; re++){
		if(strcmp(path, re->path) != 0)
			dfsredir(f, re->path, depth+1);
		else
			fmtprint(f, "%-32q %q\n", re->path, re->addr);

		free(re->addr);
		free(re->path);
	}
}

int
dfsrootinfo(Fmt *f)
{
	dfsredir(f, "", 0);
	return 0;
}


int
userinfo(Fmt *f)
{
	int got, i;
	Namelist *nl;
	Userinfo ui;

	nl = nil;
	if((got = RAPuserenum2(Sess, &Ipc, &nl)) == -1)
		if((got = RAPuserenum(Sess, &Ipc, &nl)) == -1){
			fmtprint(f, "RAPuserenum: %r\n");
			return 0;
		}

	for(i = 0; i < got; i++){
		fmtprint(f, "%-24q ", nl[i].name);

		if(RAPuserinfo(Sess, &Ipc, nl[i].name, &ui) != -1){
			fmtprint(f, "%-48q %q", ui.fullname, ui.comment);
			free(ui.user);
			free(ui.comment);
			free(ui.fullname);
			free(ui.user_comment);
		}
		free(nl[i].name);
		fmtprint(f, "\n");
	}
	free(nl);
	return 0;
}

int
groupinfo(Fmt *f)
{
	int got1, got2, i, j;
	Namelist *grps, *usrs;

	grps = nil;
	if((got1 = RAPgroupenum(Sess, &Ipc, &grps)) == -1){
		fmtprint(f, "RAPgroupenum: %r\n");
		return 0;
	}

	for(i = 0; i < got1; i++){
		fmtprint(f, "%q ", grps[i].name);
		usrs = nil;
		if((got2 = RAPgroupusers(Sess, &Ipc, grps[i].name, &usrs)) != -1){
			for(j = 0; j < got2; j++){
				fmtprint(f, "%q ", usrs[j].name);
				free(usrs[j].name);
			}
			free(usrs);
		}
		free(grps[i].name);
		fmtprint(f, "\n");
	}
	free(grps);
	return 0;
}

static int
nodelist(Fmt *f, int type)
{
	int more, got, i, j;
	Serverinfo *si;
	static char *types[] = {
		[0]	"workstation",
		[1]	"server",
		[2]	"SQL server",
		[3]	"DC",
		[4]	"backup DC",
		[5]	"time source",
		[6]	"Apple server",
		[7]	"Novell server",
		[8]	"domain member",
		[9]	"printer server",
		[10]	"dial-up server",
		[11]	"Unix",
		[12]	"NT",
		[13]	"WFW",
		[14]	"MFPN (?)",
		[15]	"NT server",
		[16]	"potential browser",
		[17]	"backup browser",
		[18]	"LMB",
		[19]	"DMB",
		[20]	"OSF Unix",
		[21]	"VMS",
		[22]	"Win95",
		[23]	"DFS",
		[24]	"NT cluster",
		[25]	"Terminal server",
		[26]	"[26]",
		[27]	"[27]",
		[28]	"IBM DSS",
	};

	si = nil;
	if((got = RAPServerenum2(Sess, &Ipc, Sess->auth->windom, type, &more,
	    &si)) == -1){
		fmtprint(f, "RAPServerenum2: %r\n");
		return 0;
	}
	if(more)
		if((got = RAPServerenum3(Sess, &Ipc, Sess->auth->windom, type,
		    got-1, si)) == -1){
			fmtprint(f, "RAPServerenum3: %r\n");
			return 0;
		}

	for(i = 0; i < got; i++){
		fmtprint(f, "%-16q %-16q ", si[i].name, si[i].comment);
		if(type != LIST_DOMAINS_ONLY){
			fmtprint(f, "v%d.%d ", si[i].major, si[i].minor);
			for(j = 0; j < nelem(types); j++)
				if(si[i].type & (1 << j) && types[j])
					fmtprint(f, "%s,", types[j]);
		}
		fmtprint(f, "\n");
		free(si[i].name);
		free(si[i].comment);
	}
	free(si);
	return 0;
}

int
domaininfo(Fmt *f)
{
	return nodelist(f, LIST_DOMAINS_ONLY);
}

int
workstationinfo(Fmt *f)
{
	return nodelist(f, ALL_LEARNT_IN_DOMAIN);
}

static char *
period(long sec)
{
	int days, hrs, min;
	static char when[32];

	days = sec  / (60L * 60L * 24L);
	sec -= days * (60L * 60L * 24L);
	hrs  = sec / (60L * 60L);
	sec -= hrs * (60L * 60L);
	min  = sec / 60L;
	sec -= min * 60L;
	if(days)
		snprint(when, sizeof(when), "%d %d:%d:%ld ", days, hrs, min, sec);
	else
		snprint(when, sizeof(when), "%d:%d:%ld ", hrs, min, sec);
	return when;
}
