#include <u.h>
#include <libc.h>
#include <auth.h>
#include "srv.h"

enum{
	CHALLEN	= 2 * AUTHLEN + 2 * NAMELEN,
	RESPLEN	= 2 * AUTHLEN + NAMELEN + DESKEYLEN,
	SULEN	= AUTHLEN + NAMELEN + DESKEYLEN,
	MAXLEN	= 2*AUTHLEN + 2*NAMELEN + DESKEYLEN,
};

char *
srvauth(char *user)
{
	char buf[MAXLEN], chal[AUTHLEN];
	int fd;

	/*
	 * get the kernel challenge and pass it to the terminal
	 */
	fd = open("#c/chal", OREAD);
	if(fd < 0)
		return "can't open challenge file";
	if(read(fd, chal, AUTHLEN) != AUTHLEN){
		close(fd);
		return "can't read challenge";
	}
	close(fd);
	if(write(1, chal, AUTHLEN) != AUTHLEN)
		return "can't write challenge";

	/*
	 * forward the encrypted challenge from the terminal
	 * to the auth server
	 */
	if(read(0, buf, CHALLEN) != CHALLEN)
		return "can't read terminal challege";
	strncpy(user, &buf[2*AUTHLEN+NAMELEN], NAMELEN);
	fd = authdial("rexauth");
	if(fd < 0)
		return "can't call authentication server";
	if(write(fd, buf, CHALLEN) != CHALLEN){
		close(fd);
		return "can't write terminal challenge";
	}
	if(read(fd, buf, RESPLEN) != RESPLEN){
		close(fd);
		return "failed to authenticate";
	}
	close(fd);

	if(write(1, buf, RESPLEN) != RESPLEN)
		return "can't write to terminal";
	if(read(0, buf, SULEN) != SULEN)
		return "can't read from terminal";
	netchown(user);
	fd = open("#c/chal", OWRITE|OTRUNC);
	if(fd < 0 || write(fd, buf, SULEN) != SULEN){
		close(fd);
		return "can't set user name";
	}
	close(fd);
	write(1, "OK", 2);
	return 0;
}
