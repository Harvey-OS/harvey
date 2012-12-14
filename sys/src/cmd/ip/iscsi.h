/*
 * iscsi - scsi over tcp/ip
 *
 * observations:
 * iscsi names may be 223 bytes long.
 * two namespaces, "iqn." and "eui.".
 * don't need FIM when using tcp.
 * rfc3720 §3.2.8.1 demonstrates that the authors don't understand (or
 * don't trust) tcp: data isn't quietly dropped.
 *
 * we're not going to implement the optional digests (crcs),
 * so those are omitted.
 */

enum {
	Bhdrsz	= 48,

	/* initiator opcodes (requests) */
	Iopnopout= 0,
	Iopcmd,
	Ioptask,
	Ioplogin,
	Ioptext,
	Iopdataout,		/* write */
	Ioplogout,
	Iopsnack= 0x10,

	/* target opcodes (responses) */
	Topnopin= 0x20,
	Topresp,
	Toptask,
	Toplogin,
	Toptext,
	Topdatain,		/* read */
	Toplogout,
	Topr2t	= 0x31,
	Topasync,
	Topreject= 0x3f,

	Immed	= 1<<6,
	Oprsrvd	= 1<<7,

	/* opspfc */
	Finalpkt= 1<<7,
};

typedef struct Datainpkt Datainpkt;
typedef struct Iscsiasynch Iscsiasynch;
typedef struct Iscsicmdreq Iscsicmdreq;
typedef struct Iscsicmdresp Iscsicmdresp;
typedef struct Iscsidatain Iscsidatain;
typedef struct Iscsidataout Iscsidataout;
typedef struct Iscsijustbhdr Iscsijustbhdr;
typedef struct Iscsiloginreq Iscsiloginreq;
typedef struct Iscsiloginresp Iscsiloginresp;
typedef struct Iscsilogoutreq Iscsilogoutreq;
typedef struct Iscsilogoutresp Iscsilogoutresp;
typedef struct Iscsinopreq Iscsinopreq;		/* nop out */
typedef struct Iscsinopresp Iscsinopresp;	/* nop in */
typedef struct Iscsiready Iscsiready;
typedef struct Iscsireject Iscsireject;
typedef struct Iscsisnackreq Iscsisnackreq;
typedef struct Iscsistate Iscsistate;
typedef struct Iscsitaskreq Iscsitaskreq;
typedef struct Iscsitaskresp Iscsitaskresp;
typedef struct Iscsitextreq Iscsitextreq;
typedef struct Iscsitextresp Iscsitextresp;
typedef struct Pkt Pkt;
typedef struct Pkts Pkts;
typedef struct Resppkt Resppkt;

/*
 * iscsi on-the-wire packet formats
 *
 * all integers are stored big-endian.
 * common pkt start is bhs, 48 bytes.
 */

/*
 * first 20 bytes are common *and* defined; another 28 follow but vary.
 */
#define Iscsibhdr \
	uchar	op;		\
	uchar	opspfc[3];	\
	uchar	totahslen;	\
	uchar	dseglen[3];	\
	uchar	lun[8];		\
	uchar	itt[4]		/* initiator task tag */

struct Iscsijustbhdr {
	Iscsibhdr;
};

struct Iscsiasynch {
	Iscsibhdr;

	uchar	_pad1[4];
	uchar	stsseq[4];
	uchar	expcmdseq[4];
	uchar	maxcmdseq[4];

	uchar	event;
	uchar	vcode;
	uchar	param1[2];
	uchar	param2[2];
	uchar	param3[2];
	uchar	_pad2[4];
	uchar	dseg[];		/* dseg[dseglen] */
};


struct Iscsiloginreq {
	Iscsibhdr;

	uchar	cid[2];
	uchar	_pad1[2];
	uchar	cmdseq[4];
	uchar	expstsseq[4];

	uchar	_pad2[16];
	uchar	dseg[];		/* dseg[dseglen] */
};

struct Iscsiloginresp {
	Iscsibhdr;

	uchar	_pad1[4];
	uchar	stsseq[4];
	uchar	expstsseq[4];
	uchar	maxstsseq[4];

	uchar	stsclass;
	uchar	stsdetail;
	uchar	_pad2[10];
	uchar	dseg[];		/* dseg[dseglen] */
};


struct Iscsilogoutreq {
	Iscsibhdr;

	uchar	cid[2];
	uchar	_pad1[2];
	uchar	cmdseq[4];
	uchar	expstsseq[4];
	uchar	_pad2[16];
};

struct Iscsilogoutresp {
	Iscsibhdr;

	uchar	_pad1[4];
	uchar	stsseq[4];
	uchar	expcmdseq[4];
	uchar	maxcmdseq[4];
	uchar	_pad2[4];
	uchar	waittime[2];
	uchar	retaintime[2];
	uchar	_pad3[4];
};


struct Iscsinopreq {		/* nop-out */
	Iscsibhdr;

	uchar	ttt[4];
	uchar	cmdseq[4];
	uchar	expcmdseq[4];
	uchar	_pad2[16];
	uchar	dseg[];		/* dseg[dseglen] */
};

struct Iscsinopresp {		/* nop-in */
	Iscsibhdr;

	uchar	ttt[4];
	uchar	stsseq[4];
	uchar	expcmdseq[4];
	uchar	maxcmdseq[4];
	uchar	_pad2[12];
	uchar	dseg[];		/* dseg[dseglen] */
};


struct Iscsiready {
	Iscsibhdr;

	uchar	ttt[4];
	uchar	stsseq[4];
	uchar	expcmdseq[4];
	uchar	r2tseq[4];
	uchar	bufoff[4];
	uchar	xferlen[4];
};

struct Iscsireject {
	Iscsibhdr;

	uchar	_pad2[4];
	uchar	stsseq[4];
	uchar	expcmdseq[4];
	uchar	maxcmdseq[4];
	uchar	r2tseq[4];
	uchar	_pad3[4+4];
	uchar	dseg[];		/* dseg[dseglen] */
};


struct Iscsicmdreq {
	Iscsibhdr;

	uchar	expxferlen[4];
	uchar	cmdseq[4];
	uchar	expstsseq[4];

	uchar	cdb[16];
	uchar	dseg[];		/* dseg[dseglen] */
};

struct Iscsicmdresp {
	Iscsibhdr;

	uchar	snacktag[4];
	uchar	stsseq[4];
	uchar	expcmdseq[4];
	uchar	maxcmdseq[4];
	uchar	expdataseq[4];
	uchar	readresid[4];
	uchar	resid[4];
	uchar	dseg[];		/* dseg[dseglen] */
};


struct Iscsidatain {
	Iscsibhdr;

	uchar	ttt[4];
	uchar	stsseq[4];
	uchar	expcmdseq[4];
	uchar	maxcmdseq[4];
	uchar	dataseq[4];
	uchar	buffoff[4];
	uchar	resid[4];
	uchar	dseg[];		/* dseg[dseglen] */
};

struct Iscsidataout {		/* a.k.a. data-out */
	Iscsibhdr;

	uchar	ttt[4];
	uchar	_pad1[4];
	uchar	expstsseq[4];
	uchar	_pad2[4];
	uchar	dataseq[4];
	uchar	buffoff[4];
	uchar	_pad3[4];
	uchar	dseg[];		/* dseg[dseglen] */
};


struct Iscsisnackreq {
	Iscsibhdr;

	uchar	ttt[4];
	uchar	_pad1[4];
	uchar	expstsseq[4];
	uchar	_pad2[8];
	uchar	begrun[4];
	uchar	runlen[4];
};


struct Iscsitaskreq {
	Iscsibhdr;

	uchar	rtt[4];
	uchar	cmdseq[4];
	uchar	expstsseq[4];
	uchar	refcmdseq[4];
	uchar	expdataseq[4];
	uchar	_pad1[8];
};

struct Iscsitaskresp {
	Iscsibhdr;

	uchar	_pad2[4];
	uchar	stsseq[4];
	uchar	expcmdseq[4];
	uchar	maxcmdseq[4];
	uchar	_pad4[12];
};


/* req & resp identical? */
struct Iscsitextreq {
	Iscsibhdr;

	uchar	ttt[4];
	uchar	cmdseq[4];
	uchar	expstsseq[4];
	uchar	_pad1[16];
	uchar	dseg[];		/* dseg[dseglen] */
};

struct Iscsitextresp {
	Iscsibhdr;

	uchar	ttt[4];
	uchar	cmdseq[4];
	uchar	expstsseq[4];
	uchar	_pad1[16];
	uchar	dseg[];		/* dseg[dseglen] */
};

/*
 * packet-specific data
 */
struct Pkt {
	union {
		Iscsijustbhdr *pkthdr;
		uchar	*pkt;
	};
	uint	len;
};
struct Resppkt {
	Iscsicmdresp *resppkt;
	uint	resplen;
};
struct Datainpkt {
	Iscsidatain *datapkt;
	uint	datalen;
};

/*
 * a request packet (pkt), its response packet (Resppkt),
 * and an optional data-in packet for cmd read operations.
 */
struct Pkts {
	int	op;
	int	immed;
	ulong	totahslen;
	ulong	dseglen;
	ulong	itt;

	Pkt;
	Datainpkt;
	Resppkt;
};

/*
 * iscsi state, maintained by both parties.
 * some are per-connection, some per-session.
 */
struct Iscsistate {
	ulong	cmdseq;		/* to be assigned to next command pkt */
	ulong	expcmdseq;	/* next cmd expected by target; previous done */
	ulong	maxcmdseq;  /* maxcmdseq-expcmdseq+1 is target's queue depth */

	ulong	stsseq;
	ulong	expstsseq;	/* all previous sts seen */

	ulong	dataseq;
	ulong	r2tseq;

	ulong	maxrcvseglen;
};
