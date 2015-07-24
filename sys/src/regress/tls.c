#include <u.h>
#include <libc.h>

enum {
	Nloop = 1000,
	Nproc = 64,
};

extern uintptr_t gettls0(void);
uint64_t table[32];

void
copier(int in, int out, int count)
{
	char buf[32];
	char buf2[32];
	int tlsfd;
	int i, off, n;
	uint64_t *ptr;
	uint64_t v;
	char c;

	snprint(buf, sizeof buf, "/proc/%d/tls", getpid());
	if((tlsfd = open(buf, ORDWR)) == -1)
		goto fail;

	srand(getpid());
	i = 0;
	while(count == -1 || i < count){
		ptr = &table[rand()%nelem(table)];
		snprint(buf, sizeof buf, "tls 0x%p\n", ptr);
		if(pwrite(tlsfd, buf, strlen(buf), 0) != strlen(buf)){
			fprint(2, "pid %d: write: %r\n", getpid());
			goto fail;
		}

		if(read(in, &c, 1) != 1)
			break;
		*ptr = rand();
		if((v = gettls0()) != *ptr){
			fprint(2, "gettls %p want %p\n", v, *ptr);
			goto fail;
		}
		write(out, &c, 1);

		if((n = pread(tlsfd, buf2, sizeof buf2-1, 0)) == -1){
			fprint(2, "pid %d: read: %r\n", getpid());
			goto fail;
		}
		buf2[n] = '\0';
		if(strcmp(buf, buf2) != 0){
			fprint(2, "pid %d: '%s' != '%s'\n", getpid(), buf, buf2);
			goto fail;
		}

		i++;
	}
	close(tlsfd);
	return;
fail:
	if(tlsfd != -1)
		close(tlsfd);
	print("FAIL %d\n", getpid());
	exits("FAIL");
	return;
}

void
main(void)
{
	int tube[2];
	int out, in;
	int i;

	pipe(tube);
	out = tube[0];
	in = tube[1];
	for(i = 0; i < Nproc; i++){
		pipe(tube);
		switch(rfork(RFPROC|RFMEM|RFFDG)){
		case -1:
			sysfatal("fork %r");
		case 0:
			close(tube[1]);
			copier(in, tube[0], Nloop);
			exits(nil);
		default:
			close(tube[0]);
			in = tube[1];
			break;
		}
	}
	write(out, "x", 1);
	copier(in, out, Nloop-1);
	close(out);
	for(i = 0; i < 8; i++)
		waitpid();
	exits(nil);
}
