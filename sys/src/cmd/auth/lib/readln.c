#include <u.h>
#include <libc.h>
#include <authsrv.h>
#include <bio.h>
#include "authcmdlib.h"

void
getpass(char *key, char *pass, int check, int confirm)
{
	char rpass[32], npass[32];
	char *err;

	if(pass == nil)
		pass = npass;

	for(;;){
		readln("Password: ", pass, sizeof npass, 1);
		if(confirm){
			readln("Confirm password: ", rpass, sizeof rpass, 1);
			if(strcmp(pass, rpass) != 0){
				print("mismatch, try again\n");
				continue;
			}
		}
		if(!passtokey(key, pass)){
			print("bad password, try again\n");
			continue;
		}
		if(check)
			if(err = okpasswd(pass)){
				print("%s, try again\n", err);
				continue;
			}
		break;
	}
}

int
getsecret(int passvalid, char *p9pass)
{
	char answer[32];

	readln("assign Inferno/POP secret? (y/n) ", answer, sizeof answer, 0);
	if(*answer != 'y' && *answer != 'Y')
		return 0;

	if(passvalid){
		readln("make it the same as the plan 9 password? (y/n) ",
			answer, sizeof answer, 0);
		if(*answer == 'y' || *answer == 'Y')
			return 1;
	}

	for(;;){
		readln("Secret(0 to 256 characters): ", p9pass,
			sizeof answer, 1);
		readln("Confirm: ", answer, sizeof answer, 1);
		if(strcmp(p9pass, answer) == 0)
			break;
		print("mismatch, try again\n");
	}
	return 1;
}

void
readln(char *prompt, char *line, int len, int raw)
{
	char *p;
	int fdin, fdout, ctl, n, nr;

	fdin = open("/dev/cons", OREAD);
	fdout = open("/dev/cons", OWRITE);
	fprint(fdout, "%s", prompt);
	if(raw){
		ctl = open("/dev/consctl", OWRITE);
		if(ctl < 0)
			error("couldn't set raw mode");
		write(ctl, "rawon", 5);
	} else
		ctl = -1;
	nr = 0;
	p = line;
	for(;;){
		n = read(fdin, p, 1);
		if(n < 0){
			close(ctl);
			error("can't read cons\n");
		}
		if(*p == 0x7f)
			exits(0);
		if(n == 0 || *p == '\n' || *p == '\r'){
			*p = '\0';
			if(raw){
				write(ctl, "rawoff", 6);
				write(fdout, "\n", 1);
			}
			close(ctl);
			return;
		}
		if(*p == '\b'){
			if(nr > 0){
				nr--;
				p--;
			}
		}else{
			nr++;
			p++;
		}
		if(nr == len){
			fprint(fdout, "line too long; try again\n");
			nr = 0;
			p = line;
		}
	}
}
