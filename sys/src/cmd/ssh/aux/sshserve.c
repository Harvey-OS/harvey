#include <u.h>
#include <libc.h>
#include <bio.h>	/* for ndb.h */

#include "../ssh.h"
#include "server.h"

char *syslogfile = "ssh";

char *progname;

uchar		server_cookie[8];
ulong		protocol_flags;
ulong		supported_cipher_mask = 0
/* Not supported	| (1 << SSH_CIPHER_IDEA) */
/* Supported: */	| (1 << SSH_CIPHER_DES)
/* Supported: */	| (1 << SSH_CIPHER_3DES)
/* Not supported	| (1 << SSH_CIPHER_TSS) */
/* Supported: */	| (1 << SSH_CIPHER_RC4) ;

ulong		supported_authentications_mask = (1 << SSH_AUTH_RSA)|(1 << SSH_AUTH_TIS);

/* Used by the server */
RSApub	*client_sess_key;
RSApub	*client_host_key;
char		 client_host_name[64];
char		*client_host_ip;

RSApriv	*server_session_key;
RSApriv	*server_host_key;

uchar		 session_id[16];
uchar		 session_key[32];
uchar		 session_cipher;

int		encryption = 0;		/* Initially off */
int		compression = 0;	/* Initially off */

char		user[16];
char		passwd[256];

int dfdin, dfdout;	/* File descriptors for TCP communications */

void
main(int argc, char *argv[]) {
	char clientvsn[512];
	int i;
	char *p;
	FILE *f;

	fmtinstall('B', mpconv);
	ARGBEGIN{
	case 'd':
		DEBUG = 0xff;
		break;
	case 'p':
		supported_authentications_mask |= (1 << SSH_AUTH_PASSWORD);
		break;
	}ARGEND;

	if (argc == 0) syserror("%s called without client-ip argument");
	if (argc > 1) syserror("%s called with too many arguments");

	progname = argv0;

	syslog(0, syslogfile, "connect request: %s", argv[0]);
	client_host_ip = argv[0];

	if (p = strchr(client_host_ip, '!')) *p = 0;
	getdomainname(client_host_ip, client_host_name, sizeof(client_host_name));

	/* Read in the host key */
	server_host_key = rsaprivalloc();
	if ((f = fopen(HOSTKEYPRIV, "r")) == nil)
		syserror("%s: %r", HOSTKEYPRIV);
	if (readsecretkey(f, server_host_key) == 0)
		syserror("%s: bad secret-key file", HOSTKEYPRIV);
	fclose(f);
	if ((f = fopen(HOSTKEYPUB, "r")) == nil)
		syserror("%s: %r", HOSTKEYPUB);
	if (readpublickey(f, &server_host_key->pub) == 0)
		syserror("%s: bad secret-key file", HOSTKEYPUB);
	fclose(f);
	server_session_key = rsagen(768, 6, 0);
	debug(DBG_PROTO, "rsagen done %d\n", mpsignif(server_session_key->pub.n));

	mkcrctab(0xedb88320);
	srand(time(0));

	dfdin = 0; dfdout = 1;
	/* First send ID string */
	debug(DBG_PROTO, "Server version: %s%s\n", PROTSTR, PROTVSN);
	fprint(dfdout, "%s%s\n", PROTSTR, PROTVSN);
	for (i = 0;;i++)  {
		if (read(dfdin, &clientvsn[i], 1) <= 0)
			syserror("main: read: %r");
		if (clientvsn[i] == '\n') break;
		if (i >= 256)
			syserror("Client version string too long");
	}
	clientvsn[i] = 0;
	debug(DBG_PROTO, "Client version: %s\n", clientvsn);
	if (strncmp(PROTSTR, clientvsn, strlen(PROTSTR)))
	    syserror("Protocol mismatch, Server: %s%s, Client: %s",
		PROTSTR, PROTVSN, clientvsn);

	srand(time(0));
	for (i = 0; i < 8; i++) server_cookie[i] = nrand(0x100);

	put_ssh_smsg_public_key();
	comp_sess_id(server_host_key->pub.n,
		server_session_key->pub.n,
		server_cookie, session_id);

	debug(DBG_CRYPTO, "Session ID: ");
	for (i = 0; i < 16; i++) debug(DBG_CRYPTO, "%2.2x", session_id[i]);
	debug(DBG_CRYPTO, "\n");

	get_ssh_cmsg_session_key();
	encryption = session_cipher;
	preparekey();
	put_ssh_smsg_success();
	get_ssh_cmsg_user();
	put_ssh_smsg_failure();
	user_auth();
	start_work();
}

void
syserror(char *fmt, ...) {
	va_list arg;
	char buf[2048];

	va_start(arg, fmt);
	doprint(buf, buf+sizeof(buf), fmt, arg);
	va_end(arg);
	syslog(0, syslogfile, buf);
	exits("Failure");
}
