/* Copyright © 2003 Russ Cox, MIT; see /sys/src/libsunrpc/COPYING */
#include <u.h>
#include <libc.h>
#include <thread.h>
#include <sunrpc.h>
#include <nfs3.h>

int chatty;
SunClient *client;

void
usage(void)
{
	fprint(2, "usage: nfsmount address [cmd]\n"
		"cmd is one of:\n"
		"\tnull\n"
		"\tmnt path\n"
		"\tdump\n"
		"\tumnt path\n"
		"\tumntall\n"
		"\texport (default)\n");
	threadexitsall("usage");
}

void
portCall(SunCall *c, PortCallType type)
{
	c->rpc.prog = PortProgram;
	c->rpc.vers = PortVersion;
	c->rpc.proc = type>>1;
	c->rpc.iscall = !(type&1);
	c->type = type;
}

int
getport(SunClient *client, uint prog, uint vers, uint prot, uint *port)
{
	PortTGetport tx;
	PortRGetport rx;

	memset(&tx, 0, sizeof tx);
	portCall(&tx.call, PortCallTGetport);
	tx.map.prog = prog;
	tx.map.vers = vers;
	tx.map.prot = prot;

	memset(&rx, 0, sizeof rx);
	portCall(&rx.call, PortCallRGetport);

	if(sunClientRpc(client, 0, &tx.call, &rx.call, nil) < 0)
		return -1;
	*port = rx.port;
	return 0;
}

uchar unixauth[] = {
	0x12, 0x23, 0x34, 0x45,	/* stamp */
	0x00, 0x00, 0x00, 0x04,	/* gnot */
	0x67, 0x6e, 0x6f, 0x74,
	0x00, 0x00, 0x03, 0xE9,	/* 1001 */
	0x00, 0x00, 0x03, 0xE9,	/* 1001 */
	0x00, 0x00, 0x00, 0x00,	/* gid list */
};
void
mountCall(SunCall *c, NfsMount3CallType type)
{
	c->rpc.prog = NfsMount3Program;
	c->rpc.vers = NfsMount3Version;
	c->rpc.proc = type>>1;
	c->rpc.iscall = !(type&1);
	if(c->rpc.iscall){
		c->rpc.cred.flavor = SunAuthSys;
		c->rpc.cred.data = unixauth;
		c->rpc.cred.ndata = sizeof unixauth;
	}
	c->type = type;
}

void
tnull(char **argv)
{
	NfsMount3TNull tx;
	NfsMount3RNull rx;

	USED(argv);

	memset(&tx, 0, sizeof tx);
	mountCall(&tx.call, NfsMount3CallTNull);

	memset(&rx, 0, sizeof rx);
	mountCall(&rx.call, NfsMount3CallRNull);

	if(sunClientRpc(client, 0, &tx.call, &rx.call, nil) < 0)
		sysfatal("rpc: %r");
}

void
tmnt(char **argv)
{
	int i;
	NfsMount3TMnt tx;
	NfsMount3RMnt rx;

	memset(&tx, 0, sizeof tx);
	mountCall(&tx.call, NfsMount3CallTMnt);
	tx.path = argv[0];

	memset(&rx, 0, sizeof rx);
	mountCall(&rx.call, NfsMount3CallRMnt);

	if(sunClientRpc(client, 0, &tx.call, &rx.call, nil) < 0)
		sysfatal("rpc: %r");

	if(rx.status != 0){
		nfs3Errstr(rx.status);
		sysfatal("mnt: %r");
	}

	print("handle %.*H\n", rx.len, rx.handle);
	print("auth:");
	for(i=0; i<rx.nauth; i++)
		print(" %ud", (uint)rx.auth[i]);
	print("\n");
}

void
tdump(char **argv)
{
	uchar *p, *ep;
	NfsMount3TDump tx;
	NfsMount3RDump rx;
	NfsMount3Entry e;

	memset(&tx, 0, sizeof tx);
	mountCall(&tx.call, NfsMount3CallTDump);
	USED(argv);

	memset(&rx, 0, sizeof rx);
	mountCall(&rx.call, NfsMount3CallRDump);

	if(sunClientRpc(client, 0, &tx.call, &rx.call, nil) < 0)
		sysfatal("rpc: %r");

	p = rx.data;
	ep = p+rx.count;
	while(p < ep){
		if(nfsMount3EntryUnpack(p, ep, &p, &e) < 0)
			sysfatal("unpack entry structure failed");
		print("%s %s\n", e.host, e.path);
	}
}

void
tumnt(char **argv)
{
	NfsMount3TUmnt tx;
	NfsMount3RUmnt rx;

	memset(&tx, 0, sizeof tx);
	mountCall(&tx.call, NfsMount3CallTUmnt);
	tx.path = argv[0];

	memset(&rx, 0, sizeof rx);
	mountCall(&rx.call, NfsMount3CallRUmnt);

	if(sunClientRpc(client, 0, &tx.call, &rx.call, nil) < 0)
		sysfatal("rpc: %r");

	print("\n");
}

void
tumntall(char **argv)
{
	NfsMount3TUmntall tx;
	NfsMount3RUmntall rx;

	memset(&tx, 0, sizeof tx);
	mountCall(&tx.call, NfsMount3CallTUmntall);
	USED(argv);

	memset(&rx, 0, sizeof rx);
	mountCall(&rx.call, NfsMount3CallRUmntall);

	if(sunClientRpc(client, 0, &tx.call, &rx.call, nil) < 0)
		sysfatal("rpc: %r");

	print("\n");
}

void
texport(char **argv)
{
	uchar *p, *ep, *tofree;
	char **g, **gg;
	int ng, i, n;
	NfsMount3TDump tx;
	NfsMount3RDump rx;
	NfsMount3Export e;

	memset(&tx, 0, sizeof tx);
	mountCall(&tx.call, NfsMount3CallTExport);
	USED(argv);

	memset(&rx, 0, sizeof rx);
	mountCall(&rx.call, NfsMount3CallRExport);

	if(sunClientRpc(client, 0, &tx.call, &rx.call, &tofree) < 0)
		sysfatal("rpc: %r");

	p = rx.data;
	ep = p+rx.count;
	g = nil;
	ng = 0;
	while(p < ep){
		n = nfsMount3ExportGroupSize(p);
		if(n > ng){
			ng = n;
			g = erealloc(g, sizeof(g[0])*ng);
		}
		if(nfsMount3ExportUnpack(p, ep, &p, g, &gg, &e) < 0)
			sysfatal("unpack export structure failed");
		print("%s", e.path);
		for(i=0; i<e.ng; i++)
			print(" %s", e.g[i]);
		print("\n");
	}
	free(tofree);
}

static struct {
	char *cmd;
	int narg;
	void (*fn)(char**);
} tab[] = {
	"null",	0,	tnull,
	"mnt",	1,	tmnt,
	"dump",	0,	tdump,
	"umnt",	1,	tumnt,
	"umntall",	1,	tumntall,
	"export",	0,	texport,
};

char*
netchangeport(char *addr, char *port)
{
	static char buf[256];
	char *r;

	strecpy(buf, buf+sizeof buf, addr);
	r = strrchr(buf, '!');
	if(r == nil)
		return nil;
	r++;
	strecpy(r, buf+sizeof buf, port);
	return buf;
}

void
threadmain(int argc, char **argv)
{
	char *dflt[] = { "export", };
	char *addr, *cmd;
	int i, proto;
	uint port;
	char buf[32];
	int mapit;

	mapit = 1;
	ARGBEGIN{
	case 'R':
		chatty++;
		break;
	case 'm':
		mapit = 0;
		break;
	}ARGEND

	if(argc < 1)
		usage();

	fmtinstall('B', sunRpcFmt);
	fmtinstall('C', sunCallFmt);
	fmtinstall('H', encodefmt);
	sunFmtInstall(&portProg);
	sunFmtInstall(&nfsMount3Prog);

	addr = netmkaddr(argv[0], "udp", "portmap");
	if(mapit){
		/* translate with port mapper */
		fprint(2, "connecting to %s\n", addr);
		if((client = sunDial(addr)) == nil)
			sysfatal("dial %s: %r", addr);
		client->chatty = chatty;
		sunClientProg(client, &portProg);
		if(strstr(addr, "udp!"))
			proto = PortProtoUdp;
		else
			proto = PortProtoTcp;
		if(getport(client, NfsMount3Program, NfsMount3Version, proto, &port) < 0)
			sysfatal("getport: %r");
		snprint(buf, sizeof buf, "%ud!r", port);
		addr = netchangeport(addr, buf);
		sunClientClose(client);
	}

	fprint(2, "connecting to %s\n", addr);
	if((client = sunDial(addr)) == nil)
		sysfatal("dial %s: %r", addr);

	client->chatty = chatty;
	sunClientProg(client, &nfsMount3Prog);

	argv++;
	argc--;

	if(argc == 0){
		argc = 1;
		argv = dflt;
	}
	cmd = argv[0];
	argv++;
	argc--;

	for(i=0; i<nelem(tab); i++){
		if(strcmp(tab[i].cmd, cmd) == 0){
			if(tab[i].narg != argc)
				usage();
			(*tab[i].fn)(argv);
			threadexitsall(nil);
		}
	}
	usage();
}
