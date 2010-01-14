#include <u.h>
#include <libc.h>
#include <auth.h>
#include <fcall.h>
#include <thread.h>
#include <9p.h>
#include "cifs.h"

struct {		/* Well known security IDs */
	char	*name;
	char	*auth;
	char	*rid;
} known[] = {
	/* default local users */
	{ "lu.dialup",			"S-1-5-1",	nil },
	{ "lu.network",			"S-1-5-2",	nil },
	{ "lu.batch",			"S-1-5-3",	nil },
	{ "lu.interactive",		"S-1-5-4",	nil },
	{ "lu.service",			"S-1-5-6",	nil },
	{ "lu.anon",			"S-1-5-7",	nil },
	{ "lu.DC",			"S-1-5-8",	nil },
	{ "lu.enterprise-domain",	"S-1-5-9",	nil },
	{ "lu.self",			"S-1-5-10",	nil },
	{ "lu.authenticated",		"S-1-5-11",	nil },
	{ "lu.restricted",		"S-1-5-12",	nil },
	{ "lu.terminal-services",	"S-1-5-13",	nil },
	{ "lu.remote-desktop",		"S-1-5-14",	nil },
	{ "lu.local-system",		"S-1-5-18",	nil },
	{ "lu.local-service",		"S-1-5-19",	nil },
	{ "lu.network-service",		"S-1-5-20",	nil },
	{ "lu.builtin",			"S-1-5-32",	nil },

	/* default local groups */
	{ "lg.null",			"S-1-0-0",	nil },
	{ "lg.world",			"S-1-1-0",	nil },
	{ "lg.local",			"S-1-2-0",	nil },
	{ "lg.creator-owner",		"S-1-3-0",	nil },
	{ "lg.creator-group",		"S-1-3-1",	nil },
	{ "lg.creator-owner-server",	"S-1-3-2",	nil },
	{ "lg.creator-group-server",	"S-1-3-3",	nil },

	/* default domain users */
	{ "du.admin", 			"S-1-5",	"500" },
	{ "du.guest",			"S-1-5",	"501" },
	{ "du.kerberos",		"S-1-5",	"502" },

	/* default domain groups */
	{ "dg.admins", 			"S-1-5-21",	"512" },
	{ "dg.users",			"S-1-5-21",	"513" },
	{ "dg.guests",			"S-1-5",	"514" },
	{ "dg.computers",		"S-1-5",	"515" },
	{ "dg.controllers",		"S-1-5",	"516" },
	{ "dg.cert-admins",		"S-1-5",	"517" },
	{ "dg.schema-admins",		"S-1-5",	"518" },
	{ "dg.enterprise-admins",	"S-1-5",	"519" },
	{ "dg.group-policy-admins",	"S-1-5",	"520" },
	{ "dg.remote-access",		"S-1-5",	"553" },

	/* default domain aliases */
	{ "da.admins",			"S-1-5",	"544" },
	{ "da.users",			"S-1-5",	"545" },
	{ "da.guests",			"S-1-5",	"546" },
	{ "da.power-users",		"S-1-5",	"547" },
	{ "da.account-operators",	"S-1-5",	"548" },
	{ "da.server-operators",	"S-1-5",	"549" },
	{ "da.print-operators",		"S-1-5",	"550" },
	{ "da.backup-operators",	"S-1-5",	"551" },
	{ "da.replicator",		"S-1-5",	"552" },
	{ "da.RAS-servers",		"S-1-5",	"553" },

};

static char *
sid2name(char *sid)
{
	int i;
	char *rid;

	if(sid == nil || (rid = strrchr(sid, '-')) == nil || *++rid == 0)
		return estrdup9p("-");

	for(i = 0; i < nelem(known); i++){
		if(strcmp(known[i].auth, sid) == 0 && known[i].rid == nil)
			return estrdup9p(known[i].name);

		if(strlen(known[i].auth) < strlen(sid) &&
		    strncmp(known[i].auth, sid, strlen(known[i].auth)) == 0 &&
		    known[i].rid && strcmp(known[i].rid, rid) == 0)
			return estrdup9p(known[i].name);
	}

	return estrdup9p(rid);
}

void
upd_names(Session *s, Share *sp, char *path, Dir *d)
{
	int fh, result;
	char *usid, *gsid;
	FInfo fi;

	if(d->uid)
		free(d->uid);
	if(d->gid)
		free(d->gid);

	if((fh = CIFS_NT_opencreate(s, sp, path, 0, 0, 0, READ_CONTROL,
	    FILE_SHARE_ALL, FILE_OPEN, &result, &fi)) == -1){
		d->uid = estrdup9p("unknown");
		d->gid = estrdup9p("unknown");
		return;
	}
	usid = nil;
	gsid = nil;
	TNTquerysecurity(s, sp, fh, &usid, &gsid);
	d->uid = sid2name(usid);
	d->gid = sid2name(gsid);
	if(fh != -1)
		CIFSclose(s, sp, fh);
}
