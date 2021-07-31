#include <u.h>
#include <libc.h>
#include <auth.h>
#include <../boot/boot.h>

char	password[NAMELEN];
#ifdef asdf
extern	char *sauth;
#endif asdf

/*
 *  get/set user name and password.  verify password with auth server.
 */
void
userpasswd(int islocal, Method *mp)
{
	char key[7];
	char buf[8 + NAMELEN];
	int fd, crfd;

	if(*username == 0 || strcmp(username, "none") == 0){
		strcpy(username, "none");
		outin("user", username, sizeof(username));
	}
	crfd = fd = -1;
	while(strcmp(username, "none") != 0){
		getpasswd(password, sizeof password);
		passtokey(key, password);
		fd = open("#c/key", OWRITE);
		if(fd < 0)
			fatal("can't open #c/key; please reboot");
		if(write(fd, key, 7) != 7)
			fatal("can't write #c/key; please reboot");
		close(fd);
		crfd = open("#c/crypt", ORDWR);
		if(crfd < 0)
			fatal("can't open crypt file");
		write(crfd, "E", 1);
		fd = -1;
		if(islocal)
			break;
		if(mp->auth)
			fd = (*mp->auth)();
		if(fd < 0){
			warning("password not checked!");
			break;
		}
		strncpy(buf+8, username, NAMELEN);
		if(read(fd, buf, 8) != 8
		|| write(crfd, buf, 8) != 8
		|| read(crfd, buf, 8) != 8
		|| write(fd, buf, 8 + NAMELEN) != 8 + NAMELEN){
			warning("password not checked!");
			break;
		}
		if(read(fd, buf, 2) == 2 && buf[0]=='O' && buf[1]=='K')
			break;
		close(fd);
		outin("user", username, sizeof(username));
	}
	close(fd);
	close(crfd);

	/* set user now that we're sure */
	fd = open("#c/user", OWRITE|OTRUNC);
	if(fd >= 0){
		if(write(fd, username, strlen(username)) < 0)
			warning("write user name");
		close(fd);
	}else
		warning("open #c/user");
}
