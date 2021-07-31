#include "lib9.h"
#include "ctype.h"

/*
 *  make an address, add the defaults
 */
void
netmkaddr(char *addr, char *linear, char *defnet, char *defsrv)
{
	char *cp;

	/*
	 *  dump network name
	 */
	cp = strchr(linear, '!');
	if(cp == 0){
		if(defnet==0){
			if(defsrv)
				sprint(addr, "net!%s!%s",
					linear, defsrv);
			else
				sprint(addr, "net!%s", linear);
		}
		else {
			if(defsrv)
				sprint(addr, "%s!%s!%s", defnet,
					linear, defsrv);
			else
				sprint(addr, "%s!%s", defnet,
					linear);
		}
		return;
	}

	/*
	 *  if there is already a service, use it
	 */
	cp = strchr(cp+1, '!');
	if(cp) {
		strcpy(addr, linear);
		return;
	}

	/*
	 *  add default service
	 */
	if(defsrv == 0) {
		strcpy(addr, linear);
		return;
	}

	sprint(addr, "%s!%s", linear, defsrv);
}
