#include <u.h>
#include <libc.h>
#include <auth.h>
#include <../boot/boot.h>

char	password[NAMELEN];

char *homsg = "can't set user name or key; please reboot";

/*
 *  get/set user name and password.  verify password with auth server.
 */
void
userpasswd(int islocal, Method *mp)
{
	int fd;
	char *msg;
	char hostkey[DESKEYLEN];

	if(*username == 0 || strcmp(username, "none") == 0){
		strcpy(username, "none");
		outin(cpuflag, "user", username, sizeof(username));
	}
	fd = -1;
	while(strcmp(username, "none") != 0){
		getpasswd(password, sizeof password);
		passtokey(hostkey, password);
		fd = -1;
		if(islocal)
			break;
		msg = checkkey(mp, username, hostkey);
		if(msg == 0)
			break;
		fprint(2, "?%s\n", msg);
		outin(cpuflag, "user", username, sizeof(username));
	}
	if(fd > 0)
		close(fd);

	/* set host's key */
	if(writefile("#c/key", hostkey, DESKEYLEN) < 0)
		fatal(homsg);

	/* set host's owner (and uid of current process) */
	if(writefile("#c/hostowner", username, strlen(username)) < 0)
		fatal(homsg);
	close(fd);
}
