#pragma	lib	"libauth.a"

/*
 * authentication
 */

enum{
	/*
	 * fsauth message types
	 */
	FScchal	= 1,
	FSschal,
	FSok,
	FSctick,
	FSstick,
	FSerr,

	/*
	 * rexauth/chal message types
	 */
	RXcchal	= 0,
	RXschal	= 0,
	RXctick	= 1,
	RXstick	= 1,

	/*
	 * changekey message types
	 */
	CKcchal	= 1,

	/*
	 * length of a challenege & message type
	 */
	AUTHLEN	= 8,
	CONFIGLEN = 14,
	NETCHLEN = 16,

	KEYDBLEN	= NAMELEN+DESKEYLEN+4+2,
};

typedef struct	Nvrsafe	Nvrsafe;
struct Nvrsafe
{
	uchar	machkey[DESKEYLEN];
	uchar	machsum;
	uchar	authkey[DESKEYLEN];
	uchar	authsum;
	char	config[CONFIGLEN];
	uchar	configsum;
};

extern	char	*auth(int, char*);
extern	char	*srvauth(char*);
extern	int	getchall(char*, char[NETCHLEN]);
extern	int	challreply(int, char*, char*);
extern	char	*newns(char*, char*);
extern	int	authdial(char*);
extern	int	passtokey(void*, char*);
extern	uchar	nvcsum(void*, int);
