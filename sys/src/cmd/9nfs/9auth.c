/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>

#define	NETCHLEN	16		/* max network challenge length	*/

char *	argv0;
int	debug;
int	delete;

char *	root = "/n/emelie";
char *	user;
char	file[64];
char	challenge[NETCHLEN];
char	response[NETCHLEN];

void
usage(void)
{
	printf("usage: %s [-d] username\n", argv0);
	exit(1);
}

void
main(int argc, char **argv)
{
	int fd, n;

	for(argv0=*argv++,--argc; argc>0; ++argv,--argc){
		if(argv[0][0] != '-' || argv[0][1] == '-')
			break;
		switch(argv[0][1]){
		case 'D':
			++debug;
			break;
		case 'd':
			++delete;
			break;
		case 'r':
			root = argv[0][2] ? &argv[0][2] : (--argc, *++argv);
			break;
		default:
			usage();
			break;
		}
	}
	if(argc != 1)
		usage();
	user = argv[0];
	snprintf(file, sizeof file, "%s/#%s", root, user);
	if(debug)
		printf("debug=%d, file=%s\n", debug, file);
	if(delete){
		fd = creat(file, 0600);
		if(fd < 0){
			perror(file);
			exit(1);
		}
		exit(0);
	}
	fd = open(file, 2);
	if(fd < 0){
		perror(file);
		exit(1);
	}
	n = read(fd, challenge, NETCHLEN);
	if(debug)
		printf("read %d\n", n);
	if(n <= 0){
		printf("read %d: ", n);
		perror("");
		exit(1);
	}
	printf("challenge: %s\n", challenge);
	write(1, "response: ", 10);
	read(0, response, NETCHLEN-1);
	lseek(fd, 0, 0);
	n = write(fd, response, NETCHLEN);
	if(debug)
		printf("write %d\n", n);
	if(n <= 0){
		printf("write %d: ", n);
		perror("");
		exit(1);
	}
	exit(0);
}
