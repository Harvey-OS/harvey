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

uint8_t statbuf[Statsz];

int
cache(int fd)
{
	int argc, i, p[2];
	char *argv[5], bd[32], buf[256], partition[64], *pp;

	if(stat("/boot/cfs", statbuf, sizeof statbuf) < 0)
		return fd;

	*partition = 0;

	bind("#S", "/dev", MAFTER);
	readfile("#e/cfs", buf, sizeof(buf));
	if(*buf){
		argc = tokenize(buf, argv, 4);
		for(i = 0; i < argc; i++){
			if(strcmp(argv[i], "off") == 0)
				return fd;
			else if(stat(argv[i], statbuf, sizeof statbuf) >= 0){
				strncpy(partition, argv[i], sizeof(partition)-1);
				partition[sizeof(partition)-1] = 0;
			}
		}
	}

	if(*partition == 0){
		readfile("#e/bootdisk", bd, sizeof(bd));
		if(*bd){
			if((pp = strchr(bd, ':')) != nil)
				*pp = 0;
			/* damned artificial intelligence */
			i = strlen(bd);
			if(strcmp("disk", &bd[i-4]) == 0)
				bd[i-4] = 0;
			else if(strcmp("fs", &bd[i-2]) == 0)
				bd[i-2] = 0;
			else if(strcmp("fossil", &bd[i-6]) == 0)
				bd[i-6] = 0;
			sprint(partition, "%scache", bd);
			if(stat(partition, statbuf, sizeof statbuf) < 0)
				*bd = 0;
		}
		if(*bd == 0){
			sprint(partition, "%scache", bootdisk);
			if(stat(partition, statbuf, sizeof statbuf) < 0)
				return fd;
		}
	}

	print("cfs...");
	if(pipe(p)<0)
		fatal("pipe");
	switch(fork()){
	case -1:
		fatal("fork");
	case 0:
		close(p[1]);
		dup(fd, 0);
		close(fd);
		dup(p[0], 1);
		close(p[0]);
		if(fflag)
			execl("/boot/cfs", "bootcfs", "-rs", "-f", partition, 0);
		else
			execl("/boot/cfs", "bootcfs", "-s", "-f", partition, 0);
		break;
	default:
		close(p[0]);
		close(fd);
		fd = p[1];
		break;
	}
	return fd;
}
