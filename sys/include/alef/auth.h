#pragma src "/sys/src/alef/libauth"
#pragma lib "/$M/lib/alef/libauth.a"

enum
{
	DOMLEN=		48,		/* length of an authentication domain name */
	DESKEYLEN=	7,		/* length of a des key for encrypt/decrypt */
	CHALLEN=	8,		/* length of a challenge */
	NETCHLEN=	16,		/* max network challenge length	*/
	CONFIGLEN=	14,

	KEYDBLEN=	NAMELEN+DESKEYLEN+4+2
};

/* encryption numberings (anti-replay) */
enum
{
	AuthTreq=1,	/* ticket request */
	AuthChal=2,	/* challenge box request */
	AuthPass=3,	/* change password */

	AuthOK=4,	/* reply follows */
	AuthErr=5,	/* error follows */

	AuthTs=64,	/* ticket encrypted with server's key */
	AuthTc,		/* ticket encrypted with client's key */
	AuthAs,		/* server generated authenticator */
	AuthAc,		/* client generated authenticator */
};

aggr Ticketreq
{
	byte	type;
	byte	authid[NAMELEN];	/* server's encryption id */
	byte	authdom[DOMLEN];	/* server's authentication domain */
	byte	chal[CHALLEN];		/* challenge from server */
	byte	hostid[NAMELEN];	/* host's encryption id */
	byte	uid[NAMELEN];		/* uid of requesting user on host */
};
#define	TICKREQLEN	(3*NAMELEN+CHALLEN+DOMLEN+1)

aggr Ticket
{
	byte	num;			/* replay protection */
	byte	chal[CHALLEN];		/* server challenge */
	byte	cuid[NAMELEN];		/* uid on client */
	byte	suid[NAMELEN];		/* uid on server */
	byte	key[DESKEYLEN];		/* nonce DES key */
};
#define	TICKETLEN	(CHALLEN+2*NAMELEN+DESKEYLEN+1)

aggr Authenticator
{
	byte	num;			/* replay protection */
	byte	chal[CHALLEN];
	uint	id;			/* authenticator id, ++'d with each auth */
};
#define	AUTHENTLEN	(CHALLEN+4+1)

aggr Passwordreq
{
	byte	num;
	byte	old[NAMELEN];
	byte	new[NAMELEN];
};
#define	PASSREQLEN	(2*NAMELEN+1)

aggr Nvrsafe
{
	byte	machkey[DESKEYLEN];
	byte	machsum;
	byte	authkey[DESKEYLEN];
	byte	authsum;
	byte	config[CONFIGLEN];
	byte	configsum;
	byte	authid[NAMELEN];
	byte	authidsum;
	byte	authdom[DOMLEN];
	byte	authdomsum;
};

aggr Chalstate
{
	int	afd;			/* /dev/authenticate */
	int	asfd;			/* authdial() */
	byte	chal[NETCHLEN];		/* challenge/response */
};

extern	int	convT2M(Ticket*, byte*, byte*);
extern	void	convM2T(byte*, Ticket*, byte*);
extern	int	convA2M(Authenticator*, byte*, byte*);
extern	void	convM2A(byte*, Authenticator*, byte*);
extern	int	convTR2M(Ticketreq*, byte*);
extern	void	convM2TR(byte*, Ticketreq*);
extern	int	convPR2M(Passwordreq*, byte*, byte*);
extern	void	convM2PR(byte*, Passwordreq*, byte*);
extern	byte	nvcsum(void*, int);
extern	int	opasstokey(void*, byte*);
extern	int	passtokey(void*, byte*);
extern	int	authenticate(int, int);
extern	int	newns(byte*, byte*);
extern	int	authdial(void);
extern	int	auth(int);
extern	int	srvauth(int, byte*);
extern	int	getchal(Chalstate*, byte*);
extern	int	chalreply(Chalstate*, byte*);
extern	int	amount(int, byte*, int, byte*);
