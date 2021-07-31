#include <alef.h>

/*
 *  make an address, add the defaults
 */


byte *
netmkaddr(byte *linear, byte *defnet, byte *defsrv)
{
	byte *cp;
	byte addr[4*(NAMELEN+1)];

	/*
	 *  dump network name
	 */
	cp = strchr(linear, '!');
	if(cp == nil) {
		if(defnet == nil) {
			if(defsrv)
				sprint(addr, "net!%.*s!%.*s", 2*NAMELEN, linear,
					NAMELEN, defsrv);
			else
				sprint(addr, "net!%.*s", 2*NAMELEN, linear);
		}
		else {
			if(defsrv)
				sprint(addr, "%.*s!%.*s!%.*s", NAMELEN, defnet,
					2*NAMELEN, linear, NAMELEN, defsrv);
			else
				sprint(addr, "%.*s!%.*s", NAMELEN, defnet,
					2*NAMELEN, linear);
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
	if(defsrv == nil)
		return linear;
	sprint(addr, "%.*s!%.*s", 3*NAMELEN, linear,
		NAMELEN, defsrv);

	return addr;
}
