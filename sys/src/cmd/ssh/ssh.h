#include <mp.h>
#include <libsec.h>
#include <stdio.h>
#pragma	varargck	argpos	error	1

/* File names */
#define HOSTKEYPRIV	"/sys/lib/ssh/hostkey.secret"
#define HOSTKEYPUB	"/sys/lib/ssh/hostkey.public"

/*
Debugging only:
#define HOSTKEYPRIV	"./hostkey.secret"
#define HOSTKEYPUB	"./hostkey.public"
 */

#define HOSTKEYRING	"/sys/lib/ssh/keyring"

#define PROTSTR	"SSH-1.5-"
#define PROTVSN	"Plan9"


#define DBG			0x01
#define DBG_CRYPTO	0x02
#define DBG_PACKET	0x04
#define DBG_AUTH	0x08
#define DBG_PROC	0x10
#define DBG_PROTO	0x20
#define DBG_IO		0x40
#define DBG_SCP		0x80

extern char *progname;

#define SSH_MSG_NONE							0
#define SSH_MSG_DISCONNECT						1
#define SSH_SMSG_PUBLIC_KEY						2
#define SSH_CMSG_SESSION_KEY					3
#define SSH_CMSG_USER							4
#define SSH_CMSG_AUTH_RHOSTS					5
#define SSH_CMSG_AUTH_RSA						6
#define SSH_SMSG_AUTH_RSA_CHALLENGE				7
#define SSH_CMSG_AUTH_RSA_RESPONSE				8
#define SSH_CMSG_AUTH_PASSWORD					9
#define SSH_CMSG_REQUEST_PTY					10
#define SSH_CMSG_WINDOW_SIZE					11
#define SSH_CMSG_EXEC_SHELL						12
#define SSH_CMSG_EXEC_CMD						13
#define SSH_SMSG_SUCCESS						14
#define SSH_SMSG_FAILURE						15
#define SSH_CMSG_STDIN_DATA						16
#define SSH_SMSG_STDOUT_DATA					17
#define SSH_SMSG_STDERR_DATA					18
#define SSH_CMSG_EOF							19
#define SSH_SMSG_EXITSTATUS						20
#define SSH_MSG_CHANNEL_OPEN_CONFIRMATION		21
#define SSH_MSG_CHANNEL_OPEN_FAILURE			22
#define SSH_MSG_CHANNEL_DATA					23
#define SSH_MSG_CHANNEL_CLOSE					24
#define SSH_MSG_CHANNEL_CLOSE_CONFIRMATION		25
/*	(OBSOLETED; was unix-domain X11 forwarding)	26 */
#define SSH_SMSG_X11_OPEN						27
#define SSH_CMSG_PORT_FORWARD_REQUEST			28
#define SSH_MSG_PORT_OPEN						29
#define SSH_CMSG_AGENT_REQUEST_FORWARDING		30
#define SSH_SMSG_AGENT_OPEN						31
#define SSH_MSG_IGNORE							32
#define SSH_CMSG_EXIT_CONFIRMATION				33
#define SSH_CMSG_X11_REQUEST_FORWARDING			34
#define SSH_CMSG_AUTH_RHOSTS_RSA				35
#define SSH_MSG_DEBUG							36
#define SSH_CMSG_REQUEST_COMPRESSION			37
#define SSH_CMSG_MAX_PACKET_SIZE				38
#define SSH_CMSG_AUTH_TIS						39
#define SSH_SMSG_AUTH_TIS_CHALLENGE				40
#define SSH_CMSG_AUTH_TIS_RESPONSE				41
#define SSH_CMSG_AUTH_KERBEROS					42
#define SSH_SMSG_AUTH_KERBEROS_RESPONSE			43
#define SSH_CMSG_HAVE_KERBEROS_TGT				44

#define SSH_CIPHER_NONE							0
#define SSH_CIPHER_IDEA							1
#define SSH_CIPHER_DES							2
#define SSH_CIPHER_3DES							3
#define SSH_CIPHER_TSS							4
#define SSH_CIPHER_RC4							5

extern char	*cipher_names[];

#define SSH_AUTH_RHOSTS							1
#define SSH_AUTH_RSA							2
#define SSH_AUTH_PASSWORD						3
#define SSH_AUTH_RHOSTS_RSA						4
#define SSH_AUTH_TIS							5
#define SSH_AUTH_USER_RSA						6

extern char	*auth_names[];

#define SSH_PROTOCOL_SCREEN_NUMBER				1
#define SSH_PROTOCOL_HOST_IN_FWD_OPEN			2

typedef struct Packet {
	uchar	*pos;	/* position in packet (while reading or writing */
	ulong	length;	/* output: #bytes before pos, input: #bytes after pos */
	uchar	pad[8];
	uchar	type;
	uchar	data[1];
} Packet;

#define SSH_MAX_DATA	262144	/* smallest allowed maximum packet size */
#define SSH_MAX_MSG	SSH_MAX_DATA+4

#define SESSION_KEY_LENGTH						32
#define SESSION_ID_LENGTH 						MD5dlen

/* Used by both */
extern uchar		session_key[SESSION_KEY_LENGTH];
extern uchar		session_id[SESSION_ID_LENGTH];
extern uchar		session_cipher;	/* Cipher used for data encryption */

extern int		encryption;
extern int		compression;
extern int		DEBUG;
extern int		doabort;

extern char		passwd[256];

extern int	dfdin, dfdout;	/* The all-important file descriptors */

enum {
	c2s, s2c
};

enum {
	key_ok, key_wrong, key_notfound, key_file
};

/* misc.c */
void			error(char *, ...);
void			debug(int, char *, ...);
void			debugbig(int, mpint*, char);
void*			mmalloc(int);
void*			zalloc(int);
void			printbig(int, mpint*, int, char);
void			printpublickey(int, RSApub *);
void			printdecpublickey(int, RSApub *);
void			printsecretkey(int, RSApriv *);
int			readsecretkey(FILE *, RSApriv *);
int			readpublickey(FILE *, RSApub *);
void			comp_sess_id(mpint*, mpint*, uchar *, uchar *);
void			RSApad(uchar *, int, uchar *, int);
mpint*			RSAunpad(mpint*, int);
mpint*			RSAEncrypt(mpint*, RSApub*);
mpint*			RSADecrypt(mpint*, RSApriv*);
int			verify_key(char *, RSApub *, char *);
int			replace_key(char *, RSApub *, char *);
int			isatty(int);
void			getdomainname(char*, char*, int);
void			mptobuf(mpint*, uchar*, int);

/* packet.c */
void			mkcrctab(ulong);
Packet*			getpacket(int);
int			putpacket(Packet *, int);
void			preparekey(void);
void			mfree(Packet *);

ulong			getlong(Packet *);
RSApub*			getRSApublic(Packet *);
ulong			getlong(Packet *);
ushort			getshort(Packet *);
uchar			getbyte(Packet *);
void			getbytes(Packet *, uchar *, int);
mpint*			getBigInt(Packet *);
int			getstring(Packet *, char *, int);

void			putlong(Packet *, ulong);
void			putshort(Packet *, ushort);
void			putbyte(Packet *, uchar);
void			putbytes(Packet *, uchar *, int);
void			putBigInt(Packet *, mpint*);
void			putRSApublic(Packet *, RSApub *);
void			putstring(Packet *, char *, int);


/* messages.c */
void			get_ssh_smsg_public_key(void);
