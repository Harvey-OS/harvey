/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

/*
 * mailbox and message representations
 *
 * these structures are allocated with emalloc and must be explicitly freed
 */
typedef struct Box	Box;
typedef struct Header	Header;
typedef struct MAddr	MAddr;
typedef struct MbLock	MbLock;
typedef struct MimeHdr	MimeHdr;
typedef struct Msg	Msg;
typedef struct NamedInt	NamedInt;
typedef struct Pair	Pair;

enum
{
	StrAlloc	= 32,		/* characters allocated at a time */
	BufSize		= 8*1024,	/* size of transfer block */
	NDigest		= 40,		/* length of digest string */
	NUid		= 10,		/* length of .imp uid string */
	NFlags		= 8,		/* length of .imp flag string */
	LockSecs	= 5 * 60,	/* seconds to wait for acquiring a locked file */
	MboxNameLen	= 256,		/* max. length of upas/fs mbox name */
	MsgNameLen	= 32,		/* max. length of a file in a upas/fs mbox */
	UserNameLen	= 64,		/* max. length of user's name */

	MUtf7Max	= 6,		/* max length for a modified utf7 character: &bbbb- */

	/*
	 * message flags
	 */
	MSeen		= 1 << 0,
	MAnswered	= 1 << 1,
	MFlagged	= 1 << 2,
	MDeleted	= 1 << 3,
	MDraft		= 1 << 4,
	MRecent		= 1 << 5,

	/*
	 * message bogus flags
	 */
	NotBogus	= 0,	/* the message is displayable */
	BogusHeader	= 1,	/* the header had bad characters */
	BogusBody	= 2,	/* the body had bad characters */
	BogusTried	= 4,	/* attempted to open the fake message */
};

struct Box
{
	char	*name;		/* path name of mailbox */
	char	*fs;		/* fs name of mailbox */
	char	*fsDir;		/* /mail/fs/box->fs */
	char	*imp;		/* path name of .imp file */
	uint8_t	writable;	/* can write back messages? */
	uint8_t	dirtyImp;	/* .imp file needs to be written? */
	uint8_t	sendFlags;	/* need flags update */
	Qid	qid;		/* qid of fs mailbox */
	Qid	impQid;		/* qid of .imp when last synched */
	long	mtime;		/* file mtime when last read */
	uint32_t	max;		/* maximum msgs->seq, same as number of messages */
	uint32_t	toldMax;	/* last value sent to client */
	uint32_t	recent;		/* number of recently received messaged */
	uint32_t	toldRecent;	/* last value sent to client */
	uint32_t	uidnext;	/* next uid value assigned to a message */
	uint32_t	uidvalidity;	/* uid of mailbox */
	Msg	*msgs;
};

/*
 * fields of Msg->info
 */
enum
{
	/*
	 * read from upasfs
	 */
	IFrom,
	ITo,
	ICc,
	IReplyTo,
	IUnixDate,
	ISubject,
	IType,
	IDisposition,
	IFilename,
	IDigest,
	IBcc,
	IInReplyTo,	/* aka internal date */
	IDate,
	ISender,
	IMessageId,
	ILines,		/* number of lines of raw body */

	IMax
};

struct Header
{
	char	*buf;		/* header, including terminating \r\n */
	uint32_t	size;		/* strlen(buf) */
	uint32_t	lines;		/* number of \n characters in buf */

	/*
	 * pre-parsed mime headers
	 */
	MimeHdr	*type;		/* content-type */
	MimeHdr	*id;		/* content-id */
	MimeHdr	*description;	/* content-description */
	MimeHdr	*encoding;	/* content-transfer-encoding */
	MimeHdr	*md5;		/* content-md5 */
	MimeHdr	*disposition;	/* content-disposition */
	MimeHdr	*language;	/* content-language */
};

struct Msg
{
	Msg	*next;
	Msg	*prev;
	Msg	*kids;
	Msg	*parent;
	char	*fsDir;		/* box->fsDir of enclosing message */
	Header	head;		/* message header */
	Header	mime;		/* mime header from enclosing multipart spec */
	int	flags;
	uint8_t	sendFlags;	/* flags value needs to be sent to client */
	uint8_t	expunged;	/* message actually expunged, but not yet reported to client */
	uint8_t	matched;	/* search succeeded? */
	uint8_t	bogus;		/* implies the message is invalid, ie contains nulls; see flags above */
	uint32_t	uid;		/* imap unique identifier */
	uint32_t	seq;		/* position in box; 1 is oldest */
	uint32_t	id;		/* number of message directory in upas/fs */
	char	*fs;		/* name of message directory */
	char	*efs;		/* pointer after / in fs; enough space for file name */

	uint32_t	size;		/* size of fs/rawbody, in bytes, with \r added before \n */
	uint32_t	lines;		/* number of lines in rawbody */

	char	*iBuf;
	char	*info[IMax];	/* all info about message */

	char	*unixDate;
	MAddr	*unixFrom;

	MAddr	*to;		/* parsed out address lines */
	MAddr	*from;
	MAddr	*replyTo;
	MAddr	*sender;
	MAddr	*cc;
	MAddr	*bcc;
};

/*
 * pre-parsed header lines
 */
struct MAddr
{
	char	*personal;
	char	*box;
	char	*host;
	MAddr	*next;
};

struct MimeHdr
{
	char	*s;
	char	*t;
	MimeHdr	*next;
};

/*
 * mapping of integer & names
 */
struct NamedInt
{
	char	*name;
	int	v;
};

/*
 * lock for all mail file operations
 */
struct MbLock
{
	int	fd;
};

/*
 * parse nodes for imap4rev1 protocol
 *
 * important: all of these items are allocated
 * in one can, so they can be tossed out at the same time.
 * this allows leakless parse error recovery by simply tossing the can away.
 * however, it means these structures cannot be mixed with the mailbox structures
 */

typedef struct Fetch	Fetch;
typedef struct NList	NList;
typedef struct SList	SList;
typedef struct MsgSet	MsgSet;
typedef struct Store	Store;
typedef struct Search	Search;

/*
 * parse tree for fetch command
 */
enum
{
	FEnvelope,
	FFlags,
	FInternalDate,
	FRfc822,
	FRfc822Head,
	FRfc822Size,
	FRfc822Text,
	FBodyStruct,
	FUid,
	FBody,			/* BODY */
	FBodySect,		/* BODY [...] */
	FBodyPeek,

	FMax
};

enum
{
	FPAll,
	FPHead,
	FPHeadFields,
	FPHeadFieldsNot,
	FPMime,
	FPText,

	FPMax
};

struct Fetch
{
	uint8_t	op;		/* F.* operator */
	uint8_t	part;		/* FP.* subpart for body[] & body.peek[]*/
	uint8_t	partial;	/* partial fetch? */
	long	start;		/* partial fetch amounts */
	long	size;
	NList	*sect;
	SList	*hdrs;
	Fetch	*next;
};

/*
 * status items
 */
enum{
	SMessages	= 1 << 0,
	SRecent		= 1 << 1,
	SUidNext	= 1 << 2,
	SUidValidity	= 1 << 3,
	SUnseen		= 1 << 4,
};

/*
 * parse tree for store command
 */
enum
{
	STFlags,
	STFlagsSilent,

	STMax
};

struct Store
{
	uint8_t	sign;
	uint8_t	op;
	int	flags;
};

/*
 * parse tree for search command
 */
enum
{
	SKNone,

	SKCharset,

	SKAll,
	SKAnswered,
	SKBcc,
	SKBefore,
	SKBody,
	SKCc,
	SKDeleted,
	SKDraft,
	SKFlagged,
	SKFrom,
	SKHeader,
	SKKeyword,
	SKLarger,
	SKNew,
	SKNot,
	SKOld,
	SKOn,
	SKOr,
	SKRecent,
	SKSeen,
	SKSentBefore,
	SKSentOn,
	SKSentSince,
	SKSet,
	SKSince,
	SKSmaller,
	SKSubject,
	SKText,
	SKTo,
	SKUid,
	SKUnanswered,
	SKUndeleted,
	SKUndraft,
	SKUnflagged,
	SKUnkeyword,
	SKUnseen,

	SKMax
};

struct Search
{
	int	key;
	char	*s;
	char	*hdr;
	uint32_t	num;
	int	year;
	int	mon;
	int	mday;
	MsgSet	*set;
	Search	*left;
	Search	*right;
	Search	*next;
};

struct NList
{
	uint32_t	n;
	NList	*next;
};

struct SList
{
	char	*s;
	SList	*next;
};

struct MsgSet
{
	uint32_t	from;
	uint32_t	to;
	MsgSet	*next;
};

struct Pair
{
	uint32_t	start;
	uint32_t	stop;
};

#include "bin.h"

extern	Bin	*parseBin;
extern	Biobuf	bout;
extern	Biobuf	bin;
extern	char	username[UserNameLen];
extern	char	mboxDir[MboxNameLen];
extern	char	*fetchPartNames[FPMax];
extern	char	*site;
extern	char	*remote;
extern	int	debug;

#include "fns.h"
