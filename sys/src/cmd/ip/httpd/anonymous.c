#include <u.h>
#include <libc.h>
#include "httpd.h"
#include "httpsrv.h"

void
anonymous(HConnect *c)
{
	if(bind(webroot, "/", MREPL) < 0){
		hfail(c, HInternal);
		exits(nil);
	}
	chdir("/");
}
