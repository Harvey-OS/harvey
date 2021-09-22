/*
 * watch an APC (American Power Corporation) UPS, notably shutting down
 * gracefully when power gets too low.  Speaks the APC `Smart' protocol;
 * see www.apcupsd.org/manual/APC_smart_protocol.html, for example.
 * N.B.: connection to UPS is assumed to be on eia1 (2nd serial port)
 * at 2400 bps!
 *
 * original from Nigel Roles.
 */
#include <u.h>
#include <libc.h>

#define DEBUG if (1) print

#define STATUSCHANGE(bit, on) { \
	if (((apc.sts.bits&(bit)) != 0) != (on)) { \
		apc.sts.change |= (bit); \
		apc.sts.bits = (apc.sts.bits&~(bit)) | ((on)? (bit): 0); \
	} \
}
/* make a tick a ms in user land */
#define MS2TK(n) (n)
#define TK2SEC(n) ((n)/1000)

enum {
	OnBattery  = 1,
	LowBattery = 2,
	AbnormalCondition = 4,

	Uartspeed = 2400,
	Trustlowbatt = 0,	// trust the low-battery bit?  i don't.
};
enum { First, Reinit };
enum { Timedout = -1, Kicked = -2, Event = -3 };	/* apcgetc return */

typedef enum { General, Cycle } CmdType;
// typedef enum { UpsOff, JustOff } Action;	/* unimplemented to date */

typedef long Timet;
typedef struct Capability Capability;
struct Capability {
	char	cmd;
	int	n;
	Capability *next;
	char	*val[1];
};

static struct apc {
	Lock;
	int	flag;
	int	gotreply;
	int	kicked;
	int	detected;
	Rendez	r;
	Rendez	doze;
	struct {
		Rendez;
		int	done;
		CmdType	cmdtype;
		char	cmd;
		void	*arg1, *arg2;
		char	resp[500];	/* response */
	} user;
	/* hardware info */
	char	model[50];
	char	fwrev[10];		/* firmware revision */

	Capability *cap;
	struct {
		ulong	bits;
		ulong	change;
	} sts;
	/* battery info */
	Timet	battonticks;		/* ticks with battery on */
	Timet	lastrepticks;		/* ticks at last report */
	ulong	battpct;		/* battery percent */
	ulong	battpctthen;

	ulong	trigger;
//	Action	action;
} apc;

static int apcfd = -1;
static char *apcfile = "/dev/eia1";

void idle(void);

static int
kicked(void*)
{
	return apc.kicked != 0;
}

static void
apcputc(char c)
{
	write(apcfd, &c, 1);
}

static void
apcputs(char *s)
{
	while (*s) {
		sleep(10);
		apcputc(*s++);
	}
}

static int
done(void *p)
{
	return *(int *)p != 0;
}

static int
eitherdone(void *)
{
	return /* TODO input queued? */ 1 || apc.kicked;
}

int
gotnote(void *, char *note)
{
	if (strstr(note, "alarm") != 0)
		return 1;	/* recognised the note */
	return 0;
}

int
apcgetc(int timo, int noevents)
{
	int n, event;
	uchar c;

	if (timo < 0)
		timo = 0;
	atnotify(gotnote, 1);
	do {
		event = 0;
		alarm(timo);
		n = read(apcfd, (char *)&c, 1);
		alarm(0);
		if (n < 0)
			return Timedout;
	
		if (apc.kicked)
			return Kicked;
	
		switch (c) {
		case '!':
			STATUSCHANGE(OnBattery, 1);
			event = 1;
			break;
		case '$':
			STATUSCHANGE(OnBattery, 0);
			event = 1;
			break;
		case '%':
			STATUSCHANGE(LowBattery, 1);
			event = 1;
			break;
		case '+':
			STATUSCHANGE(LowBattery, 0);
			event = 1;
			break;
		case '?':
			STATUSCHANGE(AbnormalCondition, 1);
			event = 1;
			break;
		case '=':
			STATUSCHANGE(AbnormalCondition, 0);
			event = 1;
			break;
		case '*':
			print("apc: turning off\n");
			break;
		case '#':
			print("apc: replace battery\n");
			break;
		case '&':
			print("apc: check alarm register\n");
			break;
		case 0x7c:
			print("apc: eeprom modified\n");
			break;
		}
		if (event) {
			print("apc: event %c\n", c);
			c = Event;
		}
	} while (event && noevents);
//	print("apc: got 0x%ux\n", c);
	return c;
}

char *
apcgets(char *buf, int len, int timo)
{
	char *q;
	int c;

	q = buf;
	while ((c = apcgetc(timo, 1)) >= 0 && c != '\r')
		if (q < buf + len - 1)
			*q++ = c;
	if (c < 0)
		return nil;

	c = apcgetc(timo, 1);
	if (c < 0 || c != '\n')
		return nil;

	*q = 0;
	return buf;
}

int
apcexpect(char *s, int skiprubbish, int timo)
{
	int first = 1;

	while (*s) {
		int c = apcgetc(timo, 1);

		if (c < 0)
			return 0;
		if (*s == c) {
			s++;
			first = 0;
			continue;
		}
		if (!first || !skiprubbish)
			return 0;
		first = 0;
	}
	return 1;
}

int
apcattention(void)				/* anybody home? */
{
	apcputc('Y');
	if (!apcexpect("SM\r\n", 1, 1000))
		return 0;
	apc.detected = 1;
	return 1;
}

char *
apccmdstrresponse(char *cmd, char *buf, int len)
{
	char *s;

	apcputs(cmd);
	s = apcgets(buf, len, 1000);
	if (s == nil) {
		print("APC asleep...\n");
		if (!apcattention())
			return nil;
		apcputs(cmd);
		return apcgets(buf, len, 1000);
	}
	return s;
}

char *
apccmdresponse(char cmd, char *buf, int len)
{
	char cmdstr[2];

	cmdstr[0] = cmd;
	cmdstr[1] = 0;
	return apccmdstrresponse(cmdstr, buf, len);
}

static void
parsecap(char *capstr, char locale)
{
	char cmd, lc, c;
	int n, el, i, j, p;

	while (*capstr) {
		char *s;
		Capability *cap;

		cmd = *capstr++;
		lc = *capstr++;
		n = *capstr++ - '0';
		el = *capstr++ - '0';
		p = lc == '4' || lc == locale;
		if (p) {
			cap = mallocz(sizeof *cap + sizeof s*(n - 1), 1);
			cap->cmd = cmd;
			cap->n = n;
			s = mallocz(n*(el + 1), 1);
			for (i = 0; i < n; i++) {
				cap->val[i] = s + i*(el + 1);
				cap->val[i][el] = 0;
			}
		} else
			cap = nil;
		for (i = 0; i < n; i++)
			for (j = 0; j < el; j++) {
				c = *capstr++;
				if (p)
					cap->val[i][j] = c;
			}
		if (p) {
			cap->next = apc.cap;
			apc.cap = cap;
		}
	}
}

static char *
cyclecmd(Capability *cap, int i)
{
	char *s;

	for (;;) {
		char resp[10];

		s = apccmdresponse(cap->cmd, resp, sizeof resp);
		if (s == nil || strcmp(resp, cap->val[i]) == 0)
			break;
		s = apccmdresponse('-', resp, sizeof resp);
		if (s == nil)
			break;
	}
	return s;
}

static ulong
getfloat(char *p, int dp)
{
	ulong total;
	int afterdp = -1;

	total = 0;
	for (; *p; p++)
		if (*p == '.')
			afterdp = 0;
		else {
			total = total*10 + *p - '0';
			if (afterdp >= 0)
				afterdp++;
		}
	if (afterdp < 0)
		afterdp = 0;
	while (afterdp > dp + 1) {
		afterdp--;
		total /= 10;
	}
	if (afterdp > dp) {
		afterdp--;
		total = (total + 5) / 10;
	}
	while (dp > afterdp) {
		afterdp++;
		total *= 10;
	}
	return total;
}

static int
apcgetstatus(void)
{
	char resp[10];
	ulong status, change;

	do {
		change = apc.sts.change;
		if (apccmdresponse('Q', resp, sizeof(resp)) == nil)
			return 0;
	} while (apc.sts.change != change);
	status = strtoul(resp, 0, 16);
	if (status&(1 << 3) && apc.sts.bits&OnBattery) {	/* online? */
		apc.sts.bits  &= ~OnBattery;
		apc.sts.change |= OnBattery;
	}
	if (status&(1 << 4) && apc.sts.bits&OnBattery) {	/* on battery */
//		apc.sts.bits   |= OnBattery;
		apc.sts.change |= OnBattery;
	}
	if (((status&(1 << 6)) != 0) != ((apc.sts.bits&LowBattery) != 0)) {
		/* low battery */
		apc.sts.bits   ^= LowBattery;
		apc.sts.change |= LowBattery;
	}
	if (apccmdresponse('f', resp, sizeof(resp)) == nil)
		return 0;
	apc.battpct = getfloat(resp, 1);
	return 1;
}

/*
 * shutdown the file server gracefully.
 */
static void
apcshuffle(char *why)
{
	char resp[10];

	print("Shutting down due to %s\n", why);
//	wlock(&mainlock);	/* don't process incoming requests from net */
//	sync("powerfail");
	apccmdstrresponse("@000",  resp, sizeof(resp));
	print("APC responded: '%s'\n", resp);
//	print("File server is now idling.\n");
//	sleep(2000);
//	splhi();
//	for (;;)
//		idle();		/* wait for the lights to go out */
}

static void
apckick(void)
{
	if (apc.detected) {		/* don't blather once per minute */
		print("No APC ups detected\n");
		apc.detected = 0;
	}
	apc.kicked = 0;
//	tsleep(&apc.doze, kicked, 0, 1 * 60 * 1000);
}

static void
apcsetup(int reinit)
{
	int dead;
	Capability *cap;

	if (reinit)
		apckick();
	do {
		while (!apcattention())
			apckick();

		apcputc('\001');		/* ASCII SOH */
		apcgets(apc.model, sizeof(apc.model), -1);
		print("APC UPS model: %s\n", apc.model);

		apcputc('b');
		apcgets(apc.fwrev, sizeof(apc.fwrev), -1);
		print("Firmware revision: %s\n", apc.fwrev);

		apcputc('\032');		/* ASCII SUB */
		apcgets(apc.user.resp, sizeof(apc.user.resp), -1);
		parsecap(apc.user.resp, apc.fwrev[strlen(apc.fwrev) - 1]);

		for (cap = apc.cap; cap; cap = cap->next) {
			int i;

			print("%c %d", cap->cmd, cap->n);
			for (i = 0; i < cap->n; i++)
				print(" %s", cap->val[i]);
			print("\n");
		}

		apc.sts.change = 0;
		dead = 0;
		if (!apcgetstatus()) {
			apckick();
			dead = 1;
		}
	} while (dead);
}

static void
apcbatton(void)
{
	Timet now, nextreport;

	now = time(nil);
	if (apc.sts.change & OnBattery) {
		apc.lastrepticks = apc.battonticks = nextreport = now;
		apc.battpctthen = apc.battpct;
	} else
		nextreport = apc.lastrepticks + MS2TK(30 * 1000);
	if (now - nextreport >= 0) {
		print("apc: on battery %lud seconds (%lud.%lud%%)",
			TK2SEC(now - apc.battonticks),
			apc.battpct / 10, apc.battpct % 10);
		if (apc.battpct < apc.battpctthen - 10) {
			Timet remaining = ((apc.battpct - apc.trigger) *
			  TK2SEC(now - apc.battonticks)) /
			  (apc.battpctthen - apc.battpct);

			print(" - estimated %lud seconds left", remaining);
		}
		print("\n");
		apc.lastrepticks = now;
	}
	if (apc.battpct <= apc.trigger)
		apcshuffle("battery percent too low");
	if (Trustlowbatt && apc.sts.bits & LowBattery)
		apcshuffle("low battery indicator");
}

void
apctask(void)
{
//	tsleep(&apc.doze, kicked, 0, 10 * 1000);

	/* set up the serial port to the UPS */
	DEBUG("apc: running: trigger below %lud%% action %s\n", apc.trigger/10,
		// (apc.action == UpsOff? "ups off": "just off")
		"ups off"
		);
	/* TODO: set line speed to 2400 */
	/* open apcfile^ctl, write "b2400" to it */

	/*
	 * pretend we've been talking to it so we'll get an
	 * error message if it's not there.
	 */
	apc.detected = 1;
	apcsetup(First);
	for (;;) {
		char *s;
		int c;

		if ((apc.sts.bits & OnBattery))
			apcbatton();
		apc.kicked = 0;
		apc.sts.change = 0;

		c = apcgetc(10 * 1000, 0);
		if (c == Timedout || c == Event) {
			if (!apcgetstatus())
				apcsetup(Reinit);
		} else if (c == Kicked) {
			apc.kicked = 0;
			switch (apc.user.cmdtype) {
			case General:
				s = apccmdresponse(apc.user.cmd,
					apc.user.resp, sizeof apc.user.resp);
				break;
			case Cycle:
				s = cyclecmd((Capability *)apc.user.arg1,
					(int)(uintptr)apc.user.arg2);
				break;
			default:
				s = nil;
				break;
			}
			apc.user.done = 1;
//			wakeup(&apc.user);
			if (s == nil)
				apcsetup(Reinit);
		} else
			fprint(2, "%s: unexpected character '%c' (0x%ux)\n",
				argv0, c, c);
	}
}

static void
enquiry(CmdType t, char c, void *arg1, void *arg2)
{
	apc.user.cmdtype = t;
	apc.user.cmd = c;
	apc.user.arg1 = arg1;
	apc.user.arg2 = arg2;
	apc.user.done = 0;
	apc.kicked = 1;
	apc.user.resp[0] = 0;
//	wakeup(&apc.rxq);
	/*
	 * BUG: can hang here forever if cable to UPS falls out or
	 * is wired wrong (need a null modem).
	 */
//	sleep(&apc.user, done, &apc.user.done);
	if (apc.user.resp[0]) {
		print("'%s'\n", apc.user.resp);
		apc.user.resp[0] = 0;
	}
}

static struct {
	char	ch;
	char	*cmd;
} genenq[] = {
	{ '\032', "capabilities" },	/* ASCII SUB */
	{ 'B', "batteryvolts" },
	{ 'C', "temperature" },
	{ 'E', "selftestinterval" },
	{ 'F', "frequency" },
	{ 'L', "lineinvolts" },
	{ 'M', "maxlineinvolts" },
	{ 'N', "minlineinvolts" },
	{ 'O', "lineoutvolts" },
	{ 'P', "powerload" },
	{ 'Q', "status" },
	{ 'V', "firmware" },
	{ 'X', "selftestresults" },
	{ 'a', "protocol" },
	{ 'b', "localid" },
	{ 'e', "returnthresh" },
	{ 'g', "nominalbatteryvolts" },
	{ 'f', "battpct" },
	{ 'h', "humidity" },
	{ 'i', "contacts" },
	{ 'j', "runtime" },
	{ 'k', "alarmdelay" },
	{ 'l', "lowtransfervolts" },
	{ 'm', "manufactured" },
	{ 'n', "serial" },
	{ 'o', "onbatteryvolts" },
	{ 'p', "grace" },
	{ 'q', "lowbatterywarntime" },
	{ 'r', "wakeupdelay" },
	{ 's', "sensitivity" },
	{ 'u', "uppertransfervolts" },
	{ 'x', "lastbatterychange" },
	{ 'y', "copyright" },
	{ '~', "register1" },
	{ ''', "register2" },
	{ '7', "switches" },
	{ '8', "register3" },
	{ '9', "linequality" },
	{ '>', "batterypacks" },
	{ '-', "cycle" },
};

int
vaguelyequal(char *a, char *b)
{
	return strcmp(a, b) == 0;
}

static void
cycle(char *name, char *val)
{
	int g, i;
	Capability *cap;

	if (strcmp(name, "trigger") == 0) {
		apc.trigger = getfloat(val, 1);
		return;
	}

	/* convert name to enquiry */
	for (g = 0; g < nelem(genenq); g++)
		if (strcmp(name, genenq[g].cmd) == 0)
			break;
	if (g >= nelem(genenq)) {
		print("no such parameter '%s'\n", name);
		return;
	}
	/* match enquiry to capability */
	for (cap = apc.cap; cap; cap = cap->next)
		if (cap->cmd == genenq[g].ch)
			break;
	if (cap == nil) {
		print("parameter %s cannot be set\n", name);
		return;
	}
	/* search capability's legal values */
	for (i = 0; i < cap->n; i++)
		if (vaguelyequal(cap->val[i], val))
			break;
	if (i >= cap->n) {
		print("%s: illegal value %s; try one of [", name, val);
		for (i = 0; i < cap->n; i++) {
			if (i > 0)
				print(" ");
			print("%s", cap->val[i]);
		}
		print("]\n");
	} else
		enquiry(Cycle, cap->cmd, cap, (void *)i);
}

void
cmd_apc(int argc, char *argv[])
{
	int i, x;

	if(argc <= 1) {
		print("apc kick -- play now\n");
		print("apc set var val -- set var to val\n");
		print("apc enquiry... -- query the ups\n");
		return;
	}
	for (i = 1; i < argc; i++) {
		if(strcmp(argv[i], "kick") == 0) {
			apc.kicked = 1;
//			wakeup(&apc.doze);
			continue;
		}
		if(strcmp(argv[i], "set") == 0) {
			if (argc - i >= 3)
				cycle(argv[i + 1], argv[i + 2]);
			i += 2;
			continue;
		}
		for (x = 0; x < nelem(genenq); x++)
			if (strcmp(argv[i], genenq[x].cmd) == 0)
				break;
		if (x < nelem(genenq))
			enquiry(General, genenq[x].ch, nil, nil);
		else {
			print("no such parameter '%s'\n", argv[i]);
			break;
		}
	}
}

void
usage(void)
{
	fprint(2, "usage: %s [-f apcfile]\n", argv0);
	exits("usage");
}

void
main(int argc, char **argv)
{
	ARGBEGIN{
	case 'f':
		apcfile = EARGF(usage());
		break;
	default:
		usage();
	}ARGEND;
	apcfd = open(apcfile, ORDWR);
	if (apcfd < 0)
		sysfatal("can't open %s: %r", apcfile);
	apc.trigger = 1000;
	// apc.action = UpsOff;
	/* TODO: provide options to set trigger */
	apctask();
}
