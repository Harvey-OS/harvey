#include	"all.h"
#include	"io.h"

void
cmd_passwd(int argc, char *argv[])
{
	char passwd[32];
	static char zeros[DESKEYLEN];
	char nkey1[DESKEYLEN], nkey2[DESKEYLEN];

	USED(argc, argv);

	if(memcmp(nvr.authkey, zeros, sizeof(nvr.authkey))) {
		print("Old password:");
		getpasswd(passwd, sizeof(passwd));
		memset(nkey1, 0, DESKEYLEN);
		passtokey(nkey1, passwd);
		if(memcmp(nkey1, nvr.authkey, DESKEYLEN)) {
			print("Bad password\n");
			delay(1000);
			return;
		}
	}

	print("New password:");
	getpasswd(passwd, sizeof(passwd));
	memset(nkey1, 0, DESKEYLEN);
	passtokey(nkey1, passwd);

	print("Confirm password:");
	getpasswd(passwd, sizeof(passwd));
	memset(nkey2, 0, DESKEYLEN);
	passtokey(nkey2, passwd);

	if(memcmp(nkey1, nkey2, DESKEYLEN)) {
		print("don't match\n");
		return;
	}
	memmove(nvr.authkey, nkey1, DESKEYLEN);
	nvr.authsum = nvcsum(nvr.authkey, DESKEYLEN);
	nvwrite(NVRAUTHADDR, &nvr, sizeof(nvr));
}

void
cmd_auth(int argc, char *argv[])
{
	int i, keys;
	Uid *u;
	char *file;
	char buf[KEYDBLEN];
	static int started;

	if(nzip(authip)) {
		ilauth();
		return;
	}

	file = "/adm/keys";
	if(argc > 1)
		file = argv[1];

	wlock(&uidgc.uidlock);
	uidgc.uidbuf = getbuf(devnone, Cuidbuf, 0);

	if(walkto(file) || con_open(FID2, 0))
		goto bad;

	uidgc.flen = 0;
	uidgc.find = 0;
	cons.offset = 0;

	keys = 0;
	for(;;) {
		i = fread(buf, KEYDBLEN);
		if(i == 0)
			break;

		if(i < KEYDBLEN)
			goto bad;

		decrypt(nvr.authkey, buf, KEYDBLEN);
		buf[NAMELEN-1] = '\0';
		u = uidpstr(buf);
		if(u && buf[KEYENABLE] == 0) {
			memmove(u->key, buf+NAMELEN, DESKEYLEN);
			keys++;
		}
	}
	print("read %d enabled keys\n", keys);

	authchan = 0;	/* This should close down the channel ! */
out:
	putbuf(uidgc.uidbuf);
	wunlock(&uidgc.uidlock);
	return;
bad:
	print("error in %s\n", file);
	goto out;
}

int
authorise(File *f, Fcall *in)
{

	if(noauth || wstatallow)		/* set to allow entry during boot */
		return 1;

	if(strcmp(in->uname, "none") == 0)
		return 1;

	decrypt(nvr.authkey, in->auth, AUTHSTR+DESKEYLEN);
	if(memcmp(in->auth, f->ticket, AUTHSTR))
		return 0;

	f->ticket[0]++;
	return 1;
}

int
authsrv(Chan *c, char *buf, int i)
{
	Msgbuf *mb;
	int n;

	if(authchan == 0) {
		print("no connection to auth server\n");
		return 0;
	}

	mb = mballoc(MAXMSG, c, Mbilauth);

	memmove(mb->data, buf, i);
	mb->count = i;
	send(c->reply, mb);

	mb = recv(authreply, 0);

	if(mb->count > AUTHLEN+ERRREC) {
		print("authsrv: long read\n");
		mbfree(mb);
		return -1;
	}
	memmove(buf, mb->data, mb->count);
	n = mb->count;
	mbfree(mb);

	return n;
}

void
getpasswd(char *str, int n)
{
	int c;
	char *p, *e;

	memset(str, 0, sizeof(n));
	p = str;
	e = str+n-1;
	echo = 0;
	for(;;) {
		if(p == e) {
			*p = '\0';
			goto out;
		}
		c = getc();
		switch(c) {
		case '\n':
			*p = '\0';
			print("\n");
			goto out;
		case '\b':
			if(p > str)
				p--;
			break;
		case 'U' - '@':
			p = str;
			break;
		default:
			*p++ = c;
		}
	}
out:
	echo = 1;
}

int
conslock(void)
{
	static char zeroes[DESKEYLEN];
	char passwd[128];
	char nkey1[DESKEYLEN];

	if(memcmp(nvr.authkey, zeroes, DESKEYLEN) == 0) {
		print("no password set\n");
		return 0;
	}

	for(;;) {
		print("%s password:", service);
		getpasswd(passwd, sizeof(passwd));
		memset(nkey1, 0, DESKEYLEN);
		passtokey(nkey1, passwd);
		if(memcmp(nkey1, nvr.authkey, DESKEYLEN) == 0) {
			prdate();
			return 1;
		}

		print("Bad password\n");
		delay(1000);
	}
	return 0;
}
