#include <u.h>
#include <libc.h>
#include <bio.h>
#include <ndb.h>

#include "../ssh.h"
#include "client.h"

static void client(char *argv[]);
char ask(char *answers, char *question, char *problem, ...);

/* Used by the client */
RSApub	*server_session_key;
RSApub	*server_host_key;
uchar		 server_cookie[8];
char		 server_host_name[64];

char *progname;

RSApriv	*client_session_key;
RSApriv	*client_host_key;

ulong		 protocol_flags;
ulong		 supported_cipher_mask;
ulong		 supported_authentications_mask;

uchar		 session_id[16];
uchar		 session_key[32];
uchar		 session_cipher;

int		encryption = 0;		/* Initially off */
int		compression = 0;	/* Initially off */
int		no_secret_key = 0;
int		crstrip = 0;		/* Strip cr from crlf */
int		verbose = 0;
int		interactive;
int		alwaysrequestpty = 0;
int		cooked = 0;
int		usemenu = 1;

char		*user;
char		*localuser;

int dfdin, dfdout;	/* File descriptor for connection to server */

int DEBUG;

void
main(int argc, char *argv[]) {
	int n, fd;
	FILE *f;
	char *host;
	char *p;

	progname = argv[0];
	interactive = -1;
	fmtinstall('B', mpconv);
	ARGBEGIN {
	case 'd':
		DEBUG = 0xff;
		break;
	case 'A':
		doabort = 1;
		break;
	case 'I':
		interactive = 0;
		break;
	case 'i':
		interactive = 1;
		break;
	case 'C':
		cooked = 1;
		break;
	case 'm':
		usemenu = 0;
		break;
	case 'l':
	case 'u':
		user = ARGF();
		break;
	case 'p':
		alwaysrequestpty++;
		break;
	case 'r':
		crstrip = 1;
		break;
	case 'v':
		verbose++;
	case 'a':
		/* Ignore, I don't know exactly what this means in any case */
	case 'x':
		/* Ignore, we don't do X forwarding */
		break;
	default:
		goto Usage;
	} ARGEND

	if (argc < 1){
	Usage:
		fprint(2, "usage: %s [-[lu] user] [-rvI] hostname [cmd [args]]\n", progname);
		exits("usage");
	}
	host = *argv++;
	if(p = strchr(host, '@')){
		*p++ = 0;
		user = host;
		host = p;
	}

	if(interactive == -1)
		interactive = isatty(0);

	if ((fd = open("/dev/user", OREAD)) < 0)
		error("Can't open /dev/user\n");
	localuser = malloc(32);
	if ((n = read(fd, localuser, 31)) <= 0)
		error("Can't read /dev/user\n");
	localuser[n] = 0;
	if (user == nil) {
		user = localuser;
	}

	mkcrctab(0xedb88320);
	srand(time(0));

	client_host_key = rsaprivalloc();
	if ((f = fopen(HOSTKEYPRIV, "r")) == nil) {
		no_secret_key = 1;
		if (verbose)
			fprint(2, "Ssh will not authenticate local host: no access to its secret key\n");
	} else {
		if (readsecretkey(f, client_host_key) == 0)
			error("%s: bad secret-key file", HOSTKEYPRIV);
		fclose(f);
	}
	if ((f = fopen(HOSTKEYPUB, "r")) == nil)
		error("%s: %r", HOSTKEYPUB);
	if (readpublickey(f, &client_host_key->pub) == 0)
		error("%s: bad secret-key file", HOSTKEYPUB);
	fclose(f);

	getdomainname(host, server_host_name, sizeof(server_host_name));

	if (verbose)
		fprint(2, "Dialing %s ...", server_host_name);

	dfdin = dial(netmkaddr(host, "tcp", "22"), 0, 0, 0);
	if (dfdin < 0) error("can't dial %s", host);
	dfdout = dfdin;
	if (verbose)
		fprint(2, "connected\n");
	client(argv);
}

static void
client(char *argv[]) {
	char servervsn[512];
	int i, r;
	char userkeyring[64];

	/* First send ID string */
	debug(DBG_PROTO, "Client version: %s%s\n", PROTSTR, PROTVSN);
	for (i = 0;;i++)  {
		if (read(dfdin, &servervsn[i], 1) <= 0)
			error("Server input: %r");
		if (servervsn[i] == '\n') break;
		if (i >= 256)
			error("Server version string too long");
	}
	debug(DBG_PROTO, "Server version: %s", servervsn);
	fprint(dfdout, "%s%s\n", PROTSTR, PROTVSN);
	if (strncmp(PROTSTR, servervsn, strlen(PROTSTR)))
	    error("%s: Protocol mismatch, Client: %s%s, Server: %s",
		PROTSTR, PROTVSN, servervsn);

	get_ssh_smsg_public_key();
	if (verbose)
		fprint(2, "Received server keys; checking ...\n");
	snprint(userkeyring, sizeof(userkeyring), "/usr/%s/lib/keyring", localuser);
/*	Return values:	key_ok, key_wrong, key_notfound, key_file */
	r = verify_key(server_host_name, server_host_key, userkeyring);
	switch (r) {
	case key_wrong:
		switch(ask("eri", "Replace (r), Ignore (i), or Exit(e) [e]",
			"Host key received differs from that in %s\n", userkeyring)) {
		case 'r':
			replace_key(server_host_name, server_host_key, userkeyring);
		case 'i':
			break;
		default:
		case 'e':
			exits("Bad key");
		}
	case key_notfound:
		if (verify_key(server_host_name, server_host_key, HOSTKEYRING) 
		    == key_ok)
			break;
		i = ask("cea", "Exit (e), Add (a), or Continue (c) [c]: ",
			"Can't find %s's host key on the key ring,\n",
		    server_host_name);
		switch(i) {
		default:
		case 'e':
			exits("Insecure host");
		case 'a':
			replace_key(server_host_name, server_host_key, userkeyring);
		case 'c':
			break;
		}
		break;
	case key_file:
		if (verify_key(server_host_name, server_host_key, HOSTKEYRING) 
		    == key_ok)
			break;
		i = ask("ice", "Exit (e), Create (c), or Ignore (i) [i]: ",
			"You don't have a key ring,\n");
		switch(i) {
		default:
		case 'e':
			exits("Insecure host");
		case 'c':
			if ((i = create(userkeyring, OTRUNC|OWRITE, 0644)) < 0)
				error(" Can't create %s\n", userkeyring);
			close(i);
			replace_key(server_host_name, server_host_key, userkeyring);
		case 'i':
			break;
		}
	case key_ok:	break;
	}
	if (verbose)
		fprint(2, "Key is ok, generating session key\n");
	comp_sess_id(server_host_key->n, server_session_key->n,
		server_cookie, session_id);

	debug(DBG_CRYPTO, "Session ID: ");
	for (i = 0; i < 16; i++) debug(DBG_CRYPTO, "%2.2x", session_id[i]);
	debug(DBG_CRYPTO, "\n");

	/* Create session key */
	genrandom(session_key, sizeof(session_key));
	
	debug(DBG_CRYPTO, "Session key is ");
	for (i = 0; i < 32; i++)
		debug(DBG_CRYPTO, "%2.2ux", session_key[i]);
	debug(DBG_CRYPTO, "\n");


	if (supported_cipher_mask & (1 << SSH_CIPHER_RC4)) {
		session_cipher = SSH_CIPHER_RC4;
		if (verbose)
			fprint(2, "Using RC4 for data encryption\n");
	} else if (supported_cipher_mask & (1 << SSH_CIPHER_3DES)) {
		session_cipher = SSH_CIPHER_3DES;
		if (verbose)
			fprint(2, "Using Triple-DES for data encryption\n");
	} else
		error("Can't agree on ciphers: 0x%x", supported_cipher_mask);

	put_ssh_cmsg_session_key();
	encryption = session_cipher;
	preparekey();
	get_ssh_smsg_success();
	if (verbose)
		fprint(2, "Encrypted connection established\n");
	debug(DBG, "Connection established\n");
	put_ssh_cmsg_user();
	user_auth();
	run_shell(argv);
}

char
ask(char *answers, char *question, char *problem, ...) {
	int cons, r;
	char buf[256];
	va_list arg;

	if(interactive == 0)
		return -1;

	if ((cons = open("/dev/cons", ORDWR)) < 0) return 0;
	r = -1;
	va_start(arg, problem);
	doprint(buf, buf+sizeof(buf), problem, arg);
	va_end(arg);
	fprint(cons, buf);
	while (r == -1) {
		fprint(cons, question);
		if (read(cons, buf, 256) <= 0) r = 0;
		else if (buf[0] == '\n') r = answers[0];
		else if (strchr(answers, buf[0])) r = buf[0];
	}
	close(cons);
	return r;
}
