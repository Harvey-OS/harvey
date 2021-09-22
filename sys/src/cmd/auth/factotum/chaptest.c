#include <u.h>
#include <libc.h>
#include <mp.h>
#include <libsec.h>
#include <auth.h>
#include <ctype.h>

char *
strupr(char *s)
{
	char *p;
	for (p = s; *p; p++)
		if (islower(*p))
			*p = toupper(*p);
	return s;
}

		
/* NT LM 0.12 challenge/response from cifs2.txt, §2.10 -- belongs in Factotum */
static void
hash(uchar pass[16], uchar c8[8], uchar p24[24])
{
	int i;
	uchar p21[21];
	ulong schedule[32];

	memset(p21, 0, sizeof p21 );
	memmove(p21, pass, 16);

	for(i=0; i<3; i++) {
		key_setup(p21+i*7, schedule);
		memmove(p24+i*8, c8, 8);
		block_cipher(schedule, p24+i*8, 0);
	}
}

void
doNTreply(char *pass, uchar chal[8], uchar reply[24])
{
	int i, n;
	uchar *w, unipass[256];
	uchar digest[MD4dlen];

	Rune r;

	// Standard says unlimited length, experience says 128 max
	if ((n = strlen(pass)) > 128)
		n = 128;

	memset(unipass, 0, sizeof unipass);
	for(i=0, w=unipass; i < n; i++) {
		pass += chartorune(&r, pass);
		*w++ = r & 0xff;
		*w++ = r >> 8;
	}

	memset(digest, 0, sizeof digest);
	md4(unipass, w - unipass, digest, nil);
	hash(digest, chal, reply);
}

void
doLMreply(char *pass, uchar chal[8], uchar reply[24])
{
	int i;
	ulong schedule[32];
	uchar p14[15], p16[16];
	uchar s8[8] = {0x4b, 0x47, 0x53, 0x21, 0x40, 0x23, 0x24, 0x25};

	memset(p14, 0, sizeof p14 -1);		// Spec says space padded, experience says otherwise
	p14[sizeof p14 - 1] = '\0';
	memmove(p14, pass, strlen(pass));
	strupr((char *)p14);			// NT4 requires uppercase, Win XP doesn't care

	for(i=0; i<2; i++) {
		key_setup(p14+i*7, schedule);
		memmove(p16+i*8, s8, 8);
		block_cipher(schedule, p16+i*8, 0);
	}

	hash(p16, chal, reply);
}

dmpkey(char *s, void *v, int n)
{
	int i;
	char *p = v;

	print("%s", s);
	for (i = 0; i < n; i++)
		print("%02x ", *p++);
	print("\n");
}

void
dochap(char *pass, int id, char chal[8], uchar resp[16])
{
	char buf[1+8+255+1];
	int n = strlen(pass);

	*buf = id;
	strcpy(buf+1, pass);
	memmove(buf+1+n, chal, 8);
	md5((uchar*)buf, 1+n+8, resp, nil);
}



void
main(int argc, char *argv[])
{
	int n, i;
	UserPasswd *up;
	Chapreply chapchal;
	MSchapreply mcr;
	char *user;
	uchar lm[64], nt[64], chal[64], chap[64], resp[64];

	if (argc != 3){
		fprint(2, "usage: %s user challange\n", argv[0]);
		exits("usage");
	}

	user = argv[1];
	strncpy((char *)chal, argv[2], sizeof(chal));

	if ((up = auth_getuserpasswd(auth_getkey, "proto=pass service=cifs user=%s", user)) == nil)
		sysfatal("auth_getuserpasswd failed %r");

	doLMreply(up->passwd, chal, lm);
	doNTreply(up->passwd, chal, nt);

	if((n = auth_respond(chal, sizeof(chal), user, strlen(user), &mcr, sizeof(mcr), auth_getkey,
		    "proto=mschap role=client service=cifs user=%s", up->user)) == -1)
			sysfatal("auth_respond failed %r");


	if (memcmp(mcr.NTresp, nt, 24)){
		fprint(2, "FAILED\n");
		dmpkey("my nt:", nt, 24);
		dmpkey("ft nt:", mcr.NTresp, 24);
	}
	else
		fprint(2, "MSCHAP NT ok\n");

	if (memcmp(mcr.LMresp, lm, 24)){
		fprint(2, "FAILED\n");
		dmpkey("my nt:", lm, 24);
		dmpkey("ft nt:", mcr.LMresp, 24);
	}
	else
		fprint(2, "MSCHAP LM ok\n");

	/*********************/

	chapchal.id = 123;
	strncpy(chapchal.resp, (char *)chal, 8);

	if((n = auth_respond(&chapchal, sizeof(chapchal), user, strlen(user), &resp, sizeof(resp), auth_getkey,
		    "proto=chap role=client service=cifs user=%s", up->user)) == -1)
			sysfatal("auth_respond failed %r");

	dochap(up->passwd, chapchal.id, chapchal.resp, chap);

	if (memcmp(chap, resp, 16)){
		fprint(2, "FAILED\n");
		dmpkey("my chap:", chap, 16);
		dmpkey("ft chap:", resp, 16);
	}
	else
		fprint(2, "CHAP ok\n");
}
