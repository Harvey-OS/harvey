#include	"all.h"
#include	"io.h"

void
cmd_passwd(int argc, char *argv[])
{
	char passwd[32];
	static char zeros[DESKEYLEN];
	char nkey1[DESKEYLEN], nkey2[DESKEYLEN];
	char authid[NAMELEN];
	char authdom[DOMLEN];

	USED(argc, argv);

	if(memcmp(nvr.authkey, zeros, sizeof(nvr.authkey))) {
		print("Old password:");
		getstring(passwd, sizeof(passwd), 0);
		memset(nkey1, 0, DESKEYLEN);
		passtokey(nkey1, passwd);
		if(memcmp(nkey1, nvr.authkey, DESKEYLEN)) {
			print("Bad password\n");
			delay(1000);
			return;
		}
	}

	print("New password:");
	getstring(passwd, sizeof(passwd), 0);
	memset(nkey1, 0, DESKEYLEN);
	passtokey(nkey1, passwd);

	print("Confirm password:");
	getstring(passwd, sizeof(passwd), 0);
	memset(nkey2, 0, DESKEYLEN);
	passtokey(nkey2, passwd);

	if(memcmp(nkey1, nkey2, DESKEYLEN)) {
		print("don't match\n");
		return;
	}
	memmove(nvr.authkey, nkey1, DESKEYLEN);
	nvr.authsum = nvcsum(nvr.authkey, DESKEYLEN);

	print("Authentication id:");
	getstring(authid, sizeof(authid), 1);
	if(authid[0]){
		memset(nvr.authid, 0, NAMELEN);
		strcpy(nvr.authid, authid);
		nvr.authidsum = nvcsum(nvr.authid, NAMELEN);
	}

	print("Authentication domain:");
	getstring(authdom, sizeof(authdom), 1);
	if(authdom[0]){
		memset(nvr.authdom, 0, NAMELEN);
		strcpy(nvr.authdom, authdom);
		nvr.authdomsum = nvcsum(nvr.authdom, NAMELEN);
	}
	nvwrite(NVRAUTHADDR, &nvr, sizeof(nvr));
}

/*
 *  create a challenge for a fid space
 */
void
mkchallenge(Chan *cp)
{
	int i;

	srand((ulong)cp + m->ticks);
	for(i = 0; i < CHALLEN; i++)
		cp->chal[i] = nrand(256);

	cp->idoffset = 0;
	cp->idvec = 0;
}

/*
 *  match a challenge from an attach
 */
int
authorize(Chan *cp, Fcall *in, Fcall *ou)
{
	Ticket t;
	Authenticator a;
	int x;
	ulong bit;

	if(noauth || wstatallow)		/* set to allow entry during boot */
		return 1;

	if(strcmp(in->uname, "none") == 0)
		return 1;

	if(in->type == Toattach)
		return 0;

	/* decrypt and unpack ticket */
	convM2T(in->ticket, &t, nvr.authkey);
	if(t.num != AuthTs){
print("bad AuthTs num\n");
		return 0;
	}

	/* decrypt and unpack authenticator */
	convM2A(in->auth, &a, t.key);
	if(a.num != AuthAc){
print("bad AuthAc num\n");
		return 0;
	}

	/* challenges must match */
	if(memcmp(a.chal, cp->chal, sizeof(a.chal)) != 0){
print("bad challenge\n");
		return 0;
	}

	/*
	 *  the id must be in a valid range.  the range is specified by a
	 *  lower bount (idoffset) and a bit vector (idvec) where a
	 *  bit set to 1 means unusable
	 */
	lock(&cp->idlock);
	x = a.id - cp->idoffset;
	bit = 1<<x;
	if(x < 0 || x > 31 || (bit&cp->idvec)){
		unlock(&cp->idlock);
print("id out of range: idoff %d idvec %lux id %d\n", cp->idoffset, cp->idvec, a.id);
		return 0;
	}
	cp->idvec |= bit;

	/* normalize the vector */
	while(cp->idvec&0xffff0001){
		cp->idvec >>= 1;
		cp->idoffset++;
	}
	unlock(&cp->idlock);

	/* ticket name and attach name must match */
	if(memcmp(in->uname, t.cuid, sizeof(in->uname)) != 0){
print("names don't match\n");
		return 0;
	}

	/* copy translated name into input record */
	memmove(in->uname, t.suid, sizeof(in->uname));

	/* craft a reply */
	a.num = AuthAs;
	memmove(a.chal, cp->rchal, CHALLEN);
	convA2M(&a, ou->rauth, t.key);

	return 1;
}

void
getstring(char *str, int n, int doecho)
{
	int c;
	char *p, *e;

	memset(str, 0, sizeof(n));
	p = str;
	e = str+n-1;
	echo = doecho;
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
		getstring(passwd, sizeof(passwd), 0);
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
