#include "vnc.h"

char*
netmkvncaddr(char *inserver)
{
	char *p, portstr[20], *server;
	int port;

	server = strdup(inserver);
	assert(server != nil);

	port = 5900;
	if(p = strchr(server, ':')) {
		*p++ = '\0';
		port += atoi(p);
	}

	sprint(portstr, "%d", port);
	p = netmkaddr(server, nil, portstr);
	free(server);
	return p;
}

char*
netmkvncsaddr(char * defnet, char *portstr)
{
	char *p, *aportstr;
	int port;

	if(verbose)
		print("parseing port string %s\n", portstr);

	if(portstr != nil) {
		aportstr = strdup(portstr);
		assert(aportstr != nil);

		port = 5900;
		if(p = strchr(aportstr, ':')) {
			*p++ = '\0';
			port += atoi(p);
		}
		free(aportstr);
	} else
		port = 5901;

	sprint(portstr, "%d", port);
	p = netmkaddr("*", defnet, portstr);
	return p;
}
