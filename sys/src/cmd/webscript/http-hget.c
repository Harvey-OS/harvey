#include <u.h>
#include <libc.h>
#include <bio.h>
#include <draw.h>
#include <regexp.h>
#include <html.h>
#include <ctype.h>
#include "dat.h"

Bytes*
httpget(char *url, char *post)
{
	int p[2], n;
	Waitmsg *w;
	Bytes *b;
	char buf[4096];
	
	if(verbose){
		fprint(2, "%s %s\n", post ? "post" : "get", url);
		if(post)
			fprint(2, "%s\n", post);
	}
	
	if(pipe(p) < 0)
		sysfatal("pipe: %r");
	switch(fork()){
	case -1:
		sysfatal("fork: %r");
	case 0:
		close(p[0]);
		dup(p[1], 1);
		if(p[1] != 1)
			close(p[1]);
		if(access(url, AEXIST) >= 0)
			execl("/bin/cat", "cat", url, nil);
		else{
			if(post)
				execl("/bin/hget", "hget", "-p", post, url, nil);
			else
				execl("/bin/hget", "hget", url, nil);
		}
		_exits("execl");
	default:
		close(p[1]);
		b = emalloc(sizeof(Bytes));
		while((n = read(p[0], buf, sizeof buf)) > 0)
			growbytes(b, buf, n);
		close(p[0]);
		w = wait();
		if(w == nil)
			sysfatal("wait: %r");
		if(w->msg[0])
			sysfatal("hget: %s", w->msg);
		free(w);
		return b;
	}
}

