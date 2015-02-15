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
#include <bio.h>
#include <auth.h>

/*
 *  see if user is in the group noworld, i.e., has all file
 *  priviledges masked with 770, and all directories with
 *  771, before checking access rights
 */
int
noworld(char *user)
{
	Biobuf *b;
	char *p;
	int n;

	b = Bopen("/adm/users", OREAD);
	if(b == nil)
		return 0;
	while((p = Brdline(b, '\n')) != nil){
		p[Blinelen(b)-1] = 0;
		p = strchr(p, ':');
		if(p == nil)
			continue;
		if(strncmp(p, ":noworld:", 9) == 0){
			p += 9;
			break;
		}
	}
	n = strlen(user);
	while(p != nil && *p != 0){
		p = strstr(p, user);
		if(p == nil)
			break;
		if(*(p-1) == ':' || *(p-1) == ',')
		if(*(p+n) == ':' || *(p+n) == ',' || *(p+n) == 0){
			Bterm(b);
			return 1;
		}
		p++;
	}
	Bterm(b);
	return 0;
}
