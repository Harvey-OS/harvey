/*
 *	Cf. /lib/rfc/rfc1014, /lib/rfc/rfc1050
 */

enum Bool
{
	FALSE	= 0,
	TRUE	= 1
};

enum Auth_flavor
{
	AUTH_NULL	= 0,
	AUTH_UNIX	= 1,
	AUTH_SHORT	= 2,
	AUTH_DES	= 3
};

enum Msg_type
{
	CALL	= 0,
	REPLY	= 1
};

/*
 * A reply to a call message can take on two forms:
 * The message was either accepted or rejected.
 */

enum Reply_stat
{
	MSG_ACCEPTED	= 0,
	MSG_DENIED	= 1
};

/*
 * Given that a call message was accepted, the following is the
 * status of an attempt to call a remote procedure.
 */
enum Accept_stat
{
	SUCCESS		= 0,	/* RPC executed successfully       */
	PROG_UNAVAIL	= 1,	/* remote hasn't exported program  */
	PROG_MISMATCH	= 2,	/* remote can't support version #  */
	PROC_UNAVAIL	= 3,	/* program can't support procedure */
	GARBAGE_ARGS	= 4	/* procedure can't decode params   */
};

/*
 * Reasons why a call message was rejected:
 */
enum Reject_stat
{
	RPC_MISMATCH	= 0,	/* RPC version number != 2          */
	AUTH_ERROR	= 1	/* remote can't authenticate caller */
};

/*
 * Why authentication failed:
 */
enum Auth_stat
{
	AUTH_BADCRED		= 1,	/* bad credentials (seal broken) */
	AUTH_REJECTEDCRED	= 2,	/* client must begin new session */
	AUTH_BADVERF		= 3,	/* bad verifier (seal broken)    */
	AUTH_REJECTEDVERF	= 4,	/* verifier expired or replayed  */
	AUTH_TOOWEAK		= 5	/* rejected for security reasons */
};

enum
{
	IPPROTO_TCP	= 6,	/* protocol number for TCP/IP */
	IPPROTO_UDP	= 17	/* protocol number for UDP/IP */
};

#define	ROUNDUP(n)	((n) + ((-(n))&3))	

#define	PLONG(x)	(dataptr[3] = ((ulong)(x)), dataptr[2] = ((ulong)(x))>>8, dataptr[1] = ((ulong)(x))>>16, dataptr[0] = ((ulong)(x))>>24, dataptr += 4)
#define	PPTR(x, n)	(memmove(dataptr, (x), n), dataptr += ROUNDUP(n))
#define	PBYTE(x)	(*dataptr++ = (x))

#define	GLONG()		(argptr += 4, (((uchar*)argptr)[-1] | (((uchar*)argptr)[-2]<<8) | (((uchar*)argptr)[-3]<<16) | (((uchar*)argptr)[-4]<<24)))
#define	GPTR(n)		(void *)(argptr); argptr += ROUNDUP(n)
#define	GBYTE()	(argptr++, ((uchar*)argptr)[-1])
