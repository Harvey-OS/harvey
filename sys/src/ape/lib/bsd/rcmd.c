/* posix */
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <signal.h>

/* socket extensions */
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/un.h>
#include <netdb.h>

#include "priv.h"

static char pbotch[] = "rcmd: protocol botch\n";
static char lbotch[] = "rcmd: botch starting error stream\n";

static void
ding(int x)
{
}

int
rcmd(char **dst, int port, char *luser, char *ruser, char *cmd, int *fd2p)
{
	char c;
	int i, fd, lfd, fd2, port2;
	struct hostent *h;
	Rock *r;
	struct sockaddr_in in;
	char buf[128];
	void	(*x)(int);

	h = gethostbyname(*dst);
	if(h == 0)
		return -1;
	*dst = h->h_name;

	/* connect using a reserved tcp port */
	fd = socket(PF_INET, SOCK_STREAM, 0);
	if(fd < 0)
		return -1;
	r = _sock_findrock(fd, 0);
	if(r == 0){
		errno = ENOTSOCK;
		return -1;
	}
	r->reserved = 1;
	in.sin_family = AF_INET;
	in.sin_port = htons(port);
	memmove(&in.sin_addr, h->h_addr_list[0], sizeof(in.sin_addr));
	if(connect(fd, &in, sizeof(in)) < 0){
		close(fd);
		return -1;
	}

	/* error stream */
	if(fd2p){
		/* create an error stream and wait for a call in */
		for(i = 0; i < 10; i++){
			lfd = rresvport(&port2);
			if(lfd < 0)
				continue;
			if(listen(lfd, 1) == 0)
				break;
			close(lfd);
		}
		if(i >= 10){
			fprintf(stderr, pbotch);
			return -1;
		}

		snprintf(buf, sizeof buf, "%d", port2);
		if(write(fd, buf, strlen(buf)+1) < 0){
			close(fd);
			close(lfd);
			fprintf(stderr, lbotch);
			return -1;
		}
	} else {
		if(write(fd, "", 1) < 0){
			fprintf(stderr, pbotch);
			return -1;
		}
	}

	/* pass id's and command */
	if(write(fd, luser, strlen(luser)+1) < 0
	|| write(fd, ruser, strlen(ruser)+1) < 0
	|| write(fd, cmd, strlen(cmd)+1) < 0){
		if(fd2p)
			close(fd2);
		fprintf(stderr, pbotch);
		return -1;
	}

	if(fd2p){
		x = signal(SIGALRM, ding);
		alarm(15);
		fd2 = accept(lfd, &in, &i);
		alarm(0);
		close(lfd);
		signal(SIGALRM, x);

		if(fd2 < 0){
			close(fd);
			close(lfd);
			fprintf(stderr, lbotch);
			return -1;
		}
		*fd2p = fd2;
	}

	/* get reply */
	if(read(fd, &c, 1) != 1){
		if(fd2p)
			close(fd2);
		fprintf(stderr, pbotch);
		return -1;
	}
	if(c == 0)
		return fd;
	i = 0;
	while(c){
		buf[i++] = c;
		if(read(fd, &c, 1) != 1)
			break;
		if(i >= sizeof(buf)-1)
			break;
	}
	buf[i] = 0;
	fprintf(stderr, "rcmd: %s\n", buf);
	close(fd);
	return -1;
}
