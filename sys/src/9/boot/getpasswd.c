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

void
getpasswd(char *p, int len)
{
	char c;
	int i, n, fd;

	fd = open("#c/consctl", OWRITE);
	if(fd < 0)
		fatal("can't open consctl; please reboot");
	write(fd, "rawon", 5);
 Prompt:		
	print("password: ");
	n = 0;
	for(;;){
		do{
			i = read(0, &c, 1);
			if(i < 0)
				fatal("can't read cons; please reboot");
		}while(i == 0);
		switch(c){
		case '\n':
			p[n] = '\0';
			close(fd);
			print("\n");
			return;
		case '\b':
			if(n > 0)
				n--;
			break;
		case 'u' - 'a' + 1:		/* cntrl-u */
			print("\n");
			goto Prompt;
		default:
			if(n < len - 1)
				p[n++] = c;
			break;
		}
	}
}
