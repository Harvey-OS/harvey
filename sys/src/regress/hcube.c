#include <u.h>
#include <libc.h>

int dim = 6;

int
fdfork(int fd, int post)
{
	static int seq;
	char path[64];
	char buf[64];
	int pfd, len;
	if(post){
		int ch[2];

		pipe(ch);

		len = snprint(path, sizeof path, "hcube.%d.%d", getpid(), seq++);
		pfd = create(path, OWRITE|ORCLOSE, 0666);
		if(pfd == -1)
			sysfatal("fdfork: create: %r");
		snprint(buf, sizeof buf, "%d", ch[0]);
		if(write(pfd, buf, strlen(buf)) != strlen(buf)){
			remove(path);
			sysfatal("fdfork: post '%s': %r", buf);
		}
		close(ch[0]);

		if(write(fd, path, len) != len){
			remove(path);
			sysfatal("fdfork: write to peer: %r");
		}
		len = read(fd, buf, sizeof buf-1);
		if(len <= 0){
			remove(path);
			sysfatal("fdfork: read ack: %s from peer: %r", len == -1 ? "error" : "eof");
		}
		buf[len] = '\0';
		if(strcmp(path, buf)){
			remove(path);
			print("ack from wrong peer: got '%s' want '%s'\n", buf, path);
			sysfatal("ack from wrong peer");
		}
		close(pfd);
		//remove(path);

		return ch[1];
	} else {
		len = read(fd, path, sizeof path-1);
		if(len <= 0)
			sysfatal("fdfork: read path: %s from peer: %r", len == -1 ? "error" : "eof");
		path[len] = '\0';
		pfd = open(path, OWRITE);
		if(pfd == -1)
			sysfatal("fdfork: open: %r");
		if(write(fd, path, len) != len)
			sysfatal("fdfork: write ack to peer: %r");

		return pfd;
	}
}

int
hyperfork(int *fd, int id, int dim)
{
	int chfd[dim];
	int ch[2];
	int i;

	for(i = 0; i < dim; i++)
		chfd[i] = fdfork(fd[i], (id & (1<<i)) == 0);
	pipe(ch);
	switch(fork()){
	case -1:
		sysfatal("rfork");
	case 0:
		for(i = 0; i < dim; i++){
			close(fd[i]);
			fd[i] = chfd[i];
		}
		id |= 1 << dim;
		close(ch[0]);
		fd[dim] = ch[1];
		break;
	default:
		for(i = 0; i < dim; i++)
			close(chfd[i]);
		close(ch[1]);
		fd[dim] = ch[0];
		break;
	}
	return id;
}

void
main(int argc, char *argv[])
{
	int fd[32];
	char buf[32], buf2[32];
	int id;
	int i;

	if(argc > 1)
		dim = strtol(argv[1], nil, 0);

	id = 0;
	chdir("/srv");
	for(i = 0; i < dim; i++)
		id = hyperfork(fd, id, i);

	for(i = 0; i < dim; i++){
		int tlen, rlen;
		tlen = snprint(buf, sizeof buf, "hello %d\n", id);
		write(fd[i], buf, tlen);
		rlen = read(fd[i], buf, sizeof buf-1);
		buf[rlen] = '\0';
		snprint(buf2, sizeof buf2, "%d: %s", id, buf);
	}

	for(i = 0; i < dim; i++)
		if((id & (1<<i)) == 0)
			waitpid();
	print("PASS\n");
	exits(nil);
}
