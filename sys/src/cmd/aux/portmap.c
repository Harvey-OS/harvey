/* Copyright © 2003 Russ Cox, MIT; see /sys/src/libsunrpc/COPYING */
#include <u.h>
#include <libc.h>
#include <thread.h>
#include <sunrpc.h>

int chatty;
SunClient *client;

void
usage(void)
{
	fprint(2, "usage: portmap address [cmd]\n"
		"cmd is one of:\n"
		"\tnull\n"
		"\tset prog vers proto port\n"
		"\tunset prog vers proto port\n"
		"\tgetport prog vers proto\n"
		"\tdump (default)\n");
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

void
tnull(char **argv)
{
	PortTNull tx;
	PortRNull rx;

	USED(argv);

	memset(&tx, 0, sizeof tx);
	portCall(&tx.call, PortCallTNull);

	memset(&rx, 0, sizeof rx);
	portCall(&rx.call, PortCallRNull);

	if(sunClientRpc(client, 0, &tx.call, &rx.call, nil) < 0)
		sysfatal("rpc: %r");
}

void
tset(char **argv)
{
	PortTSet tx;
	PortRSet rx;

	memset(&tx, 0, sizeof tx);
	portCall(&tx.call, PortCallTSet);
	tx.map.prog = strtol(argv[0], 0, 0);
	tx.map.vers = strtol(argv[1], 0, 0);
	tx.map.prot = strtol(argv[2], 0, 0);
	tx.map.port = strtol(argv[3], 0, 0);

	memset(&rx, 0, sizeof rx);
	portCall(&rx.call, PortCallRSet);

	if(sunClientRpc(client, 0, &tx.call, &rx.call, nil) < 0)
		sysfatal("rpc: %r");

	if(rx.b == 0)
		print("rejected\n");
}

void
tunset(char **argv)
{
	PortTUnset tx;
	PortRUnset rx;

	memset(&tx, 0, sizeof tx);
	portCall(&tx.call, PortCallTUnset);
	tx.map.prog = strtol(argv[0], 0, 0);
	tx.map.vers = strtol(argv[1], 0, 0);
	tx.map.prot = strtol(argv[2], 0, 0);
	tx.map.port = strtol(argv[3], 0, 0);

	memset(&rx, 0, sizeof rx);
	portCall(&rx.call, PortCallRUnset);

	if(sunClientRpc(client, 0, &tx.call, &rx.call, nil) < 0)
		sysfatal("rpc: %r");

	if(rx.b == 0)
		print("rejected\n");
}

void
tgetport(char **argv)
{
	PortTGetport tx;
	PortRGetport rx;

	memset(&tx, 0, sizeof tx);
	portCall(&tx.call, PortCallTGetport);
	tx.map.prog = strtol(argv[0], 0, 0);
	tx.map.vers = strtol(argv[1], 0, 0);
	tx.map.prot = strtol(argv[2], 0, 0);

	memset(&rx, 0, sizeof rx);
	portCall(&rx.call, PortCallRGetport);

	if(sunClientRpc(client, 0, &tx.call, &rx.call, nil) < 0)
		sysfatal("rpc: %r");

	print("%d\n", rx.port);
}

void
tdump(char **argv)
{
	int i;
	uchar *p;
	PortTDump tx;
	PortRDump rx;
	PortMap *m;

	USED(argv);

	memset(&tx, 0, sizeof tx);
	portCall(&tx.call, PortCallTDump);

	memset(&rx, 0, sizeof rx);
	portCall(&rx.call, PortCallRDump);

	if(sunClientRpc(client, 0, &tx.call, &rx.call, &p) < 0)
		sysfatal("rpc: %r");

	for(i=0, m=rx.map; i<rx.nmap; i++, m++)
		print("%ud %ud %ud %ud\n", (uint)m->prog, (uint)m->vers, (uint)m->prot, (uint)m->port);

	free(p);
}

static struct {
	char *cmd;
	int narg;
	void (*fn)(char**);
} tab[] = {
	"null",	0,	tnull,
	"set",	4,	tset,
	"unset",	4,	tunset,
	"getport",	3,	tgetport,
	"dump",	0,	tdump,
};

void
threadmain(int argc, char **argv)
{
	char *dflt[] = { "dump", };
	char *addr, *cmd;
	int i;

	ARGBEGIN{
	case 'R':
		chatty++;
		break;
	}ARGEND

	if(argc < 1)
		usage();

	fmtinstall('B', sunRpcFmt);
	fmtinstall('C', sunCallFmt);
	fmtinstall('H', encodefmt);
	sunFmtInstall(&portProg);

	addr = netmkaddr(argv[0], "udp", "portmap");
	if((client = sunDial(addr)) == nil)
		sysfatal("dial %s: %r", addr);

	client->chatty = chatty;
	sunClientProg(client, &portProg);

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
