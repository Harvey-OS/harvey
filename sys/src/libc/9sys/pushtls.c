#include <u.h>
#include <libc.h>
#include <auth.h>
#include <mp.h>
#include <libsec.h>

enum {
	TLSFinishedLen = 12,
	HFinished = 20,
};

static int
finished(int hand, int isclient)
{
	int i, n;
	uchar buf[500], buf2[500];

	buf[0] = HFinished;
	buf[1] = TLSFinishedLen>>16;
	buf[2] = TLSFinishedLen>>8;
	buf[3] = TLSFinishedLen;
	n = TLSFinishedLen+4;

	for(i=0; i<2; i++){
		if(i==0)
			memmove(buf+4, "client finished", TLSFinishedLen);
		else
			memmove(buf+4, "server finished", TLSFinishedLen);
		if(isclient == 1-i){
			if(write(hand, buf, n) != n)
				return -1;
		}else{
			if(readn(hand, buf2, n) != n || memcmp(buf,buf2,n) != 0)
				return -1;
		}
	}
	return 1;
}


// given a plain fd and secrets established beforehand, return encrypted connection
int
pushtls(int fd, char *hashalg, char *encalg, int isclient, char *secret, char *dir)
{
	char buf[8];
	char dname[64];
	int n, data, ctl, hand;

	// open a new filter; get ctl fd
	data = hand = -1;
	// /net/tls uses decimal file descriptors to name channels, hence a
	// user-level file server can't stand in for #a; may as well hard-code it.
	ctl = open("#a/tls/clone", ORDWR);
	if(ctl < 0)
		goto error;
	n = read(ctl, buf, sizeof(buf)-1);
	if(n < 0)
		goto error;
	buf[n] = 0;
	if(dir)
		sprint(dir, "#a/tls/%s", buf);

	// get application fd
	sprint(dname, "#a/tls/%s/data", buf);
	data = open(dname, ORDWR);
	if(data < 0)
		goto error;

	// get handshake fd
	sprint(dname, "#a/tls/%s/hand", buf);
	hand = open(dname, ORDWR);
	if(hand < 0)
		goto error;

	// speak a minimal handshake
	if(fprint(ctl, "fd %d 0x301", fd) < 0 ||
	   fprint(ctl, "version 0x301") < 0 ||
	   fprint(ctl, "secret %s %s %d %s", hashalg, encalg, isclient, secret) < 0 ||
	   fprint(ctl, "changecipher") < 0 ||
	   finished(hand, isclient) < 0 ||
	   fprint(ctl, "opened") < 0){
		close(hand);
		hand = -1;
		goto error;
	}
	close(ctl);
	close(hand);
	close(fd);
	return data;

error:
	if(data>=0)
		close(data);
	if(ctl>=0)
		close(ctl);
	if(hand>=0)
		close(hand);
	return -1;
}
