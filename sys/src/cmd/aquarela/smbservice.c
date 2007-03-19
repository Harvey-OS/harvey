#include "headers.h"

static SmbService local = {
	.name = "local",
	.type = "A:",
	.stype = STYPE_DISKTREE,
	.remark = "The standard namespace",
	.path = "/n/local",
};

static SmbService ipc = {
	.name = "IPC$",
	.type = "IPC",
	.stype = STYPE_IPC,
	.remark = "The aquarela IPC service",
	.path = nil,
	.next = &local,
};

SmbService *smbservices = &ipc;

static int
run9fs(char *arg)
{
	int rv;
	Waitmsg *w;

	rv = fork();
	if (rv < 0)
		return -1;
	if (rv == 0) {
		char *argv[3];
		argv[0] = "/rc/bin/9fs";
		argv[1] = arg;
		argv[2] = 0;
		exec(argv[0], argv);
		exits("failed to exec 9fs");
	}
	for (;;) {
		w = wait();
		if (w == nil)
			return -1;
		if (w->pid == rv)
			break;
		free(w);
	}
	if (w->msg[0]) {
		smblogprint(SMB_COM_TREE_CONNECT_ANDX, "smbservicefind: %s\n", w->msg);
		free(w);
		return -1;
	}
	free(w);
	smblogprint(SMB_COM_TREE_CONNECT_ANDX, "smbservicefind: 9fs %s executed successfully\n", arg);
	return 0;
}

SmbService *
smbservicefind(SmbSession *s, char *uncpath, char *servicetype, uchar *errclassp, ushort *errorp)
{
	char *p, *q;
	if ((uncpath[0] == '/' && uncpath[1] == '/')
	||  (uncpath[0] == '\\' && uncpath[1] == '\\')) {
		/* check that the server name matches mine */
		p = uncpath + 2;
		q = strchr(p, uncpath[0]);
		if (q == nil)
			goto bad;
		*q++ = 0;
//		if (cistrcmp(p, smbglobals.serverinfo.name) != 0)
//			goto bad;
	}
	else
		q = uncpath + 1;
	if (strcmp(servicetype, "?????") == 0 && strcmp(q, "IPC$") == 0)
		return &ipc;
	if ((strcmp(servicetype, "?????") == 0 || strcmp(servicetype, "A:") == 0)) {
		SmbService *serv;
		if (cistrcmp(q, local.name) == 0)
			return &local;
		/* try the session specific list */
		for (serv = s->serv; serv; serv = serv->next)
			if (cistrcmp(q, serv->name) == 0)
				return serv;
		/* exec "9fs q" in case it invents /n/q */
		for (p = q; *p; p++)
			if (*p >= 'A' && *p <= 'Z')
				*p = tolower(*p);
		if (run9fs(q) >= 0) {
			serv = smbemallocz(sizeof(*serv), 1);
			serv->name = smbestrdup(q);
			serv->type = smbestrdup("A:");
			serv->stype = STYPE_DISKTREE;
			smbstringprint(&serv->remark, "9fs %s", q);
			smbstringprint(&serv->path, "/n/%s", q);
			serv->next = s->serv;
			s->serv = serv;
			return serv;
		}
	}
bad:
	*errclassp = ERRDOS;
	*errorp = ERRbadpath;
	return nil;
}

void
smbserviceget(SmbService *serv)
{
	incref(serv);
}

void
smbserviceput(SmbService *serv)
{
	decref(serv);
}
