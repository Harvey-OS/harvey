#include <u.h>
#include <libc.h>
#include <thread.h>

#define STACKSIZE (32*1024)

int count;

Channel *stopchan;

void
error(char *fmt, ...)
{
	int n;
	va_list arg;

	threadprint(2, fmt, arg);
	threadkillall();
	threadexits("error");
}

void
chansendproc(void *ch) {
	Channel**c = ch;
	long v;

	for (v = 1; v <= count; v++) {
		send(c[0], &v);
		recv(c[1], &v);
	}
	send(stopchan, &v);
}

void
chanrecvproc(void *ch) {
	Channel**c = ch;
	long v = 0;

	for (;;) {
		recv(c[0], &v);
		send(c[1], &v);
		if (v == count) break;
	}
	send(stopchan, &v);
}

void
pipesendproc(void *ch) {
	int *p = ch;
	long v;

	for (v = 1; v <= count; v++) {
		write(p[1], &v, sizeof(v));
		read(p[1], &v, sizeof(v));
	}
	send(stopchan, &v);
}

void
piperecvproc(void *ch) {
	int *p = ch;
	long v = 0;

	for (;;) {
		read(p[0], &v, sizeof(v));
		write(p[0], &v, sizeof(v));
		if (v == count) break;
	}
	send(stopchan, &v);
}

void
yieldproc(void *) {
	long i;

	for (i = 0; i < count; i++) {
		yield();
	}
	send(stopchan, &i);
}

void
chanmain(int argc, char *argv[]) {
	Channel *c[2];
	long done;

	count = atoi(argv[1]);

	c[0] = chancreate(sizeof(long), 0);
	c[1] = chancreate(sizeof(long), 0);
	stopchan = chancreate(sizeof(long), 0);
	proccreate(chansendproc, (void *)c, STACKSIZE);
	proccreate(chanrecvproc, (void *)c, STACKSIZE);
	recv(stopchan, &done);
	recv(stopchan, &done);
}

void
threadchanmain(int argc, char *argv[]) {
	Channel *c[2];
	long done;

	count = atoi(argv[1]);

	c[0] = chancreate(sizeof(long), 0);
	c[1] = chancreate(sizeof(long), 0);
	stopchan = chancreate(sizeof(long), 0);
	threadcreate(chansendproc, (void *)c, STACKSIZE);
	threadcreate(chanrecvproc, (void *)c, STACKSIZE);
	recv(stopchan, &done);
	recv(stopchan, &done);
}

void
pipemain(int argc, char *argv[]) {
	int p[2];
	long done;

	count = atoi(argv[1]);

	pipe(p);
	stopchan = chancreate(sizeof(long), 0);
	proccreate(pipesendproc, (void *)p, STACKSIZE);
	proccreate(piperecvproc, (void *)p, STACKSIZE);
	recv(stopchan, &done);
	recv(stopchan, &done);
}

void
yieldmain(int argc, char *argv[]) {
	Channel *c[2];
	long done;

	count = atoi(argv[1]);

	stopchan = chancreate(sizeof(long), 0);
	threadcreate(yieldproc, nil, STACKSIZE);
	threadcreate(yieldproc, nil, STACKSIZE);
	recv(stopchan, &done);
	recv(stopchan, &done);
}

void
threadmain(int argc, char *argv[]) {

//	chanmain(argc, argv);
//	pipemain(argc, argv);
//	threadchanmain(argc, argv);
	yieldmain(argc, argv);
	exits(0);
}
