#include "all.h"

static void	pcinit(int, char**);
static int	pcnull(int, Rpccall*, Rpccall*);
static int	pcinfo(int, Rpccall*, Rpccall*);
static int	pcauth(int, Rpccall*, Rpccall*);
static int	pcfacilities[15];
static char	no_comment[] = "Trust me.";
static char	pc_vers[] = "@(#)pcnfsd_v2.c	1.6 - rpc.pcnfsd V2.0 (c) 1994 P9, GmbH";
static char	pc_home[] = "merrimack:/";

static Procmap pcproc[] = {
	0, pcnull,
	1, pcinfo,
	13, pcauth,
	0, 0
};

int	myport = 1111;

Progmap progmap[] = {
	150001, 2, pcinit, pcproc,
	0, 0, 0,
};

void
pcinit(int argc, char **argv)
{
	Procmap *p;
	int i;

	USED(argc, argv);
	clog("pc init\n");

	for(i=0; i<nelem(pcfacilities); i++)
		pcfacilities[i] = -1;
	for(p=pcproc; p->procp; p++)
		pcfacilities[p->procno] = 100;
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

	chat("host=%I, port=%d: pcinfo...",
		cmd->host, cmd->port);
	if(n <= 16)
		return garbage(reply, "count too small");
	argptr += string2S(argptr, &vers);
	argptr += string2S(argptr, &cm);
	if(argptr != &((uchar *)cmd->args)[n])
		return garbage(reply, "bad count");
	chat("\"%.*s\",\"%.*s\"\n", vers.n, vers.s, cm.n, cm.s);
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
pcauth(int n, Rpccall *cmd, Rpccall *reply)
{
	uchar *argptr = cmd->args;
	uchar *dataptr = reply->results;
	String sys, id, pw, cm;

	chat("host=%I, port=%d: pcauth...",
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
	chat("\"%.*s\",\"%.*s\",\"%.*s\",\"%.*s\"\n", sys.n, sys.s,
		id.n, id.s, pw.n, pw.s, cm.n, cm.s);
	PLONG(0);	/* status - OK */
	PLONG(1);	/* uid */
	PLONG(1);	/* gid */
	PLONG(0);	/* ngids */
	PLONG(sizeof(pc_home)-1);
	PPTR(pc_home, sizeof(pc_home)-1);
	PLONG(0);	/* umask */
	PLONG(sizeof(no_comment)-1);
	PPTR(no_comment, sizeof(no_comment)-1);
	return dataptr - (uchar *)reply->results;
}
