#include <u.h>
#include <libc.h>

#include "../ssh.h"
#include "client.h"

/* Get the SSH_SMSG_PUBLIC_KEY message */
void
get_ssh_smsg_public_key(void) {
	Packet *packet;
	int i;

	packet = getpacket(s2c);
	if (packet == 0)
		exits("Connection lost");
	if (packet->type != SSH_SMSG_PUBLIC_KEY)
		error("Wrong message, expected %d, got %d",
			SSH_SMSG_PUBLIC_KEY, packet->type);
	if ((packet->length -= 8) < 0) error("Packet too small");
	memcpy(server_cookie, packet->data, 8);
	packet->pos += 8;
	server_session_key = getRSApublic(packet);
	server_host_key = getRSApublic(packet);

	debug(DBG_CRYPTO, "Receiving session key ");
	debugbig(DBG_CRYPTO, server_session_key->ek, ' ');
	debugbig(DBG_CRYPTO, server_session_key->n, '\n');
	debug(DBG_CRYPTO, "Receiving host key ");
	debugbig(DBG_CRYPTO, server_host_key->ek, ' ');
	debugbig(DBG_CRYPTO, server_host_key->n, '\n');

	protocol_flags = getlong(packet);
	debug(DBG_PROTO, "Protocol flags %10lx\n", protocol_flags);	
	supported_cipher_mask = getlong(packet);

	debug(DBG_PROTO, "Supported ciphers:\n");
	for (i = 0; cipher_names[i]; i++)
		if ((1 << i) & supported_cipher_mask)
			debug(DBG_PROTO, "\t%s\n", cipher_names[i]);

	supported_authentications_mask = getlong(packet);

	debug(DBG_PROTO|DBG_AUTH, "Supported authentication methods:\n");
	for (i = 0; auth_names[i]; i++)
		if ((1 << i) & supported_authentications_mask)
			debug(DBG_PROTO|DBG_AUTH, "\t%s\n", auth_names[i]);
	debug(DBG_PACKET, "Remaining Length: %d\n", packet->length);

	mfree(packet);
}


static mpint*
rsa_encrypt_buf(uchar *buf, int n, RSApub *key)
{
	int m;
	mpint *b;

	m = (mpsignif(key->n)+7)/8;
	RSApad(buf, n, buf, m);
	b = betomp(buf, m, nil);
	debug(DBG_CRYPTO, "padded %B\n", b);
	b = RSAEncrypt(b, key);
	debug(DBG_CRYPTO, "encrypted %B\n", b);
	return b;
}

/* Send the SSH_CMSG_SESSION_KEY message */
void
put_ssh_cmsg_session_key(void) {
	Packet *packet;
	RSApub *kbig, *ksmall;
	mpint *b;
	int i, n, session_key_len, host_key_len;
	uchar *buf;

	packet = (Packet *)mmalloc(2048);
	packet->type = SSH_CMSG_SESSION_KEY;
	packet->length = 0;
	packet->pos = packet->data;

	putbyte(packet, session_cipher);
	debug(DBG_PROTO|DBG_AUTH, "Cipher type is %s\n",
		cipher_names[session_cipher]);	
	putbytes(packet, server_cookie, 8);

	session_key_len = mpsignif(server_session_key->n);
	host_key_len = mpsignif(server_host_key->n);

	ksmall = kbig = nil;
	if(session_key_len+128 <= host_key_len) {
		ksmall = server_session_key;
		kbig = server_host_key;
	} else if(host_key_len+128 <= session_key_len) {
		ksmall = server_host_key;
		kbig = server_session_key;
	} else
		error("Server session and host keys must differ by at least 128 bits\n");

	n = (mpsignif(kbig->n)+7)/8;
	buf = mmalloc(n);

	memmove(buf, session_key, SESSION_KEY_LENGTH);
	for(i = 0; i < SESSION_ID_LENGTH; i++)
		buf[i] ^= session_id[i];

	b = rsa_encrypt_buf(buf, SESSION_KEY_LENGTH, ksmall);
	n = (mpsignif(ksmall->n)+7) / 8;
	mptobuf(b, buf, n);
	mpfree(b);

	b = rsa_encrypt_buf(buf, n, kbig);
	putBigInt(packet, b);
	mpfree(b);

	n = (mpsignif(kbig->n)+7)/8;
	memset(buf, 0, n);
	free(buf);

	putlong(packet, 0);	/* Protocol flags */

	putpacket(packet, c2s);
}

void
get_ssh_smsg_success(void) {
	Packet *packet;

	packet = getpacket(s2c);
	if (packet == 0)
		error("Connection lost");
	if (packet->type != SSH_SMSG_SUCCESS)
		error("Wrong message, expected %d, got %d",
			SSH_SMSG_SUCCESS, packet->type);
	mfree(packet);
}

/* Send the SSH_CMSG_USER message */
void
put_ssh_cmsg_user(void) {
	Packet *packet;

	packet = (Packet *)mmalloc(1024);
	packet->type = SSH_CMSG_USER;
	packet->length = 0;
	packet->pos = packet->data;

	debug(DBG_PROTO|DBG_AUTH, "Sending User message, user is %s\n", user);

	putstring(packet, user, strlen(user));
	putpacket(packet, c2s);
}

int authorder[] = {
	(1 << SSH_AUTH_RSA),
	(1 << SSH_AUTH_USER_RSA),
	(1 << SSH_AUTH_TIS),
	(1 << SSH_AUTH_PASSWORD),
	(1 << SSH_AUTH_RHOSTS_RSA),
	(1 << SSH_AUTH_RHOSTS),
	0
};

void
user_auth(void) {
	Packet *packet;
	uint i, auth_tricks;
	int consctl, cons, n;
	char passwd[20];
	mpint *challenge;
	uchar chalbuf[48], response[16];
	int method;
	FILE *userkeys;
	char userkeyfile[128];
	RSApriv *userkey;

	auth_tricks = 
		(1 << SSH_AUTH_RHOSTS) |
		(1 << SSH_AUTH_RSA) |
		(1 << SSH_AUTH_PASSWORD) |
		(1 << SSH_AUTH_TIS) |
		(no_secret_key?0:(1 << SSH_AUTH_RHOSTS_RSA));

	snprint(userkeyfile, sizeof userkeyfile,
		"/usr/%s/lib/userkeyring", localuser);
	auth_tricks &= supported_authentications_mask;
	if ((userkeys = fopen(userkeyfile, "r")))
		auth_tricks |= (1 << SSH_AUTH_USER_RSA);
	if (interactive == 0)
		auth_tricks &= ~((1 << SSH_AUTH_PASSWORD)|(1 << SSH_AUTH_TIS));

	method = 0;

	/* Can't do RSA host authentication without the secret key */
	while(1) {
		packet = getpacket(s2c);
		if (packet == 0)
			exits("Connection lost");
		switch (packet->type) {
		case SSH_SMSG_SUCCESS:
			if (verbose)
				fprint(2, "User successfully authenticated\n");
			debug(DBG_AUTH, "User_auth done\n");
			mfree(packet);
			if (userkeys)
				fclose(userkeys);
			return;
		case SSH_SMSG_FAILURE:
				debug(DBG_AUTH, "Received FAILURE\n");
		fail:	mfree(packet);
			for(;; method++){
				if(authorder[method] == 0) {
					if (verbose)
						fprint(2, "User not authenticated, connection refused\n");
					else
						fprint(2, "Connection refused\n");
					exits("Connection refused");
				}
				if(auth_tricks & authorder[method])
					break;
			}
			i = authorder[method];
			packet = (Packet *)mmalloc(1024);
			packet->length = 0;
			packet->pos = packet->data;
		
			switch(i) {
			case (1<<SSH_AUTH_RHOSTS):
				packet->type = SSH_CMSG_AUTH_RHOSTS;
				debug(DBG_AUTH, "Sending SSH_CMSG_AUTH_RHOSTS\n");
				putstring(packet, user, strlen(user));
				method++;
				break;
			case (1<<SSH_AUTH_RSA):
				if (verbose)
					fprint(2, "Authenticating client host\n");
				packet->type = SSH_CMSG_AUTH_RSA;
				debug(DBG_AUTH, "Sending SSH_CMSG_AUTH_RSA\n");
				putBigInt(packet,
					client_host_key->pub.n);
				method++;
				break;
			case (1<<SSH_AUTH_PASSWORD):
				assert(interactive);
				packet->type = SSH_CMSG_AUTH_PASSWORD;
				debug(DBG_AUTH, "Sending SSH_CMSG_AUTH_PASSWORD\n");
				if ((cons = open("/dev/cons", ORDWR)) < 0)
					error("Can't open console");
				if ((consctl=open("/dev/consctl", OWRITE)) < 0)
					error("Can't turn off echo");
				fprint(consctl, "rawon");
				fprint(cons, "%s@%s's password: ", user, server_host_name);
				for (n = 0; n < 20; n++) {
					if (read(cons, passwd+n, 1) != 1)
						error("Can't read console");
					if (passwd[n] == '\b') n -= 2;
					if (passwd[n] == 0x15) n = 0;
					if (passwd[n] == '\n') break;
					if (passwd[n] == '\r') break;
					if (n < 0) n = 0;
				}
				passwd[n] = 0;
				fprint(consctl, "rawoff");
				fprint(cons, "\n");
				close(cons); close(consctl);
				putstring(packet, passwd, n);
				if (verbose)
					fprint(2, "Sending (encrypted) password\n");
				method++;
				break;
			case (1<<SSH_AUTH_RHOSTS_RSA):
				if (verbose)
					fprint(2, "Authenticating host by responding to RSA challenge\n");
				packet->type = SSH_CMSG_AUTH_RHOSTS_RSA;
				debug(DBG_AUTH, "Sending SSH_CMSG_AUTH_RHOSTS_RSA\n");
				putstring(packet, user, strlen(user));
				putRSApublic(packet, &client_host_key->pub);
				putpacket(packet, c2s);

				packet = getpacket(s2c);
				if (packet == 0)
					error("Connection lost");
				if (packet->type == SSH_SMSG_FAILURE){
					method++;
					goto fail;
				}
				if (packet->type != SSH_SMSG_AUTH_RSA_CHALLENGE)
					error("Expected Challenge, got %d\n",
						packet->type);
				challenge = getBigInt(packet);
				mfree(packet);
				challenge = RSADecrypt(challenge,
					client_host_key);
				mptobuf(RSAunpad(challenge, 32), chalbuf, 32);
				memcpy(chalbuf+32, session_id, 16);
				md5(chalbuf, 32+16, response, 0);
								
				packet = (Packet *)mmalloc(1024);
				packet->type = SSH_CMSG_AUTH_RSA_RESPONSE;
				packet->length = 0;
				packet->pos = packet->data;
				putbytes(packet, response, 16);
				method++;
				break;
			case (1<<SSH_AUTH_USER_RSA):
				userkey = rsaprivalloc();
				if (readsecretkey(userkeys, userkey) == 0) {
					rsaprivfree(userkey);
					fclose(userkeys);
					userkeys = nil;
					method++;
					goto fail;
				}
				if (verbose)
					fprint(2, "Authenticating user by responding to RSA challenge\n");
				packet->type = SSH_CMSG_AUTH_RSA;
				putBigInt(packet, userkey->pub.n);
				debug(DBG_AUTH, "Sending SSH_CMSG_AUTH_RSA\n");
				debug(DBG_AUTH, "Asking server about modulus %B\n", userkey->pub.n);
				putpacket(packet, c2s);

				packet = getpacket(s2c);
				if (packet == 0)
					error("Connection lost");
				if (packet->type == SSH_SMSG_FAILURE) {
					debug(DBG_AUTH, "Recievd SSH_SMSG_FAILURE\n");
					goto fail;
				}
				if (packet->type != SSH_SMSG_AUTH_RSA_CHALLENGE)
					error("Expected Challenge, got %d\n", packet->type);
				challenge = getBigInt(packet);
				mfree(packet);
				challenge = RSADecrypt(challenge, userkey);
				mptobuf(RSAunpad(challenge, 32), chalbuf, 32);
				memcpy(chalbuf+32, session_id, 16);
				md5(chalbuf, 32+16, response, 0);

				packet = (Packet *)mmalloc(1024);
				packet->type = SSH_CMSG_AUTH_RSA_RESPONSE;
				packet->length = 0;
				packet->pos = packet->data;
				putbytes(packet, response, 16);
				debug(DBG_AUTH, "Sending SSH_CMSG_AUTH_RSA_RESPONSE\n");
				break;
			case (1<<SSH_AUTH_TIS):
				assert(interactive);
				packet->type = SSH_CMSG_AUTH_TIS;
				if (packet == 0)
					error("Connection lost");
				debug(DBG_AUTH, "Sending SSH_CMSG_AUTH_TIS\n");
				putpacket(packet, c2s);
				packet = getpacket(s2c);
				if (packet == 0)
					error("Connection lost");
				if (packet->type == SSH_SMSG_FAILURE) {
					method++;
					goto fail;
				}
				if (packet->type != SSH_SMSG_AUTH_TIS_CHALLENGE)
					error("Expected TIS Chal, got %d\n",
						packet->type);
				getstring(packet, (char *)chalbuf, sizeof(chalbuf));
				mfree(packet);
				if ((cons = open("/dev/cons", ORDWR)) < 0)
					error("Can't open console");
				if (chalbuf[sizeof(chalbuf)-1] == '\n')
					chalbuf[sizeof(chalbuf)-1] = 0;
				fprint(cons, "%s", (char*)chalbuf);
				n = read(cons, passwd, sizeof(passwd));
				if (n <= 0) error("Can't read console");
				passwd[n] = 0;
				if (passwd[n-1] == '\n') passwd[--n] = 0;
				close(cons);
				packet = (Packet *)mmalloc(1024);
				packet->type = SSH_CMSG_AUTH_TIS_RESPONSE;
				packet->length = 0;
				packet->pos = packet->data;
				putstring(packet, passwd, n);
				if (verbose)
					fprint(2, "Sending response\n");
				method++;
				break;
			default:
				error("Impossible: %d", i);
			}
			putpacket(packet, c2s);
			break;
		default:
			error("Unknown message type %d", packet->type);
		}
	}
}

static int
int_env(char *key, int dflt)
{
	int ival;
	char *eval;

	eval = getenv(key);
	if (eval != nil && (ival=atoi(eval)) > 0)
		return ival;
	return dflt;
}

void
request_pty(void) {
	Packet *packet;
	char *term;

	if (verbose)
		fprint(2, "Requesting a pseudo tty\n");
	packet = (Packet *)mmalloc(1024);
	packet->type = SSH_CMSG_REQUEST_PTY;
	packet->length = 0;
	packet->pos = packet->data;

	term = getenv("TERM");
	if(term == nil)
		term = "9term";

	putstring(packet, term, strlen(term));
	putlong(packet, int_env("LINES", 24));	/* rows */
	putlong(packet, int_env("COLS", 80));	/* columns */
	putlong(packet, 0);	/* width in pixels; 0 ≡ not supported */
	putlong(packet, 0);	/* height in pixels; 0 ≡ not supported */

	/*
	 * In each option line, the first byte is the option number
	 * and the second is either a boolean bit or an actually 
	 * ASCII code.
	 */
	putbytes(packet, (uchar*)"\x01\x7F", 2);	/* interrupt = DEL */
	putbytes(packet, (uchar*)"\x02\x11", 2);	/* quit = ^Q */
	putbytes(packet, (uchar*)"\x03\x08", 2);	/* backspace = ^H */
	putbytes(packet, (uchar*)"\x04\x15", 2);	/* line kill = ^U */
	putbytes(packet, (uchar*)"\x05\x04", 2);	/* EOF = ^D */
	putbytes(packet, (uchar*)"\x20\x00", 2);	/* don't strip high bit */
	putbytes(packet, (uchar*)"\x48\x01", 2);	/* give us CRs */

	putbytes(packet, (uchar*)"\x00", 1);	/* end of options */

	putpacket(packet, c2s);
	packet = getpacket(s2c);
	if (packet == 0)
		error("Connection lost");
	if (packet->type != SSH_SMSG_SUCCESS)
		debug(DBG_IO, "PTY allocated\n");
	if (packet->type != SSH_SMSG_FAILURE)
		debug(DBG_IO, "PTY allocation failed\n");
	if (packet->type != SSH_SMSG_SUCCESS &&
		packet->type != SSH_SMSG_FAILURE)
		error("Unexpected message %d\n", packet->type);
	mfree(packet);
}
