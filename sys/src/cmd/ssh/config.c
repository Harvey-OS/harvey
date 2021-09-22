/* common ssh cipher configuration */

#include "ssh.h"

Cipher *allcipher[] = {
//	&cipherrc4,			/* deprecated */
	&cipherblowfish,
	&cipher3des,
	&cipherdes,
	&ciphernone,
	&ciphertwiddle,
};
int nelemallcipher = nelem(allcipher);
char *cipherlist = "blowfish 3des";	/* rc4 deprecated */

Cipher*
findcipher(char *name, Cipher **list, int nlist)
{
	int i;

	for(i=0; i<nlist; i++)
		if(strcmp(name, list[i]->name) == 0)
			return list[i];
	error("unknown cipher %s", name);
	return nil;
}

void
conninit(Conn *conn, int fd, char *user, char *host)
{
	int i;
	char *f[16];

	memset(conn, 0, sizeof *conn);
	if (user) {
		conn->fd[0] = conn->fd[1] = fd;
		conn->user = user;
		conn->host = host;
		setaliases(conn, host);
	}

	conn->nokcipher = getfields(cipherlist, f, nelem(f), 1, ", ");
	conn->okcipher = emalloc(sizeof(Cipher*)*conn->nokcipher);
	for(i=0; i<conn->nokcipher; i++)
		conn->okcipher[i] = findcipher(f[i], allcipher, nelemallcipher);

#ifdef notdef
	conn->nokauth = getfields(authlist, f, nelem(f), 1, ", ");
	conn->okauth = emalloc(sizeof(Auth*)*conn->nokauth);
	for(i=0; i<conn->nokauth; i++)
		conn->okauth[i] = findauth(f[i], allauth, nelemallauth);
#endif
}
