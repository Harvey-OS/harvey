/*
 * manage APC (American Power Corporation) UPS, notably shutting down
 * gracefully when power gets too low. enabled by "ups0=type=apc" in plan9.ini.
 * N.B.: connection to UPS is assumed to be on eia1 (2nd serial port)
 * at 2400 bps!  due to hardcoded use of uartputc1, etc., probably can't
 * override port with "port=0" in plan9.ini entry.
 */
#include "all.h"
#include "mem.h"
#include "io.h"

//#define DEBUG if(cons.flags&apc.flag)print
#define DEBUG print

#define STATUSCHANGE(bit, on) { \
	if (((apc.status.bits&(bit)) != 0) != (on)) { \
		apc.status.change |= (bit); \
		apc.status.bits = (apc.status.bits&~(bit)) | ((on)? (bit): 0); \
	} \
}

enum {
	OnBattery = 1,
	LowBattery = 2,
	AbnormalCondition = 4,
	Uartspeed = 2400,
	Trustlowbatt = 0,	// trust the low-battery bit?  i don't.
};
enum { First, Reinit };
enum { Timedout = -1, Kicked = -2, Event = -3 };	/* apcgetc return */

typedef enum { General, Cycle } CmdType;
typedef enum { UpsOff, JustOff } Action;	/* unimplemented to date */

typedef struct Capability {
	char	cmd;
	int	n;
	struct Capability *next;
	char	*val[1];
} Capability;

static struct apc {
	Lock;
	int	flag;
	int	gotreply;
	int	kicked;
	int	detected;
	Rendez	r;
	Rendez	doze;
	struct {
		Lock;
		Rendez;
		int	count;
		uchar	buf[100];
		uchar	*in;
		uchar	*out;
	} rxq;
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
	} status;
	/* battery info */
	Timet	battonticks;		/* ticks with battery on */
	Timet	lastrepticks;		/* ticks at last report */
	ulong	battpct;		/* battery percent */
	ulong	battpctthen;

	ulong	trigger;
	Action	action;
	int	port;
} apc;

extern void uartspecial1(int port, void (*rx)(int), int (*tx)(void), int baud);
extern void uartputc1(int c);

void idle(void);

static int
kicked(void*)
{
	return apc.kicked != 0;
}

static int
apctxint(void)
{
	return -1;
}

static void
apcputc(char c)
{
	uartputc1(c);
}

static void
apcputs(char *s)
{
	while (*s) {
		delay(10);
		apcputc(*s++);
	}
}

void
apcrxint(int c)
{
	int s;
	uchar *p;

	s = splhi();
	lock(&apc.rxq);
	if (apc.rxq.count < sizeof(apc.rxq.buf)) {
		p = apc.rxq.in;
		*p++ = c;
		if(p >= apc.rxq.buf + sizeof(apc.rxq.buf))
			p = apc.rxq.buf;
		apc.rxq.in = p;
		apc.rxq.count++;
		wakeup(&apc.rxq);
	}
	unlock(&apc.rxq);
	splx(s);
}

static int
done(void *p)
{
	return *(int *)p != 0;
}

static int
eitherdone(void *)
{
	return apc.rxq.count != 0 || apc.kicked;
}

int
apcgetc(int timo, int noevents)
{
	char c;
	int s;

loop:
	if (timo < 0)
		sleep(&apc.rxq, eitherdone, 0);
	else
		tsleep(&apc.rxq, eitherdone, 0, timo);
	if (apc.kicked)
		return Kicked;

	s = splhi();
	lock(&apc.rxq);
	if (apc.rxq.count == 0) {
		unlock(&apc.rxq);
		splx(s);
		if (timo >= 0)
			return Timedout;
		goto loop;
	}
	c = *apc.rxq.out++;
	if (apc.rxq.out >= apc.rxq.buf + sizeof(apc.rxq.buf))
		apc.rxq.out = apc.rxq.buf;
	apc.rxq.count--;
	unlock(&apc.rxq);
	splx(s);

	switch (c) {
	case '!':
		STATUSCHANGE(OnBattery, 1);
report_event:
		print("apc: event %c\n", c);
		if (noevents)
			goto loop;
		return Event;
	case '$':
		STATUSCHANGE(OnBattery, 0);
		goto report_event;
	case '%':
		STATUSCHANGE(LowBattery, 1);
		goto report_event;
	case '+':
		STATUSCHANGE(LowBattery, 0);
		goto report_event;
	case '?':
		STATUSCHANGE(AbnormalCondition, 1);
		goto report_event;
	case '=':
		STATUSCHANGE(AbnormalCondition, 0);
		goto report_event;
	case '*':
		print("apc: turning off\n");
		goto loop;
	case '#':
		print("apc: replace battery\n");
		goto loop;
	case '&':
		print("apc: check alarm register\n");
		goto loop;
	case 0x7c:
		print("apc: eeprom modified\n");
		goto loop;
	default:
		break;
	}

//	print("apc: got 0x%.2ux\n", c);
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
		if (!first)
			return 0;
		if (!skiprubbish)
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
			cap = ialloc(sizeof *cap + sizeof s*(n - 1), 0);
			cap->cmd = cmd;
			cap->n = n;
			s = ialloc(n*(el + 1), 0);
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

		s = apccmdresponse(cap->cmd, resp, sizeof(resp));
		if (s == nil || strcmp(resp, cap->val[i]) == 0)
			break;
		s = apccmdresponse('-', resp, sizeof(resp));
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
		change = apc.status.change;
		if (apccmdresponse('Q', resp, sizeof(resp)) == nil)
			return 0;
	} while (apc.status.change != change);
	status = strtoul(resp, 0, 16);
	if (status&(1 << 3) && apc.status.bits&OnBattery) {	/* online? */
		apc.status.bits  &= ~OnBattery;
		apc.status.change |= OnBattery;
	}
	if (status&(1 << 4) && apc.status.bits&OnBattery) {	/* on battery */
//		apc.status.bits   |= OnBattery;
		apc.status.change |= OnBattery;
	}
	if (((status&(1 << 6)) != 0) != ((apc.status.bits&LowBattery) != 0)) {
		/* low battery */
		apc.status.bits   ^= LowBattery;
		apc.status.change |= LowBattery;
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
	wlock(&mainlock);	/* don't process incoming requests from net */
	sync("powerfail");
	apccmdstrresponse("@000",  resp, sizeof(resp));
	print("APC responded: '%s'\n", resp);
	print("File server is now idling.\n");
	delay(2000);
	splhi();
	for (;;)
		idle();		/* wait for the lights to go out */
}

static void
apckick(void)
{
	if (apc.detected) {		/* don't blather once per minute */
		print("No APC ups detected\n");
		apc.detected = 0;
	}
	apc.kicked = 0;
	tsleep(&apc.doze, kicked, 0, 1 * 60 * 1000);
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

		apcputc(1);
		apcgets(apc.model, sizeof(apc.model), -1);
		print("APC UPS model: %s\n", apc.model);

		apcputc('b');
		apcgets(apc.fwrev, sizeof(apc.fwrev), -1);
		print("Firmware revision: %s\n", apc.fwrev);

		apcputc('');
		apcgets(apc.user.resp, sizeof(apc.user.resp), -1);
		parsecap(apc.user.resp, apc.fwrev[strlen(apc.fwrev) - 1]);

		for (cap = apc.cap; cap; cap = cap->next) {
			int i;

			print("%c %d", cap->cmd, cap->n);
			for (i = 0; i < cap->n; i++)
				print(" %s", cap->val[i]);
			print("\n");
		}

		apc.status.change = 0;
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

	now = MACHP(0)->ticks;
	if (apc.status.change & OnBattery) {
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
	if (Trustlowbatt && apc.status.bits & LowBattery)
		apcshuffle("low battery indicator");
}

void
apctask(void)
{
	tsleep(&apc.doze, kicked, 0, 10 * 1000);

	/* set up the serial port to the UPS */
	DEBUG("apc: running: port %d trigger below %lud%% action %s\n",
		apc.port, apc.trigger / 10,
		(apc.action == UpsOff? "ups off": "just off"));
	apc.rxq.in = apc.rxq.out = apc.rxq.buf;
	uartspecial1(apc.port, apcrxint, apctxint, Uartspeed);

	/*
	 * pretend we've been talking to it so we'll get an
	 * error message if it's not there.
	 */
	apc.detected = 1;
	apcsetup(First);
	for (;;) {
		char *s;
		int c;

		if ((apc.status.bits & OnBattery))
			apcbatton();
		apc.kicked = 0;
		apc.status.change = 0;

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
					(int)apc.user.arg2);
				break;
			default:
				s = nil;
				break;
			}
			apc.user.done = 1;
			wakeup(&apc.user);
			if (s == nil)
				apcsetup(Reinit);
		} else
			print("apc: unexpected character '%c' (0x%.2ux)\n",
				c, c);
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
	wakeup(&apc.rxq);
	/*
	 * BUG: can hang here forever if cable to UPS falls out or
	 * is wired wrong (need a null modem).
	 */
	sleep(&apc.user, done, &apc.user.done);
	if (apc.user.resp[0]) {
		print("'%s'\n", apc.user.resp);
		apc.user.resp[0] = 0;
	}
}

static struct {
	char ch;
	char *cmd;
} generalenquiries[] = {
	{ '', "capabilities" },
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
	for (g = 0; g < nelem(generalenquiries); g++)
		if (strcmp(name, generalenquiries[g].cmd) == 0)
			break;
	if (g >= nelem(generalenquiries)) {
		print("no such parameter '%s'\n", name);
		return;
	}
	/* match enquiry to capability */
	for (cap = apc.cap; cap; cap = cap->next)
		if (cap->cmd == generalenquiries[g].ch)
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
			wakeup(&apc.doze);
			continue;
		}
		if(strcmp(argv[i], "set") == 0) {
			if (argc - i >= 3)
				cycle(argv[i + 1], argv[i + 2]);
			i += 2;
			continue;
		}
		for (x = 0; x < nelem(generalenquiries); x++)
			if (strcmp(argv[i], generalenquiries[x].cmd) == 0)
				break;
		if (x < nelem(generalenquiries))
			enquiry(General, generalenquiries[x].ch, nil, nil);
		else {
			print("no such parameter '%s'\n", argv[i]);
			return;
		}
	}
}

void
apcinit(void)
{
	ISAConf isa;
	int o;

	print("apcinit...");
	memset(&isa, 0, sizeof isa);	/* prevent surprises */
	isa.port = 1;			/* default port */
	if (!isaconfig("ups", 0, &isa) || strcmp(isa.type, "apc") != 0) {
		print("no ups in plan9.ini, or not type `apc'\n");
		return;
	}

	cmd_install("apc", "subcommand -- apc ups driver", cmd_apc);
	apc.flag = flag_install("apc", "-- verbose");

	apc.trigger = 1000;
	apc.action = UpsOff;

	for (o = 0; o < isa.nopt; o++)
		if (cistrncmp(isa.opt[o], "trigger=", 8) == 0)
			apc.trigger = strtoul(isa.opt[o] + 8, 0, 0) * 10;
		else if (cistrncmp(isa.opt[o], "action=", 7) == 0) {
			if (strcmp(isa.opt[o] + 8, "off") == 0)
				apc.action = JustOff;
		}

	apc.port = isa.port;
	/*
	 * it's a little early to be starting this, before config mode is
	 * even started.
	 */
	print("apc...\n");
	userinit(apctask, 0, "apc");
}
