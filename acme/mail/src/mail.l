#include <alef.h>
#include <bio.h>
#include "dat.h"
#include "fns.h"

void timeslave(int, uint*, chan(int));

void
main(int argc, byte **argv)
{
	byte buf[512];
	int fd;
	int readonly;

	date = ctime(time());
	if(date==nil || (date=strdup(date))==nil)
		error("can't get current time");
	user = getuser();
	if(user == nil)
		user = "Wile.E.Coyote";
	usermboxdir = malloc(strlen("/mail/box//")+strlen(user)+1);
	sprint(usermboxdir, "/mail/box/%s/", user);
	usermboxfile = malloc(strlen("/mail/box//mbox")+strlen(user)+1);
	sprint(usermboxfile, "/mail/box/%s/mbox", user);
	if(argc > 1)
		mboxfile = argv[1];
	else
		mboxfile = usermboxfile;

	fd = -1;
	readonly = False;
	if(strcmp(mboxfile, usermboxfile) == 0){
		sprint(buf, "/mail/box/%s/L.reading", user);
		fd = openlockfile(buf);
		if(fd < 0){
			fprint(2, "Mail: %s in use; opened read-only\n", mboxfile);
			readonly = True;
		}
	}
	mbox = mbox->read(mboxfile, readonly);
	proc timeslave(fd, &mbox->len, mbox->cmore);
	mbox->slave();
	error(nil);
}

void
timeslave(int rlock, uint *lenp, chan(int) c)
{
	byte buf[DIRLEN];
	Dir d;

	for(;;){
		sleep(30*1000);
		if(rlock>=0 && write(rlock, buf, 0)<0)
			error("can't maintain L.reading: %r");
		if(dirstat(mboxfile, &d)>=0 && d.length > *lenp)
			c <-= 0;
	}
}
