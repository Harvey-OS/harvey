#include <u.h>
#include <libc.h>
#include <libg.h>
#include <fb.h>
#include <panel.h>
#include "mothra.h"
void httpheader(Url *, char *);
/*
 * Given a url, return a file descriptor on which caller can
 * read a gopher document.
 */
int gopher(Url *url){
	int pfd[2];
	char port[30];
	if(pipe(pfd)==-1) return -1;
	switch(rfork(RFFDG|RFPROC|RFNOWAIT)){
	case -1:
		close(pfd[0]);
		close(pfd[1]);
		return -1;
	case 0:
		dup(pfd[1], 1);
		close(pfd[0]);
		close(pfd[1]);
		sprint(port, "%d", url->port);
		execl("/bin/aux/gopher2html",
			"gopher2html", url->ipaddr, port, url->reltext+1, 0);
		fprint(2, "Can't exec aux/gopher2html!\n");
		print("<head><title>Mothra error</title></head>\n");
		print("<body><h1>Mothra error</h1>\n");
		print("Can't exec aux/gopher2html!</body>\n");
		exits("no exec");
	default:
		close(pfd[1]);
		url->type=HTML;
		return pfd[0];
	}
}
