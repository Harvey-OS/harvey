#include "agent.h"

int mainstacksize = 4096;
Tree *tree;
int verbose;

Srv fsrv = {
	.open=	fsopen,
	.read=	fsread,
	.write=	fswrite,
	.attach=	fsattach,
	.clunkaux=	fsclunkaux,
	.flush=	fsflush,
};

void
usage(void)
{
	fprint(2, "usage: agent [-m mtpt]\n");
	exits("usage");
}

void
threadmain(int argc, char **argv)
{
	char *mtpt;
	Config *c;
	String *s;

	mtpt = "/mnt/auth";
	ARGBEGIN{
	default:
		usage();
	case 'D':
		lib9p_chatty++;
		break;
	case 'm':
		mtpt = EARGF(usage());
		break;
	}ARGEND;

	user = getuser();

	s = s_reset(nil);
	c = strtoconfig(nil, s);
	if(c == nil)
		sysfatal("whoops: %r");

	tree = mktree(nil, nil, CHDIR|0555);
	setconfig(c);
	fsrv.tree = tree;
	threadnonotes();
	threadpostmountsrv(&fsrv, nil, mtpt, MREPL);
}
