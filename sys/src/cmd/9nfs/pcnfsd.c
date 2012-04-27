#include "all.h"

static void	pcinit(int, char**);
static int	pcnull(int, Rpccall*, Rpccall*);
static int	pcinfo(int, Rpccall*, Rpccall*);
static int	pcauth(int, Rpccall*, Rpccall*);
static int	pc1auth(int, Rpccall*, Rpccall*);
static int	pcfacilities[15];
static char	no_comment[] = "Trust me.";
static char	pc_vers[] = "@(#)pcnfsd_v2.c	1.6 - rpc.pcnfsd V2.0 (c) 1994 P9, GmbH";
static char	pc_home[] = "merrimack:/";

static Procmap pcproc[] = {	/* pcnfsd v2 */
	0, pcnull,
	1, pcinfo,
	13, pcauth,
	0, 0
};

static Procmap pc1proc[] = { 	/* pc-nfsd v1 */
	0, pcnull,
	1, pc1auth,
	0, 0
};

int	myport = 1111;

Progmap progmap[] = {
	150001, 2, pcinit, pcproc,
	150001, 1, 0, pc1proc,
	0, 0, 0,
};

void
main(int argc, char *argv[])
{
	server(argc, argv, myport, progmap);
}

static void
pcinit(int argc, char **argv)
{
	Procmap *p;
	int i;
	char *config = "config";

	ARGBEGIN{
	case 'c':
		config = ARGF();
		break;
	default:
		if(argopt(ARGC()) < 0)
			sysfatal("usage: %s %s [-c config]", argv0, commonopts);
		break;
	}ARGEND;
	clog("pc init\n");

	for(i=0; i<nelem(pcfacilities); i++)
		pcfacilities[i] = -1;
	for(p=pcproc; p->procp; p++)
		pcfacilities[p->procno] = 100;
	readunixidmaps(config);
}

static int
pcnull(int n, Rpccall *cmd, Rpccall *reply)
{
	USED(n, cmd, reply);
	return 0;
}

static void
scramble(String *x)
{
	int i;

	for(i=0; i<x->n; i++)
		x->s[i] = (x->s[i] ^ 0x5b) & 0x7f;
}

static int
pcinfo(int n, Rpccall *cmd, Rpccall *reply)
{
	uchar *argptr = cmd->args;
	uchar *dataptr = reply->results;
	String vers, cm;
	int i;

	chat("host=%I, port=%ld: pcinfo...",
		cmd->host, cmd->port);
	if(n <= 16)
		return garbage(reply, "count too small");
	argptr += string2S(argptr, &vers);
	argptr += string2S(argptr, &cm);
	if(argptr != &((uchar *)cmd->args)[n])
		return garbage(reply, "bad count");
	chat("\"%.*s\",\"%.*s\"\n", utfnlen(vers.s, vers.n), vers.s, utfnlen(cm.s, cm.n), cm.s);
	PLONG(sizeof(pc_vers)-1);
	PPTR(pc_vers, sizeof(pc_vers)-1);
	PLONG(sizeof(no_comment)-1);
	PPTR(no_comment, sizeof(no_comment)-1);
	PLONG(nelem(pcfacilities));
	for(i=0; i<nelem(pcfacilities); i++)
		PLONG(pcfacilities[i]);
	return dataptr - (uchar *)reply->results;
}

static int
pc1auth(int n, Rpccall *cmd, Rpccall *reply)
{
	uchar *argptr = cmd->args;
	uchar *dataptr = reply->results;
	String id, pw;
	Unixidmap *m;
	int uid;

	chat("host=%I, port=%ld: pcauth...",
		cmd->host, cmd->port);
	if(n <= 8)
		return garbage(reply, "count too small");
	argptr += string2S(argptr, &id);
	argptr += string2S(argptr, &pw);
	if(argptr != &((uchar*)cmd->args)[n])
		return garbage(reply, "bad count");
	scramble(&id);
	scramble(&pw);
	m = pair2idmap("pcnfsd", cmd->host);
	uid = -1;
	if(m)
		uid = name2id(&m->u.ids, id.s);
	if(uid < 0)
		uid = 1;
	chat("\"%.*s\",\"%.*s\" uid=%d\n", utfnlen(id.s, id.n), id.s, utfnlen(pw.s, pw.n), pw.s, uid);
	PLONG(0);	/* status */
	PLONG(uid);	/* uid */
	PLONG(uid);	/* gid */
	return dataptr - (uchar*)reply->results;
}

static int
pcauth(int n, Rpccall *cmd, Rpccall *reply)
{
	uchar *argptr = cmd->args;
	uchar *dataptr = reply->results;
	String sys, id, pw, cm;
	Unixidmap *m;
	int uid;

	chat("host=%I, port=%ld: pcauth...",
		cmd->host, cmd->port);
	if(n <= 16)
		return garbage(reply, "count too small");
	argptr += string2S(argptr, &sys);
	argptr += string2S(argptr, &id);
	argptr += string2S(argptr, &pw);
	argptr += string2S(argptr, &cm);
	if(argptr != &((uchar *)cmd->args)[n])
		return garbage(reply, "bad count");
	scramble(&id);
	scramble(&pw);

	m = pair2idmap("pcnfsd", cmd->host);
	uid = -1;
	if(m)
		uid = name2id(&m->u.ids, id.s);
	if(uid < 0)
		uid = 1;
	chat("\"%.*s\",\"%.*s\",\"%.*s\",\"%.*s\"\n", utfnlen(sys.s, sys.n), sys.s,
		utfnlen(id.s, id.n), id.s, utfnlen(pw.s, pw.n), pw.s, utfnlen(cm.s, cm.n), cm.s);
	PLONG(0);	/* status - OK */
	PLONG(uid);
	PLONG(uid);	/* gid */
	PLONG(0);	/* ngids */
	PLONG(sizeof(pc_home)-1);
	PPTR(pc_home, sizeof(pc_home)-1);
	PLONG(0);	/* umask */
	PLONG(sizeof(no_comment)-1);
	PPTR(no_comment, sizeof(no_comment)-1);
	return dataptr - (uchar *)reply->results;
}
