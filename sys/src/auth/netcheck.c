#include <u.h>
#include <libc.h>
#include <auth.h>
#include "authsrv.h"

char *
findnetkey(char *user)
{
	char *key;
	char k[DESKEYLEN];
	int fd;
	char buf[sizeof NETKEYDB + NAMELEN + 6];

	sprint(buf, "%s/%s/key", NETKEYDB, user);
	fd = open(buf, OREAD);
	if(fd < 0)
		return 0;
	if(read(fd, k, DESKEYLEN) != DESKEYLEN){
		close(fd);
		return 0;
	}
	close(fd);
	key = malloc(DESKEYLEN);
	if(key == 0)
		return 0;
	memmove(key, k, DESKEYLEN);
	return key;
}

/*
 * compute the proper response.  We encrypt the ascii of
 * challenge number, with trailing binary zero fill.
 * This process was derived empirically.
 * this was copied from inet's guard.
 */
char *
netresp(char *key, long chal, char *answer)
{
	uchar buf[8];

	memset(buf, 0, 8);
	sprint((char *)buf, "%lud", chal);
	if(encrypt(key, buf, 8) < 0)
		error("can't encrypt response");
	chal = (buf[0]<<24)+(buf[1]<<16)+(buf[2]<<8)+buf[3];
	sprint(answer, "%.8lux", chal);

	return answer;
}

char *
netdecimal(char *answer)
{
	int i;

	for(i = 0; answer[i]; i++)
		switch(answer[i]){
		case 'a': case 'b': case 'c':
			answer[i] = '2';
			break;
		case 'd': case 'e': case 'f':
			answer[i] = '3';
			break;
		}
	return answer;
}

int
netcheck(char *key, long chal, char *response)
{
	char answer[32], *p;
	int i;

	if(p = strchr(response, '\n'))
		*p = '\0';
	netresp(key, chal, answer);

	/*
	 * check for hex response -- securenet mode 1 or 5
	 */
	for(i = 0; response[i]; i++)
		if(response[i] >= 'A' && response[i] <= 'Z')
			response[i] -= 'A' - 'a';
	if(strcmp(answer, response) == 0)
		return 1;

	/*
	 * check for decimal response -- securenet mode 0 or 4
	 */
	return strcmp(netdecimal(answer), response) == 0;
}
