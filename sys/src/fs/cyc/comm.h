#define	COMM	((Comm*)0x10000)

enum
{
	NRQ	= 200,				/* host->hotrod request q */
	PRIME	= 1,

	Ureset	= 0,				/* commands */
	Ureboot,
	Uread,
	Uwrite,
	Urpc,
	Ubuf,
	Ureply,
	Utest,
	Uping,
	Upingreply,
};

typedef	struct	Comm	Comm;

/*
 * communication region
 * top of ram. stack just below.
 */
struct	Comm
{
	/*
	 * host->cyclone request q.
	 * circular.
	 */
	ulong	reqstp;
	ulong	reqstq[NRQ];

	/*
	 * cyclone->host reply q.
	 * circular.
	 */
	ulong	replyp;
	ulong	replyq[NRQ];

	/*
	 * boot rom jump pc
	 */
	ulong	entry;
	uchar	vmevec;
};
