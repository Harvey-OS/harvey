#include <u.h>
#include <libc.h>
#include <stdio.h>

void
usage(void)
{
	fprintf(stderr, "usage: LOCK dir pid\n");
	exits("usage");
}

void
main(int argc, char *argv[])
{
	int fd, rv;
	char *procname;
	char *lockname;
	char lockentry[8];
	struct Dir dirbuf;

	if (argc<3) usage();
	lockname = (char *)malloc(strlen(argv[1])+strlen(argv[2])+2);
	procname = (char *)malloc(strlen("/proc/")+strlen(argv[2])+1);
	if (lockname == (char *)0) {
		fprintf(stderr, "malloc failed\n");
		exits("malloc failed");
	}
	sprintf(lockname, "%s/LOCK", argv[1]);
	fd = open(lockname, ORDWR);
	if (fd<0) {
		fd = create(lockname, ORDWR, CHEXCL|0666);
		if (fd<0) exits("lock failed in create");
setlock:	seek(fd, 0, 0);
		rv=write(fd, argv[2], strlen(argv[2]));
		if (rv<0) exits("lock failed in write");
	} else {
		rv = read(fd, lockentry, 6);
		if (rv<0) exits("lock failed in read");
		if (rv>0) {
			if (strncmp(lockentry, argv[2], rv)!=0) {
				sprintf(procname, "/proc/%s", lockentry);
				if (dirstat(procname, &dirbuf)>=0) {
					exits("lock failed in dirstat");
				}
				goto setlock;
			}
		}
	}
	close(fd);
	exits("");
}
