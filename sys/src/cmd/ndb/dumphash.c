#include <u.h>
#include <libc.h>
#include <bio.h>
#include <ndb.h>

char*
syserr(void)
{
	static char buf[ERRLEN];

	errstr(buf);
	return buf;
}

main(int argc, char *argv[])
{
	int fd;
	uchar buf[8];
	char file[2*NAMELEN];
	ulong hlen;
	ulong date;
	ulong ptr;
	int i;

	if(argc != 3){
		fprint(2, "usage: dumphash db-file attribute\n");
		exits("usage");
	}
	snprint(file, sizeof(file), "%s.%s", argv[1], argv[2]);
	fd = open(file, OREAD);
	if(fd < 0){
		fprint(2, "%s opening %s\n", syserr(), file);
		exits(syserr());
	}
	if(read(fd, buf, NDBHLEN) != NDBHLEN){
		fprint(2, "premature EOF\n");
		exits("1");
	}
	date = NDBGETUL(buf);
	hlen = NDBGETUL(buf+4);
	print("hlen %d time %s", hlen, asctime(localtime(date)));
	for(i = 0; i < hlen; i++){
		if(seek(fd, NDBHLEN+NDBPLEN*i, 0) < 0
		|| read(fd, buf, NDBPLEN) != NDBPLEN){
			fprint(2, "premature EOF %d\n", i);
			exits("1");
		}
		ptr = NDBGETP(buf);
		if(ptr == NDBNAP)
			continue;
		if((ptr & NDBCHAIN) == 0){
			print("%d\n", ptr);
			continue;
		}
		print("ind %d\n", ptr & ~NDBCHAIN);
		for(;;){
			ptr &= ~NDBCHAIN;
			if(seek(fd, NDBHLEN+ptr, 0) < 0
			|| read(fd, buf, 6) != 6){
				fprint(2, "premature EOF %d\n", ptr);
				exits("2");
			}
			ptr = NDBGETP(buf);
			print("->%d\n", ptr);
			ptr = NDBGETP(buf+NDBPLEN);
			if((ptr & NDBCHAIN) == 0){
				print("->%d\n", ptr);
				break;
			}
		}
	}
}
