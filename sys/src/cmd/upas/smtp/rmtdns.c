#include	"common.h"
#include	<ndb.h>

int
rmtdns(char *path)
{

	int fd, n, r;
	char *domain, *cp, buf[1024];

	if(path == 0)
		return 0;

	domain = strdup(path);
	cp = strchr(domain, '!');
	if(cp){
		*cp = 0;
		n = cp-domain;
	} else
		n = strlen(domain);

	if(*domain == '[' && domain[n-1] == ']'){	/* accept [nnn.nnn.nnn.nnn] */
		domain[n-1] = 0;
		r = strcmp(ipattr(domain+1), "ip");
		domain[n-1] = ']';
	} else
		r = strcmp(ipattr(domain), "ip");	/* accept nnn.nnn.nnn.nnn */

	if(r == 0){
		free(domain);
		return 0;
	}

	n = snprint(buf, sizeof(buf), "%s all", domain);
	free(domain);

	fd = open("/net.alt/dns", ORDWR);	/* look up all others */
	if(fd < 0)				/* dns screw up - can't check */
		return 0;
	seek(fd, 0, 0);
	n = write(fd, buf, n);
	close(fd);

	if(n < 0){
		errstr(buf);
		if (strcmp(buf, "dns: name does not exist") == 0)
			return -1;
	}
	return 0;
}

