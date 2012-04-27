/*
 * The protocol packs requests within transport channels.
 *
 * The format in the wire is len[2] tag[2] msg[]
 * where len is the number of bytes of the message and
 * tag is a channel number.
 *
 * there two special bits in tags: Tfirst, and Tlast.
 * The first message in a channel must set Tfirst
 * the last one must set Tlast.
 * Tfirst = 0x8000, Tlast = 0x4000
 *
 * A channel is duplex and is closed when both directions have
 * exchanged messages with Tlast set.
 * An error reply always has Tlast set.
 *
 * Authentication is fully left out of the protocol.
 * The underlying transport must be secured, in a way that
 * provides trust between both parties.
 *
 * This is the set of individual requests or messages:
 * Here, data[] means what's left of the message (i.e., count is implied).
 *
 *      IXTversion msize[4] version[s]
 *      IXRversion msize[4] version[s]
 *      IXTattach uname[s] aname[s]
 *      IXRattach fid[4]				// fid becomes the current fid
 *	IXTfid fid[4]
 *	IXRfid
 *      IXRerror ename[s]
 *      IXTclone cflags[1]
 *      IXRclone fid[4]				// fid becomes the current fid
 *      IXTwalk wname[s]
 *      IXRwalk
 *      IXTopen mode[1]				// mode includes cflags as well
 *      IXRopen
 *      IXTcreate name[s] perm[4] mode[1]	// mode includes cflags as well
 *      IXRcreate
 *      IXTread nmsg[2] offset[8] count[4]
 *      IXRread data[]
 *      IXTwrite offset[8] endoffset[8] data[]
 *      IXRwrite offset[8] count[4]
 *      IXTclunk
 *      IXRclunk
 *      IXTremove
 *      IXRremove
 *      IXTattr attr[s]			//"*" means: return attr name list
 *      IXRattr value[]
 *      IXTwattr attr[s] value[]	// perhaps: cacheable "y"
 *      IXRwattr
 *      IXTcond op[1] attr[s] value[]
 *      IXRcond
 *      IXTmove dirfid[4] newname[s]
 *      IXRmove
 *
 * There is no flush. Flushing is done by flushing the channel.
 */

enum
{
	IXTversion = 50,
	IXRversion,
	IXTattach,
	IXRattach,
	IXTfid,
	IXRfid,
	__IXunused__,
	IXRerror,
	IXTclone,
	IXRclone,
	IXTwalk,
	IXRwalk,
	IXTopen,
	IXRopen,
	IXTcreate,
	IXRcreate,
	IXTread,
	IXRread,
	IXTwrite,
	IXRwrite,
	IXTclunk,
	IXRclunk,
	IXTremove,
	IXRremove,
	IXTattr,
	IXRattr,
	IXTwattr,
	IXRwattr,
	IXTcond,
	IXRcond,
	IXTmove,
	IXRmove,
	IXTmax,

	/*
	 * flags used in Tclone, Topen, Tcreate
	 */
	OCEND = 0x4,		/* clunk on end of rpc */
	OCERR = 0x8,		/* clunk on error */

	CEQ = 0,		/* Tcond.op */
	CGE,
	CGT,
	CLE,
	CLT,
	CNE,
	CMAX,
};


typedef struct IXcall IXcall;

/*
 * len[2] tag[2] prepended by the transport.
 * This is an individual call request.T
 * Fids are selected by the server and handed to the client.
 */
struct IXcall
{
	uchar	type;
	union{
		struct{				/* Tversion, Rversion */
			u32int	msize;
			char	*version;
		};
		struct{
			u32int	fid;		/* Tfid, Rattach, Rclone */
		};
		struct{
			char	*uname;		/* Tattach */
			char	*aname;		/* Tattach */
		};
		struct{
			char	*ename;		/* Rerror */
		};
		struct{
			uchar	cflags;		/* Tclone (OCEND|OCERR) */
		};
		struct{
			uchar	mode;		/* Topen, Tcreate */
			u32int	perm;		/* Tcreate */
			char	*name;		/* Tcreate */
		};
		struct{				/* Twalk */
			char	*wname;
		};
		struct{
			u16int	nmsg;		/* Tread */
			uvlong	offset;		/* Tread, Twrite, Rwrite */
			uvlong	endoffset;	/* Twrite */
			u32int	count;		/* Tread, Rread, Twrite, Rwrite */
			uchar	*data;		/* Twrite, Rread */
		};
		struct{
			uchar	op;		/* Tcond */
			char	*attr;		/* Tattr, Twattr, Tcond */
			uchar	*value;		/* Rattr, Twattr, Tcond */
			u32int	nvalue;		/* Twattr, Rattr*/
		};
		struct{				/* Tmove */
			u32int	dirfid;
			char	*newname;
		};
		/* With struct{}:
		 * Rfid,
		 * Rwalk, Ropen, Rcreate, Tclunk, Rclunk, Tclose, Rclose,
		 * Tremove, Rremove, Rwattr, Rcond, Rmove
		 */
	};
};

/* 8c bug: varargck does not like IX */
typedef IXcall COMPILERBUGpcall;
#pragma	varargck	type	"G"	COMPILERBUGpcall*

