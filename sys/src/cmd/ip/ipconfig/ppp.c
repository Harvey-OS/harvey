#include <u.h>
#include <libc.h>
#include <ip.h>
#include <bio.h>
#include <ndb.h>
#include "../dhcp.h"
#include "ipconfig.h"

void
pppbinddev(void)
{
	int ac, pid;
	char *av[12];
	Waitmsg *w;

	/* ppp does the binding */

	/* start with an empty config file */
	if(nip == 0)
		writendb("", 0, 0);

	switch(pid = rfork(RFPROC|RFFDG|RFMEM)){
	case -1:
		sysfatal("can't start ppp: %r");
	case 0:
		ac = 0;
		av[ac++] = "ppp";
		av[ac++] = "-uf";
		av[ac++] = "-p";
		av[ac++] = conf.dev;
		av[ac++] = "-x";
		av[ac++] = conf.mpoint;
		if(conf.baud != nil){
			av[ac++] = "-b";
			av[ac++] = conf.baud;
		}
		av[ac] = nil;
		exec("/bin/ip/ppp", av);
		exec("/ppp", av);
		sysfatal("execing /ppp: %r");
	}

	/* wait for ppp to finish connecting and configuring */
	while((w = wait()) != nil){
		if(w->pid == pid){
			if(w->msg[0] != 0)
				sysfatal("/ppp exited with status: %s", w->msg);
			free(w);
			break;
		}
		free(w);
	}
	if(w == nil)
		sysfatal("/ppp disappeared");

	/* ppp sets up the configuration itself */
	noconfig = 1;
	getndb();
}

