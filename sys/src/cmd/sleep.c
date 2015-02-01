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

void
main(int argc, char *argv[])
{
	long n;
	char *p, *q;

	if(argc>1){
		for(n = strtol(argv[1], &p, 0); n > 0; n--)
			sleep(1000);
		/*
		 * no floating point because it is useful to
		 * be able to run sleep when bootstrapping
		 * a machine.
		 */
		if(*p++ == '.' && (n = strtol(p, &q, 10)) > 0){
			switch(q - p){
			case 0:
				break;
			case 1:
				n *= 100;
				break;
			case 2:
				n *= 10;
				break;
			default:
				p[3] = 0;
				n = strtol(p, 0, 10);
				break;
			}
			sleep(n);
		}
	}
	exits(0);
}
