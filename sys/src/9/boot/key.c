#include <u.h>
#include <libc.h>
#include <auth.h>
#include <../boot/boot.h>

void
key(int islocal, Method *mp)
{
	Nvrsafe safe;
	char password[20];
	int prompt, fd;

	USED(islocal);
	USED(mp);

	prompt = kflag;
	fd = open("#r/nvram", ORDWR);
	if(fd < 0){
		prompt = 1;
		warning("can't open nvram");
	}
	if(seek(fd, 1024+900, 0) < 0
	|| read(fd, &safe, sizeof safe) != sizeof safe)
		warning("can't read nvram key");

getp:
	if(prompt){
		do
			getpasswd(password, sizeof password);
		while(!passtokey(safe.machkey, password));
	}else if(nvcsum(safe.machkey, DESKEYLEN) != safe.machsum){
		warning("bad nvram key");
		prompt = 1;
		kflag = 1;
		goto getp;
	}
	safe.machsum = nvcsum(safe.machkey, DESKEYLEN);
	if(kflag){
		if(seek(fd, 1024+900, 0) < 0
		|| write(fd, &safe, sizeof safe) != sizeof safe)
			warning("can't write key to nvram");
	}
	close(fd);
	fd = open("#c/key", OWRITE);
	if(fd < 0){
		warning("can't open #c/key");
		return;
	}
	else if(write(fd, safe.machkey, DESKEYLEN) != DESKEYLEN)
		warning("can't set #c/key");
	close(fd);
}
