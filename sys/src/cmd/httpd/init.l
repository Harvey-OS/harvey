#include <alef.h>
#include <bio.h>
#include "httpd.h"

void
usage()
{
	fprint(2, "usage: httpd [-n namespace] [-d domain] [-r remoteip] method version uri [search]\n");
	exits("usage");
}

(byte*, byte*, byte*, byte*)
init(int argc, byte **argv)
{
	Arg *arg;
	int c;

	ALEFcheck = checkfail;
	bout.init(1, OWRITE);
	namespace = nil;
	mydomain = nil;
	remotesys = nil;
	arg = arginit(argc, argv);
	fmtinstall('D', dateconv);
	fmtinstall('H', httpconv);
	while(c = argopt(arg)){
		switch(c){
		case 'd':
			mydomain = argf(arg);
			break;
		case 'n':
			namespace = argf(arg);
			break;
		case 'r':
			remotesys = argf(arg);
			break;
		default:
			usage();
		}
	}

	if(namespace == nil)
		namespace = "/lib/namespace.http";
	if(remotesys == nil)
		remotesys = "unknown";
	if(mydomain == nil)
		mydomain = "unknown";

	/*
	 * open all files we might need before castrating namespace
	 */
	time();
	syslog(0, HTTPLOG, nil);

	if(arg->ac != 4 && arg->ac != 3)
		usage();

	contentinit();

	return (arg->av[0], arg->av[1], arg->av[2], arg->av[3]);
}
