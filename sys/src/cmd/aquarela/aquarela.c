#include "headers.h"
#include <pool.h>

static void
disconnecttree(void *magic, void *arg)
{
	smbtreedisconnect((SmbSession *)magic, (SmbTree *)arg);
}

static void
closesearch(void *magic, void *arg)
{
	smbsearchclose((SmbSession *)magic, (SmbSearch *)arg);
}

static void
smbsessionfree(SmbSession *s)
{
	if (s) {
		smbidmapfree(&s->tidmap, disconnecttree, s);
		smbidmapfree(&s->sidmap, closesearch, s);
		smbbufferfree(&s->response);
		free(s->client.accountname);
		free(s->client.primarydomain);
		free(s->client.nativeos);
		free(s->client.nativelanman);
		free(s->transaction.in.parameters);
		free(s->transaction.in.data);
		free(s->transaction.in.setup);
		free(s->transaction.in.name);
		smbbufferfree(&s->transaction.out.parameters);
		smbbufferfree(&s->transaction.out.data);
		auth_freechal(s->cs);
		free(s);
	}
}

int
smbsessionwrite(SmbSession *smbs, void *p, long n)
{
	SmbHeader h;
	SmbOpTableEntry *ote;
	uchar *pdata;
	int rv;
	SmbBuffer *b = nil;
	ushort bytecount;
	SmbProcessResult pr;

	if (smbs->response == nil)
		smbs->response = smbbuffernew(576);
	else
		smbresponsereset(smbs);
	smbs->errclass = SUCCESS;
	smbs->error = SUCCESS;
//	print("received %ld bytes\n", n);
	if (n <= 0)
		goto closedown;
	b = smbbufferinit(p, p, n);
	if (!smbbuffergetheader(b, &h, &pdata, &bytecount)) {
		smblogprint(-1, "smb: invalid header\n");
		goto closedown;
	}
smbloglock();
smblogprint(h.command, "received:\n");
smblogdata(h.command, smblogprint, p, n, 0x1000);
smblogunlock();
	ote = smboptable + h.command;
	if (ote->name == nil) {
		smblogprint(-1, "smb: illegal opcode 0x%.2ux\n", h.command);
		goto unimp;
	}
	if (ote->process == nil) {
		smblogprint(-1, "smb: opcode %s unimplemented\n", ote->name);
		goto unimp;
	}
	if (smbs->nextcommand != SMB_COM_NO_ANDX_COMMAND
		&& smbs->nextcommand != h.command) {
		smblogprint(-1, "smb: wrong command - expected %.2ux\n", smbs->nextcommand);
		goto misc;
	}
	smbs->nextcommand = SMB_COM_NO_ANDX_COMMAND;
	switch (h.command) {
	case SMB_COM_NEGOTIATE:
	case SMB_COM_SESSION_SETUP_ANDX:
	case SMB_COM_TREE_CONNECT_ANDX:
	case SMB_COM_ECHO:
		break;
	default:
		if (smbs->state != SmbSessionEstablished) {
			smblogprint(-1, "aquarela: command %.2ux unexpected\n", h.command);
			goto unimp;
		}
	}
	pr = (*ote->process)(smbs, &h, pdata, b);
	switch (pr) {
	case SmbProcessResultUnimp:
	unimp:
		smbseterror(smbs, ERRDOS, ERRunsup);
		pr = SmbProcessResultError;
		break;
	case SmbProcessResultFormat:
		smbseterror(smbs, ERRSRV, ERRsmbcmd);
		pr = SmbProcessResultError;
		break;
	case SmbProcessResultMisc:
	misc:
		smbseterror(smbs, ERRSRV, ERRerror);
		pr = SmbProcessResultError;
		break;
	}
	if (pr == SmbProcessResultError) {
		smblogprint(h.command, "reply: error %d/%d\n", smbs->errclass, smbs->error);
		if (!smbresponseputerror(smbs, &h, smbs->errclass, smbs->error))
			pr = SmbProcessResultDie;
		else
			pr = SmbProcessResultReply;
	}
	else
		smblogprint(h.command, "reply: ok\n");
	if (pr == SmbProcessResultReply)
		rv = smbresponsesend(smbs) == SmbProcessResultOk ? 0 : -1;
	else if (pr == SmbProcessResultDie)
		rv = -1;
	else
		rv = 0;
	goto done;
closedown:
	rv = -1;
done:
	if (rv < 0) {
		smblogprintif(smbglobals.log.sessions, "shutting down\n");
		smbsessionfree(smbs);
	}
	smbbufferfree(&b);
	if (smbglobals.log.poolparanoia)
		poolcheck(mainmem);
	return rv;
}

static int
nbwrite(NbSession *nbss, void *p, long n)
{
	return smbsessionwrite((SmbSession *)nbss->magic, p, n);
}

static int
cifswrite(SmbCifsSession *cifs, void *p, long n)
{
	return smbsessionwrite((SmbSession *)cifs->magic, p, n);
}

int
nbssaccept(void *, NbSession *s, NBSSWRITEFN **writep)
{
	SmbSession *smbs = smbemallocz(sizeof(SmbSession), 1);
	smbs->nbss = s;
	s->magic = smbs;
	smbs->nextcommand = SMB_COM_NO_ANDX_COMMAND;
	*writep = nbwrite;
	smblogprintif(smbglobals.log.sessions, "netbios session started\n");
	return 1;
}

int
cifsaccept(SmbCifsSession *s, SMBCIFSWRITEFN **writep)
{
	SmbSession *smbs = smbemallocz(sizeof(SmbSession), 1);
	smbs->cifss = s;
	s->magic = smbs;
	smbs->nextcommand = SMB_COM_NO_ANDX_COMMAND;
	*writep = cifswrite;
	smblogprintif(smbglobals.log.sessions, "cifs session started\n");
	return 1;
}

void
usage(void)
{
	fprint(2, "usage: %s [-np] [-d debug] [-u N] [-w workgroup]\n", argv0);
	threadexitsall("usage");
}

static void
logset(char *cmd)
{
	int x;
	if (strcmp(cmd, "allcmds") == 0) {
		for (x = 0; x < 256; x++)
			smboptable[x].debug = 1;
		for (x = 0; x < smbtrans2optablesize; x++)
			smbtrans2optable[x].debug = 1;
		return;
	}
	if (strcmp(cmd, "tids") == 0) {
		smbglobals.log.tids = 1;
		return;
	}
	if (strcmp(cmd, "sids") == 0) {
		smbglobals.log.sids = 1;
		return;
	}
	if (strcmp(cmd, "fids") == 0) {
		smbglobals.log.fids = 1;
		return;
	}
	if (strcmp(cmd, "rap2") == 0) {
		smbglobals.log.rap2 = 1;
		return;
	}
	else if (strcmp(cmd, "find") == 0) {
		smbglobals.log.find = 1;
		return;
	}
	if (strcmp(cmd, "query") == 0) {
		smbglobals.log.query = 1;
		return;
	}
	if (strcmp(cmd, "sharedfiles") == 0) {
		smbglobals.log.sharedfiles = 1;
		return;
	}
	if (strcmp(cmd, "poolparanoia") == 0) {
		mainmem->flags |= POOL_PARANOIA;
		smbglobals.log.poolparanoia = 1;
		return;
	}
	if (strcmp(cmd, "sessions") == 0) {
		smbglobals.log.sessions = 1;
		return;
	}
	if (strcmp(cmd, "rep") == 0) {
		smbglobals.log.rep = 1;
		return;
	}
	if (strcmp(cmd, "locks") == 0) {
		smbglobals.log.locks = 1;
		return;
	}

	for (x = 0; x < 256; x++)
		if (smboptable[x].name && strcmp(smboptable[x].name, cmd) == 0) {
			smboptable[x].debug = 1;
			return;
		}
	for (x = 0; x < smbtrans2optablesize; x++)
		if (smbtrans2optable[x].name && strcmp(smbtrans2optable[x].name, cmd) == 0) {
			smbtrans2optable[x].debug = 1;
			return;
		}
	if (strlen(cmd) == 4 && cmd[0] == '0' && cmd[1] == 'x') {
		int c;
		c = strtoul(cmd + 2, 0, 16);
		if (c >= 0 && c <= 255) {
			smboptable[c].debug = 1;
			return;
		}
	}
	print("debugging command %s not recognised\n", cmd);
}


void
threadmain(int argc, char **argv)
{
	NbName from, to;
	char *e = nil;
	int netbios = 0;
	ARGBEGIN {
	case 'u':
		smbglobals.unicode = strtol(ARGF(), 0, 0) != 0;
		break;
	case 'p':
		smbglobals.log.print = 1;
		break;
	case 'd':
		logset(ARGF());
		break;
	case 'w':
		smbglobals.primarydomain = ARGF();
		break;
	case 'n':
		netbios = 1;
		break;
	default:
		usage();
	} ARGEND;
	smbglobalsguess(0);
	smblistencifs(cifsaccept);
	if (netbios) {
		nbinit();
		nbmknamefromstring(from, "*");
		nbmknamefromstring(to, "*smbserver\\x20");
		nbsslisten(to, from, nbssaccept, nil);
		nbmknamefromstringandtype(to, smbglobals.serverinfo.name, 0x20);
		nbsslisten(to, from, nbssaccept, nil);
	}
	smblogprint(-1, "Aquarela %d.%d running\n", smbglobals.serverinfo.vmaj, smbglobals.serverinfo.vmin);
	for (;;) {
		if (netbios&& !smbbrowsesendhostannouncement(smbglobals.serverinfo.name, 60 * 1000,
			SV_TYPE_SERVER,
			smbglobals.serverinfo.remark, &e)) {
			smblogprint(-1, "hostannounce failed: %s\n", e);
		}
		if (sleep(60 * 1000) < 0)
			break;
	}
}

