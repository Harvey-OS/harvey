#include <u.h>
#include <libc.h>
#include <auth.h>

void
usage(void)
{
	fprint(2, "usage: auth/challenge 'params'\n");
	exits("usage");
}

void
main(int argc, char **argv)
{
	char buf[128], bufu[128];
	int afd, n;
	AuthInfo *ai;
	AuthRpc *rpc;
	Chalstate *c;

	ARGBEGIN{
	default:
		usage();
	}ARGEND

	if(argc != 1)
		usage();

	if((afd = open("/mnt/factotum/rpc", ORDWR)) < 0)
		sysfatal("open /mnt/factotum/rpc: %r");

	rpc = auth_allocrpc(afd);
	if(rpc == nil)
		sysfatal("auth_allocrpc: %r");

	if((c = auth_challenge("%s", argv[0])) == nil)
		sysfatal("auth_challenge: %r");

	print("challenge: %s\n", c->chal);
	print("user:");
	n = read(0, bufu, sizeof bufu);
	if(n > 0){
		bufu[n-1] = '\0';
		c->user = bufu;
	}

	print("response: ");
	n = read(0, buf, sizeof buf);
	if(n < 0)
		sysfatal("read: %r");
	if(n == 0)
		exits(nil);
	c->nresp = n-1;
	c->resp = buf;
	if((ai = auth_response(c)) == nil)
		sysfatal("auth_response: %r");

	print("%s %s\n", ai->cuid, ai->suid);
}
