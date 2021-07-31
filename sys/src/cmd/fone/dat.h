typedef struct Call	Call;
typedef struct Cmd	Cmd;
typedef struct Mname	Mname;

#define	DISPSIZE	32

enum{
	Altered = 1,
	Xfrdial = 2,
	Confdial = 4,
	Spkrdial = 8
};

struct Call{
	int	state;
	int	assoc;
	ulong	flags;
	int	id;
	char *	dialp;
	char	num[DISPSIZE];
	char	who[DISPSIZE];
	char	tty[DISPSIZE];
	long	intime;
	long	smail;
	Call *	other;
	int	confcount;
};

#define	CMDSIZE	32

struct Cmd{
	Cmd *	next;
	char *	wptr;
	char	buf[CMDSIZE];
};

struct Mname{
	int	msg;
	char *	name;
};

enum _Messages {
	OK		= 0x0,
	Connected	= 0x01,
	Ring		= 0x02,
	Cleared		= 0x03,
	Error		= 0x4,
	Delivered	= 0x05,
	Display		= 0x06,
	Busy		= 0x07,
	Feature		= 0x08,
	Proceeding	= 0x0a,
	Progress	= 0x0b,
	Prompt		= 0x0c,
	Signal		= 0x0d,
	Outcall		= 0x0e,
	Completed	= 0x0f,
	Rejected	= 0x10,
	ATT_lamp	= 0x78,
	ATT_tones	= 0x79,
	Switchhook	= 0x7c,
	Disconnect	= 0x7d,
	L3_error	= 0x7e,
	ATT_display	= 0x7f,
	Associated	= 0x80,
	ATT_cause	= 0x81,
	ATT_HI		= 0x83,
	Pro_phone	= 0x100,
	API_phone	= 0x101,
	Red_lamp	= 0x102,
	Green_lamp	= 0x103
};

	/* Call states (table 4-7/Q.931, mostly) */

enum _Callstates {
	Null		= 0,
	Call_initiated	= 1,
	Overlap_send	= 2,
	Out_proceeding	= 3,
	Call_delivered	= 4,
	Call_present	= 6,
	Call_received	= 7,
	Connect_req	= 8,
	In_proceeding	= 9,
	Active		= 10,
	Disconnect_req	= 11,
	Disconnect_ind	= 12,
	Suspend_req	= 15,
	Suspended 	= ((16<<8)|Active),
	Resume_req	= 17,
	Release_req	= 19,
	Call_abort	= 22,
	Overlap_recv	= 25,
	Transfer_req	= 30,
	Query_req	= 35,
};

	/* Associated types (AT&T TR 801-802-100, sec. 8.2.3) */

#define	Asc_setup	0x05
#define	Asc_connect	0x07
#define	Asc_hold	0x08
#define	Asc_reconn	0x0c
#define	Asc_excl	0x0d
#define	Asc_noconn	0x0e
#define	Asc_noclear	0x0f

	/* switchhook flags */

#define	Off		0x01

#define	Sw_Handset	0x00
#define	Sw_Adjunct	0x02
#define	Sw_Speaker	0x04
#define	Sw_Spokesman	0x06
#define	Sw_Mute		0x08
#define	Sw_Remote	0x0a

#define	Handset		0x01
#define	Adjunct		0x02
#define	Speaker		0x04
#define	Spokesman	0x08
#define	Mute		0x10
#define	Remote		0x20

	/* Display fields */

#define	Call_appear_id	0x01
#define	Called_id	0x02
#define	Calling_id	0x03
#define	Calling_name	0x05
#define	Isdn_call_id	0x07
#define Dir_info	0x09
#define	Date_Time	0x0a

	/* Buttons (API only) */

#define	B_Shifted	0x8000
#define	B_Group 	0x7f00
#define	B_Number 	0x00ff
#define	B_Keypad	0x0200
#define	B_Fixed		0x0300
#define	B_Local		0x0400
#define	B_Clock		(B_Shifted|B_Fixed|3)
#define	B_Normal	(B_Local|0)
#define	B_Mute		(B_Local|1)
#define	B_Select	(B_Local|2)

extern Mname	M_Messages[];
extern Mname	M_Callstates[];

extern char	helptext[];

extern Call	calls[];
extern Call *	callsN;

extern char *	fname;
extern char *	dsrvname;
extern char *	srvname;
extern int	pipefd[];
extern char *	shname;
extern char *	telcmd;
extern int	debug;
extern int	apiflag;
extern char *	dialcmd;
extern char *	dquerycmd;
extern int	Startquery;
extern int	Nextquery;
extern int	fonefd;
extern int	fonectlfd;
extern int	cmdpending;
