#include "vnc.h"

char*
netmkvncaddr(char *inserver)
{
	char *p, portstr[20], *server;
	int port;

	server = strdup(inserver);
	assert(server != nil);

	port =  5900;
	if(p = strchr(server, ':')) {
		*p++ = '\0';
		port += atoi(p);
	}

	sprint(portstr, "%d", port);
	p = netmkaddr(server, nil, portstr);
	free(server);
	return p;
}

