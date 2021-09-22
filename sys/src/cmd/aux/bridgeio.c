#include <u.h>
#include <libc.h>

void
main(int, char **)
{
	int bridgepipe[2], emupipe[2];
	int srvfd;
	char buf[2048];
	int n;

	pipe(bridgepipe);
	pipe(emupipe);
	remove("/srv/bridgepipe");
	remove("/srv/emupipe");
	srvfd = create("/srv/bridgepipe", OWRITE, 0666);
	fprint(srvfd, "%d", bridgepipe[0]);
	close(srvfd);
	close(bridgepipe[0]);
	srvfd = create("/srv/emupipe", OWRITE, 0666);
	fprint(srvfd, "%d", emupipe[0]);
	close(srvfd);
	close(emupipe[0]);
	if(fork()){
		while((n = read(emupipe[1], buf, sizeof buf)) >= 0){
			write(bridgepipe[1], buf, n);
		}
	}else{
		while((n = read(bridgepipe[1], buf + 2, sizeof buf - 2)) >= 0){
			buf[0] = n;
			buf[1] = n >> 8;
			write(emupipe[1], buf, n + 2);
		}
	}
}
