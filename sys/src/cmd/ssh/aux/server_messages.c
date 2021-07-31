#include <u.h>
#include <libc.h>
#include <auth.h>

#include "../ssh.h"
#include "server.h"

/* Send the SSH_SMSG_PUBLIC_KEY message */
void
put_ssh_smsg_public_key(void) {
	Packet *packet;

	packet = (Packet *)mmalloc(1024);
	packet->type = SSH_SMSG_PUBLIC_KEY;
	packet->length = 0;
	packet->pos = packet->data;

	putbytes(packet, server_cookie, 8);

	debug(DBG_CRYPTO, "Sending session key ");
	debugbig(DBG_CRYPTO, server_session_key->pub.ek, ' ');
	debugbig(DBG_CRYPTO, server_session_key->pub.n, '\n');
	debug(DBG_CRYPTO, "Sending host key ");
	debugbig(DBG_CRYPTO, server_host_key->pub.ek, ' ');
	debugbig(DBG_CRYPTO, server_host_key->pub.n, '\n');

	putRSApublic(packet, &server_session_key->pub);
	putRSApublic(packet, &server_host_key->pub);
	putlong(packet, protocol_flags);
	putlong(packet, supported_cipher_mask);
	putlong(packet, supported_authentications_mask);
	debug(DBG_PACKET, "Packet Length: %d\n", packet->length);
	putpacket(packet, s2c);
}

/* Get the SSH_CMSG_SESSION_KEY message */
void
get_ssh_cmsg_session_key(void) {
	RSApriv *ksmall, *kbig;
	Packet *packet;
	uchar cookie[8];
	int prot_flags;
	mpint *b, *ob;
	int i, s, session_key_len, host_key_len;

	packet = getpacket(c2s);
	if (packet == 0)
		exits("Connection lost");
	if (packet->type != SSH_CMSG_SESSION_KEY)
		syserror("get_ssh_cmsg_session_key: wrong message %d",
			packet->type);
	session_cipher = getbyte(packet);
	debug(DBG_PROTO|DBG_CRYPTO, "Cipher type is %s\n", cipher_names[session_cipher]);	
	getbytes(packet, cookie, 8);
	if (strncmp((char *)cookie, (char *)server_cookie, 8))
		syserror("get_ssh_cmsg_session_key: bad cookie\n");

	session_key_len = mpsignif(server_session_key->pub.n);
	host_key_len = mpsignif(server_host_key->pub.n);

	ksmall = kbig = nil;
	if(session_key_len+128 <= host_key_len) {
		ksmall = server_session_key;
		kbig = server_host_key;
	} else if(host_key_len+128 <= session_key_len) {
		ksmall = server_host_key;
		kbig = server_session_key;
	} else
		error("Server session and host keys must differ by at least 128 bits\n");

	b = getBigInt(packet);
	b = RSADecrypt(b, kbig);
	s = (mpsignif(ksmall->pub.n)+7)/8;
	debug(DBG_CRYPTO, "decrypted %B, unpadding %d\n", b, s);
	ob = b;
	b = RSAunpad(ob, s);
	mpfree(ob);
	debug(DBG_CRYPTO, "unpadded %B\n", b);
	b = RSADecrypt(b, ksmall);
	debug(DBG_CRYPTO, "decrypted %B, unpadding %d\n", b, SESSION_KEY_LENGTH);
	ob = b;
	b = RSAunpad(b, SESSION_KEY_LENGTH);
	mpfree(ob);
	debug(DBG_CRYPTO, "unpadded %B\n", b);
	mptobe(b, session_key, SESSION_KEY_LENGTH, nil);

	debug(DBG_CRYPTO, "Raw session key is ");
	for (i = 0; i < SESSION_KEY_LENGTH; i++)
		debug(DBG_CRYPTO, "%2.2x", session_key[i]);
	debug(DBG_CRYPTO, "\n");

	for (i = 0; i < SESSION_ID_LENGTH; i++)
		session_key[i] ^= session_id[i];
	
	mpfree(b);

	debug(DBG_CRYPTO, "Cleaned up session key is ");
	for (i = 0; i < SESSION_KEY_LENGTH; i++)
		debug(DBG_CRYPTO, "%2.2x", session_key[i]);
	debug(DBG_CRYPTO, "\n");

	prot_flags = getlong(packet);

	debug(DBG_PROTO, "Protocol flags: 0x%x\n", prot_flags);
	mfree(packet);
}

void
put_ssh_smsg_success(void) {
	Packet *packet;

	packet = (Packet *)mmalloc(sizeof(*packet)+4);
	packet->type = SSH_SMSG_SUCCESS;
	packet->length = 0;
	putpacket(packet, s2c);
}


void
put_ssh_smsg_failure(void) {
	Packet *packet;

	packet = (Packet *)mmalloc(sizeof(*packet)+4);
	packet->type = SSH_SMSG_FAILURE;
	packet->length = 0;
	putpacket(packet, s2c);
}

/* Get the SSH_CMSG_USER message */
void
get_ssh_cmsg_user(void) {
	Packet *packet;

	packet = getpacket(c2s);
	if (packet == 0)
		exits("Connection lost");
	if (packet->type != SSH_CMSG_USER)
		syserror("get_ssh_cmsg_user: wrong message %d",
			packet->type);
	getstring(packet, user, sizeof(user));
	debug(DBG_AUTH, "User is %s\n", user);
	mfree(packet);
}

int
ssh_smsg_auth_rsa_challenge(void) {
	Packet *packet;
	uchar challenge[32+16];
	uchar cryptobuf[256];
	mpint *cryptonum, *b;
	int i, cryptobytes;

	packet = (Packet *)mmalloc(sizeof(*packet)+256);
	packet->type = SSH_SMSG_AUTH_RSA_CHALLENGE;
	packet->length = 0;
	packet->pos = packet->data;
	for (i=0; i<32; i++)
		challenge[i] = nrand(0x100);
	cryptobytes = (mpsignif(client_host_key->n)+7)>>3;
	RSApad(challenge, 32, cryptobuf, cryptobytes);
	cryptonum = betomp(cryptobuf, cryptobytes, nil);
	b = RSAEncrypt(cryptonum, client_host_key);
	putBigInt(packet, b);
	putpacket(packet, s2c);
	memcpy(challenge+32, session_id, 16);
	md5(challenge, 32+16, cryptobuf, 0);
	packet = getpacket(c2s);
	if (packet == 0)
		exits("Connection lost");
	if (packet->type != SSH_CMSG_AUTH_RSA_RESPONSE)
		syserror("ssh_smsg_auth_rsa_challenge: wrong message %d",
			packet->type);
	if (packet->length < 16 || memcmp(cryptobuf, packet->pos, 16)) {
		fprint(2, "Bad response, packet->length = %lud\n",
			packet->length);
		debug(DBG_AUTH|DBG_CRYPTO,"Bad response, packet->length = %d\n",
			packet->length);
		return 0;
	}
	return 1;
}

int
ssh_smsg_auth_tis(void) {
	Packet *packet;
	char challenge[64];
	char response[256];
	Chalstate c;
	char *ns;

	if (getchal(&c, user) < 0)
		return 0;
	snprint(challenge, sizeof(challenge), "Challenge: %s\nResponse: ", c.chal);
	packet = (Packet *)mmalloc(sizeof(*packet)+64);
	packet->type = SSH_SMSG_AUTH_TIS_CHALLENGE;
	packet->length = 0;
	packet->pos = packet->data;
	putstring(packet, challenge, strlen(challenge));
	putpacket(packet, s2c);
	packet = getpacket(c2s);
	if (packet == 0) {
		syslog(0, syslogfile, "Connection lost");
		exits("Connection lost");
	}
	if (packet->type != SSH_CMSG_AUTH_TIS_RESPONSE) {
		syserror("ssh_smsg_auth_tis_challenge: wrong message %d",
			packet->type);
	}
	getstring(packet, response, 256);
	mfree(packet);
	debug(DBG_AUTH, "Response: %s\n", response);
	if (chalreply(&c, response) < 0)
		return 0;
	if(noworld(user))
		ns = "/lib/namespace.noworld";
	else
		ns = nil;
	newns(user, ns);
	return 1;
}

void
get_ssh_cmsg_exit_confirmation(void) {
	Packet *packet;

	packet = getpacket(c2s);
	if (packet == 0)
		exits("Connection lost");
	if (packet->type != SSH_CMSG_EXIT_CONFIRMATION)
		syserror("get_ssh_cmsg_exit_confirmation: wrong message %d",
			packet->type);
	mfree(packet);
}

void
user_auth(void) {
	Packet *packet;
	int auth_tricks;
	char *ns;

	packet = 0;
	auth_tricks = 0;
	while((auth_tricks & ((1 << SSH_AUTH_PASSWORD)|(1 << SSH_AUTH_TIS))) == 0){
		debug(DBG_AUTH, "auth_tricks = 0x%x\n", auth_tricks);
		if (packet) {
			put_ssh_smsg_failure();
			mfree(packet);
		}
		packet = getpacket(c2s);
		if (packet == 0)
			exits("Connection lost");
		switch (packet->type) {
		case SSH_CMSG_AUTH_RHOSTS:
			debug(DBG_AUTH, "Got SSH_CMSG_AUTH_RHOSTS\n");
			debug(DBG_AUTH, "SSH_AUTH_RHOSTS succeeded\n");
			auth_tricks |= 1 << SSH_AUTH_RHOSTS;
			break;
		case SSH_CMSG_AUTH_RSA:
			debug(DBG_AUTH, "Got SSH_CMSG_AUTH_RSA (always fails)\n");
			break;
		case SSH_CMSG_AUTH_PASSWORD:
			if((supported_authentications_mask & (1 << SSH_AUTH_PASSWORD)) == 0){
				debug(DBG_AUTH, "Got SSH_CMSG_AUTH_PASSWORD not supported\n");
				continue;
			}
			debug(DBG_AUTH, "Got SSH_CMSG_AUTH_PASSWORD\n");
			getstring(packet, passwd, sizeof(passwd));
			if(noworld(user))
				ns = "/lib/namespace.noworld";
			else
				ns = nil;
			if (login(user, passwd, ns) < 0) {
				syslog(0, syslogfile,
					"Bad password: %s@%s",
					user, client_host_name);
				break;
			}
			debug(DBG_AUTH, "Password is good\n");
			auth_tricks |= 1 << SSH_AUTH_PASSWORD;
			break;
		case SSH_CMSG_AUTH_RHOSTS_RSA:
			debug(DBG_AUTH, "Got SSH_CMSG_AUTH_RHOSTS_RSA (always fails)\n");
			break;
		case SSH_CMSG_AUTH_TIS:
			debug(DBG_AUTH, "Got SSH_CMSG_AUTH_TIS\n");
			if (ssh_smsg_auth_tis() == 0) {
				syslog(0, syslogfile,
					"Bad TIS response from %s@%s",
					user, client_host_name);
				break;
			}
			debug(DBG_AUTH|DBG_CRYPTO,"TIS Response is good\n");
			auth_tricks |= 1 << SSH_AUTH_TIS;
			break;
		default:
			syserror("user_auth: unknown message %d",
				packet->type);
		}
	}
	debug(DBG_AUTH, "Login succeeded\n");
	syslog(0, syslogfile, "Connection established: %s@%s",
		user, client_host_name);
	put_ssh_smsg_success();
	mfree(packet);
}


void
start_work(void) {
	Packet *packet;
	char cmd[1024];
	ulong max;

	while(1) {
		packet = getpacket(c2s);
		if (packet == 0)
			exits("Connection lost");
		switch (packet->type) {
		case SSH_MSG_DISCONNECT:
			syserror("Client disconnects");
		case SSH_CMSG_REQUEST_PTY:
			put_ssh_smsg_success();
			break;
		case SSH_CMSG_X11_REQUEST_FORWARDING:
			put_ssh_smsg_failure();
			break;
		case SSH_CMSG_EXEC_SHELL:
			run_cmd(0);
			mfree(packet);
			break;
		case SSH_CMSG_EXEC_CMD:
			getstring(packet, cmd, sizeof(cmd));
			debug(DBG, "Command is %s\n", cmd);
			run_cmd(cmd);
			mfree(packet);
			break;
		case SSH_CMSG_MAX_PACKET_SIZE:
			max = getlong(packet);
			put_ssh_smsg_success();
			syslog(0, syslogfile, "Max packet size %uld", max);
			break;
		case SSH_CMSG_REQUEST_COMPRESSION:
			put_ssh_smsg_failure();
			break;
		default:
			syserror("start_work: unknown message %d",
				packet->type);
		}
		mfree(packet);
	}
}

/* No longer used
int
rshostslookup(char *rshosts) {
	FILE *f;
	char line[256], *p;
	char shosts[32];

	snprint(shosts, sizeof(shosts), "/usr/%s/lib/%s", user, rshosts);
	if ((f = fopen(shosts, "r")) == nil) {
		debug(DBG_AUTH, "No %s file\n", shosts);
		return 0;
	}
	while (fgets(line, sizeof(line), f)) {
		if (line[0] == '#' || line[0] == '\n') continue;
		p = strtok(line, " \t");
		if (p == 0 || strcmp(p, client_host_name))
			continue;
		p = strtok(0, " \t\n");
		if (p ==0 || strcmp(p, user))
			continue;
		if (strtok(0, " \t\n")) {
			debug(DBG_AUTH, "Malformed %s in %s\n", line, shosts);
			continue;
		}
		return 1;
	}
	return 0;
}
*/
