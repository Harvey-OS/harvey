#include <bio.h>
#include "ssh2.h"		/* ugh */

#define MYID "SSH-2.0-Plan9"

#pragma varargck type "M" mpint*

enum {
	Server =	0,
	Client,

	Maxpktpay =	35000,

	/* qid.path components: level (2), type (4), conn (7), chan (7) */
	Connshift =	7,
	MAXCONN =	1 << Connshift,		/* also Maxchan */
	Chanmask =	MAXCONN - 1,
	Connmask =	Chanmask,

	Qtypeshift =	2 * Connshift,		/* conn + chan */

	Qroot =		0,
	Qclone =	1 << Qtypeshift,
	Qctl =		2 << Qtypeshift,
	Qdata =		3 << Qtypeshift,
	Qlisten =	4 << Qtypeshift,
	Qlocal =	5 << Qtypeshift,
	Qreqrem =	6 << Qtypeshift,	/* request or remote */
	Qstatus =	7 << Qtypeshift,
	Qtcp =		8 << Qtypeshift,
	Qtypemask =	017 << Qtypeshift,

	Levshift =	Qtypeshift + 4,

	/* levels of /net/ssh hierarchy */
	Top =		0,
	Connection,
	Subchannel,
};

/*
 * The stylistic anomaly with these names of unbounded length
 * is a result of following the RFCs in using the same names for
 * these constants.  I did that to make it easier to search and
 * cross-reference between the code and the RFCs.
 */
enum {					/* SSH2 Protocol Packet Types */
	SSH_MSG_DISCONNECT =		1,
	SSH_MSG_IGNORE =		2,
	SSH_MSG_UNIMPLEMENTED,
	SSH_MSG_DEBUG,
	SSH_MSG_SERVICE_REQUEST,
	SSH_MSG_SERVICE_ACCEPT,

	SSH_MSG_KEXINIT =		20,
	SSH_MSG_NEWKEYS,

	SSH_MSG_KEXDH_INIT =		30,
	SSH_MSG_KEXDH_REPLY,

	SSH_MSG_USERAUTH_REQUEST =	50,
	SSH_MSG_USERAUTH_FAILURE,
	SSH_MSG_USERAUTH_SUCCESS,
	SSH_MSG_USERAUTH_BANNER,

	SSH_MSG_USERAUTH_PK_OK =	60,
	SSH_MSG_USERAUTH_PASSWD_CHANGEREQ = 60,

	SSH_MSG_GLOBAL_REQUEST =	80,
	SSH_MSG_REQUEST_SUCCESS,
	SSH_MSG_REQUEST_FAILURE,

	SSH_MSG_CHANNEL_OPEN =		90,
	SSH_MSG_CHANNEL_OPEN_CONFIRMATION,
	SSH_MSG_CHANNEL_OPEN_FAILURE,
	SSH_MSG_CHANNEL_WINDOW_ADJUST,
	SSH_MSG_CHANNEL_DATA,
	SSH_MSG_CHANNEL_EXTENDED_DATA,
	SSH_MSG_CHANNEL_EOF,
	SSH_MSG_CHANNEL_CLOSE,
	SSH_MSG_CHANNEL_REQUEST,
	SSH_MSG_CHANNEL_SUCCESS,
	SSH_MSG_CHANNEL_FAILURE,
};

enum {				/* SSH2 reason codes */
	SSH_DISCONNECT_HOST_NOT_ALLOWED_TO_CONNECT = 1,
	SSH_DISCONNECT_PROTOCOL_ERROR,
	SSH_DISCONNECT_KEY_EXCHANGE_FAILED,
	SSH_DISCONNECT_RESERVED,
	SSH_DISCONNECT_MAC_ERROR,
	SSH_DISCONNECT_COMPRESSION_ERROR,
	SSH_DISCONNECT_SERVICE_NOT_AVAILABLE,
	SSH_DISCONNECT_PROTOCOL_VERSION_NOT_SUPPORTED,
	SSH_DISCONNECT_HOST_KEY_NOT_VERIFIABLE,
	SSH_DISCONNECT_CONNECTION_LOST,
	SSH_DISCONNECT_BY_APPLICATION,
	SSH_DISCONNECT_TOO_MANY_CONNECTIONS,
	SSH_DISCONNECT_AUTH_CANCELLED_BY_USER,
	SSH_DISCONNECT_NO_MORE_AUTH_METHODS_AVAILABLE,
	SSH_DISCONNECT_ILLEGAL_USR_NAME,

	SSH_OPEN_ADMINISTRATIVELY_PROHIBITED = 1,
	SSH_OPEN_CONNECT_FAILED,
	SSH_OPEN_UNKNOWN_CHANNEL_TYPE,
	SSH_OPEN_RESOURCE_SHORTAGE,
};

enum {				/* SSH2 type code */
	SSH_EXTENDED_DATA_STDERR = 1,
};

enum {				/* connection and channel states */
	Empty = 0,
	Allocated,
	Initting,
	Listening,
	Opening,
	Negotiating,
	Authing,
	Established,
	Eof,
	Closing,
	Closed,
};

enum {
	NoKeyFile,
	NoKey,
	KeyWrong,
	KeyOk,
};

typedef struct Cipher Cipher;
typedef struct CipherState CipherState;
typedef struct Conn Conn;
typedef struct Kex Kex;
typedef struct MBox MBox;
typedef struct PKA PKA;
typedef struct Packet Packet;
typedef struct Plist Plist;
typedef struct SSHChan SSHChan;

#pragma incomplete CipherState

struct Plist {
	Packet	*pack;
	uchar	*st;
	int	rem;
	Plist	*next;
};

struct SSHChan {
	Rendez	r;			/* awaiting input? */
	int	id;
	int	otherid;
	int	state;
	int	waker;
	int	conn;
	ulong	rwindow;
	ulong	twindow;
	ulong	sent;
	ulong	inrqueue;
	char	*ann;
	Req	*lreq;

	/* File* for each Qid type */
	File	*dir;
	File	*ctl;
	File	*data;
	File	*listen;
	File	*request;
	File	*status;
	File	*tcp;

	Plist	*dataq;
	Plist	*datatl;
	Plist	*reqq;
	Plist	*reqtl;

	Channel	*inchan;
	Channel	*reqchan;
	QLock	xmtlock;
	Rendez	xmtrendez;
};

struct Conn {
	QLock	l;
	Rendez	r;			/* awaiting input? */

	Ioproc	*dio;
	Ioproc	*cio;
	Ioproc	*rio;

	int	state;
	int	role;
	int	id;

	char	*remote;
	char	*user;
	char	*password;

	char	*service;
	char	*cap;
	char	*authkey;
	int	nchan;

	/* underlying tcp connection */
	int	datafd;
	int	ctlfd;
	int	stifle;		/* flag: no i/o between listen and sshsession */
	int	poisoned;
	int	tcpconn;

	int	rpid;

	int	inseq;
	int	outseq;
	int	kexalg;
	int	pkalg;

	int	cscrypt;
	int	ncscrypt;
	int	sccrypt;
	int	nsccrypt;

	int	csmac;
	int	ncsmac;
	int	scmac;
	int	nscmac;

	int	encrypt;
	int	decrypt;

	int	outmac;
	int	inmac;

	/* File* for each Qid type */
	File	*dir;
	File	*clonefile;
	File	*ctlfile;
	File	*datafile;
	File	*listenfile;
	File	*localfile;
	File	*remotefile;
	File	*statusfile;
	File	*tcpfile;

	Packet	*skexinit;
	Packet	*rkexinit;
	mpint	*x;
	mpint	*e;
	int	got_sessid;
	uchar	sessid[SHA1dlen];

	uchar	c2siv[SHA1dlen*2];
	uchar	nc2siv[SHA1dlen*2];
	uchar	s2civ[SHA1dlen*2];
	uchar	ns2civ[SHA1dlen*2];

	uchar	c2sek[SHA1dlen*2];
	uchar	nc2sek[SHA1dlen*2];
	uchar	s2cek[SHA1dlen*2];
	uchar	ns2cek[SHA1dlen*2];

	uchar	c2sik[SHA1dlen*2];
	uchar	nc2sik[SHA1dlen*2];
	uchar	s2cik[SHA1dlen*2];
	uchar	ns2cik[SHA1dlen*2];

	char	*otherid;
	uchar	*inik;
	uchar	*outik;
	CipherState *s2ccs;
	CipherState *c2scs;
	CipherState *enccs;
	CipherState *deccs;
	SSHChan	*chans[MAXCONN];

	char	idstring[256];		/* max allowed by SSH spec */
};

struct Packet {
	Conn	*c;
	ulong	rlength;
	ulong	tlength;
	uchar	nlength[4];
	uchar	pad_len;
	uchar	payload[Maxpktpay];
};

struct Cipher {
	char	*name;
	int	blklen;
	CipherState *(*init)(Conn*, int);
	void	(*encrypt)(CipherState*, uchar*, int);
	void	(*decrypt)(CipherState*, uchar*, int);
};

struct Kex {
	char	*name;
	int	(*serverkex)(Conn *, Packet *);
	int	(*clientkex1)(Conn *, Packet *);
	int	(*clientkex2)(Conn *, Packet *);
};

struct PKA {
	char	*name;
	Packet	*(*ks)(Conn *);
	Packet	*(*sign)(Conn *, uchar *, int);
	int	(*verify)(Conn *, uchar *, int, char *, char *, int);
};

struct MBox {
	Channel	*mchan;
	char	*msg;
	int	state;
};

extern Cipher cipheraes128, cipheraes192, cipheraes256;
extern Cipher cipherblowfish, cipher3des, cipherrc4;
extern int debug;
extern int sshkeychan[];
extern Kex dh1sha1, dh14sha1;
extern MBox keymbox;
extern PKA rsa_pka, dss_pka, *pkas[];

/* pubkey.c */
int appendkey(char *, char *, RSApub *);
int findkey(char *, char *, RSApub *);
RSApub *readpublickey(Biobuf *, char **);
int replacekey(char *, char *, RSApub *);

/* dh.c */
void dh_init(PKA *[]);

/* transport.c */
void	add_block(Packet *, void *, int);
void	add_byte(Packet *, char);
void	add_mp(Packet *, mpint *);
int	add_packet(Packet *, void *, int);
void	add_string(Packet *, char *);
void	add_uint32(Packet *, ulong);
void	dump_packet(Packet *);
int	finish_packet(Packet *);
mpint	*get_mp(uchar *q);
uchar	*get_string(Packet *, uchar *, char *, int, int *);
ulong	get_uint32(Packet *, uchar **);
void	init_packet(Packet *);
Packet	*new_packet(Conn *);
int	undo_packet(Packet *);
