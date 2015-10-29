#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include "util.h"

char *readpass(char *pw, int n);
void setecho(int);

static void
usage(void)
{
	fprint(2, "usage: passtokey\n");
	exit(1);
}

int
main(int argc, char *argv[])
{
	int c;
	char pw0[28+1], pw1[28+1];
	uchar key[Deskeylen];
	char *err;

	while((c = getopt(argc, argv, "")) != -1)
		switch(c) {
		default:
			usage();
		}
	argc -= optind;
	argv += optind;
	if(argc != 0)
		usage();

	setecho(0);
	for(;;) {
		fprintf(stderr, "password: ");
		fflush(stderr);
		if((err = readpass(pw0, sizeof pw0)) != nil) {
			fprintf(stderr, "\nbad password (%s), try again\n", err);
			continue;
		}
		fprintf(stderr, "\nconfirm: ");
		readpass(pw1, sizeof pw1);
		if(strcmp(pw0, pw1) == 0)
			break;
		fprintf(stderr, "\nmismatch, try again\n");
	}
	setecho(1);
	fprintf(stderr, "\n");
	
	passtokey(key, pw0);
	if(write(1, key, sizeof key) != sizeof key) {
		fprintf(stderr, "writing key: %s\n", strerror(errno));
		exit(1);
	}
	exit(0);
}

void
setecho(int on)
{
	struct termios term;

	tcgetattr(0, &term);
	if(on)
		term.c_lflag |= (ECHO|ECHOE|ECHOK|ECHONL);
	else
		term.c_lflag &= ~(ECHO|ECHOE|ECHOK|ECHONL);
	tcsetattr(0, TCSAFLUSH, &term);
}

char *
readpass(char *pw, int n)
{
	size_t len;
	char buf[64];

	if(fgets(buf, sizeof buf, stdin) == nil)
		return "read error";
	len = strlen(buf);
	if(buf[len-1] == '\n')
		buf[--len] = '\0';
	if(len < 6)
		return "too short";
	if(len >= n)
		return "too long";
	memmove(pw, buf, len+1);
	return nil;
}
