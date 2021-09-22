/* client ssh auth configuration */

#include "ssh.h"

Auth *allauth[] = {
	&authpassword,
	&authrsa,
	&authtis,
};
int nelemallauth = nelem(allauth);
char *authlist = "rsa password tis";
