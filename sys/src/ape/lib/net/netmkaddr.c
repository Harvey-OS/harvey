#include <stdlib.h>
#include <sys/types.h>
#include <string.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <libnet.h>

/*
 *  make an address, add the defaults
 */
char *
netmkaddr(char *linear, char *defnet, char *defsrv)
{
	static char addr[256];
	char *cp;

	/*
	 *  dump network name
	 */
	cp = strchr(linear, '!');
	if(cp == 0){
		if(defnet==0){
			if(defsrv)
				snprintf(addr, sizeof(addr), "net!%s!%s",
					linear, defsrv);
			else
				snprintf(addr, sizeof(addr), "net!%s", linear);
		}
		else {
			if(defsrv)
				snprintf(addr, sizeof(addr), "%s!%s!%s", defnet,
					linear, defsrv);
			else
				snprintf(addr, sizeof(addr), "%s!%s", defnet,
					linear);
		}
		return addr;
	}

	/*
	 *  if there is already a service, use it
	 */
	cp = strchr(cp+1, '!');
	if(cp)
		return linear;

	/*
	 *  add default service
	 */
	if(defsrv == 0)
		return linear;
	snprintf(addr, sizeof(addr), "%s!%s", linear, defsrv);

	return addr;
}
