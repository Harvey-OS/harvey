/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include <u.h>
#include <libc.h>
#include <../boot/boot.h>

char *fparts[] =
{
	"add bootldr	0x0000000 0x0040000",
	"add params	0x0040000 0x0080000",
	"add kernel	0x0080000 0x0140000",
	"add user	0x0140000 0x0200000",
	"add ramdisk	0x0200000 0x0600000",
};

void
configpaq(Method*)
{
	int fd;
	int i;

	if(bind("#F", "/dev", MAFTER) < 0)
		fatal("bind #c");
	if(bind("#p", "/proc", MREPL) < 0)
		fatal("bind #p");
	fd = open("/dev/flash/flashctl", OWRITE);
	if(fd < 0)
		fatal("opening flashctl");
	for(i = 0; i < nelem(fparts); i++)
		if(fprint(fd, fparts[i]) < 0)
			fatal(fparts[i]);
	close(fd);
}

int
connectpaq(void)
{
	int  p[2];
	char **arg, **argp;

	print("paq...");
	if(pipe(p)<0)
		fatal("pipe");
	switch(fork()){
	case -1:
		fatal("fork");
	case 0:
		arg = malloc(10*sizeof(char*));
		argp = arg;
		*argp++ = "paqfs";
		*argp++ = "-v";
		*argp++ = "-i";
		*argp++ = "/dev/flash/ramdisk";
		*argp = 0;

		dup(p[0], 0);
		dup(p[1], 1);
		close(p[0]);
		close(p[1]);
		exec("/boot/paqfs", arg);
		fatal("can't exec paqfs");
	default:
		break;
	}
	waitpid();

	close(p[1]);
	return p[0];
}
