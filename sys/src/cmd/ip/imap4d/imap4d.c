#include <u.h>
#include <libc.h>
#include <auth.h>
#include <bio.h>
#include "imap4d.h"

/*
 * these should be in libraries
 */
char	*csquery(char *attr, char *val, char *rattr);

/*
 * /lib/rfc/rfc2060 imap4rev1
 * /lib/rfc/rfc2683 is implementation advice
 * /lib/rfc/rfc2342 is namespace capability
 * /lib/rfc/rfc2222 is security protocols
 * /lib/rfc/rfc1731 is security protocols
 * /lib/rfc/rfc2221 is LOGIN-REFERRALS
 * /lib/rfc/rfc2193 is MAILBOX-REFERRALS
 * /lib/rfc/rfc2177 is IDLE capability
 * /lib/rfc/rfc2195 is CRAM-MD5 authentication
 * /lib/rfc/rfc2088 is LITERAL+ capability
 * /lib/rfc/rfc1760 is S/Key authentication
 *
 * outlook uses "Secure Password Authentication" aka ntlm authentication
 *
 * capabilities from nslocum
 * CAPABILITY IMAP4 IMAP4REV1 NAMESPACE IDLE SCAN SORT MAILBOX-REFERRALS LOGIN-REFERRALS AUTH=LOGIN THREAD=ORDEREDSUBJECT
 */

typedef struct	ParseCmd	ParseCmd;

enum
{
	UlongMax	= 4294967295,
};

struct ParseCmd
{
	char	*name;
	void	(*f)(char *tg, char *cmd);
};

static	void	appendCmd(char *tg, char *cmd);
static	void	authenticateCmd(char *tg, char *cmd);
static	void	capabilityCmd(char *tg, char *cmd);
static	void	closeCmd(char *tg, char *cmd);
static	void	copyCmd(char *tg, char *cmd);
static	void	createCmd(char *tg, char *cmd);
static	void	deleteCmd(char *tg, char *cmd);
static	void	expungeCmd(char *tg, char *cmd);
static	void	fetchCmd(char *tg, char *cmd);
static	void	idleCmd(char *tg, char *cmd);
static	void	listCmd(char *tg, char *cmd);
static	void	loginCmd(char *tg, char *cmd);
static	void	logoutCmd(char *tg, char *cmd);
static	void	namespaceCmd(char *tg, char *cmd);
static	void	noopCmd(char *tg, char *cmd);
static	void	renameCmd(char *tg, char *cmd);
static	void	searchCmd(char *tg, char *cmd);
static	void	selectCmd(char *tg, char *cmd);
static	void	statusCmd(char *tg, char *cmd);
static	void	storeCmd(char *tg, char *cmd);
static	void	subscribeCmd(char *tg, char *cmd);
static	void	uidCmd(char *tg, char *cmd);
static	void	unsubscribeCmd(char *tg, char *cmd);

static	void	copyUCmd(char *tg, char *cmd, int uids);
static	void	fetchUCmd(char *tg, char *cmd, int uids);
static	void	searchUCmd(char *tg, char *cmd, int uids);
static	void	storeUCmd(char *tg, char *cmd, int uids);

static	void	imap4(int);
static	void	status(int expungeable, int uids);
static	void	cleaner(void);
static	void	check(void);
static	int	catcher(void*, char*);

static	Search	*searchKey(int first);
static	Search	*searchKeys(int first, Search *tail);
static	char	*astring(void);
static	char	*atomString(char *disallowed, char *initial);
static	char	*atom(void);
static	void	badsyn(void);
static	void	clearcmd(void);
static	char	*command(void);
static	void	crnl(void);
static	Fetch	*fetchAtt(char *s, Fetch *f);
static	Fetch	*fetchWhat(void);
static	int	flagList(void);
static	int	flags(void);
static	int	getc(void);
static	char	*listmbox(void);
static	char	*literal(void);
static	ulong	litlen(void);
static	MsgSet	*msgSet(int);
static	void	mustBe(int c);
static	ulong	number(int nonzero);
static	int	peekc(void);
static	char	*quoted(void);
static	void	sectText(Fetch *f, int mimeOk);
static	ulong	seqNo(void);
static	Store	*storeWhat(void);
static	char	*tag(void);
static	ulong	uidNo(void);
static	void	ungetc(void);

static	ParseCmd	SNonAuthed[] =
{
	{"capability",		capabilityCmd},
	{"logout",		logoutCmd},
	{"x-exit",		logoutCmd},
	{"noop",		noopCmd},

	{"login",		loginCmd},
	{"authenticate",	authenticateCmd},

	nil
};

static	ParseCmd	SAuthed[] =
{
	{"capability",		capabilityCmd},
	{"logout",		logoutCmd},
	{"x-exit",		logoutCmd},
	{"noop",		noopCmd},

	{"append",		appendCmd},
	{"create",		createCmd},
	{"delete",		deleteCmd},
	{"examine",		selectCmd},
	{"select",		selectCmd},
	{"idle",		idleCmd},
	{"list",		listCmd},
	{"lsub",		listCmd},
	{"namespace",		namespaceCmd},
	{"rename",		renameCmd},
	{"status",		statusCmd},
	{"subscribe",		subscribeCmd},
	{"unsubscribe",		unsubscribeCmd},

	nil
};

static	ParseCmd	SSelected[] =
{
	{"capability",		capabilityCmd},
	{"logout",		logoutCmd},
	{"x-exit",		logoutCmd},
	{"noop",		noopCmd},

	{"append",		appendCmd},
	{"create",		createCmd},
	{"delete",		deleteCmd},
	{"examine",		selectCmd},
	{"select",		selectCmd},
	{"idle",		idleCmd},
	{"list",		listCmd},
	{"lsub",		listCmd},
	{"namespace",		namespaceCmd},
	{"rename",		renameCmd},
	{"status",		statusCmd},
	{"subscribe",		subscribeCmd},
	{"unsubscribe",		unsubscribeCmd},

	{"check",		noopCmd},
	{"close",		closeCmd},
	{"copy",		copyCmd},
	{"expunge",		expungeCmd},
	{"fetch",		fetchCmd},
	{"search",		searchCmd},
	{"store",		storeCmd},
	{"uid",			uidCmd},

	nil
};

static	char		*atomStop = "(){%*\"\\";
static	Chalstate	*chal;
static	int		chaled;
static	ParseCmd	*imapState;
static	jmp_buf		parseJmp;
static	char		*parseMsg;
static	int		allowPass;
static	int		allowCR;
static	int		exiting;
static	QLock		imaplock;
static	int		idlepid = -1;

Biobuf	bout;
Biobuf	bin;
char	username[UserNameLen];
char	mboxDir[MboxNameLen];
char	*servername;
char	*site;
char	*remote;
Box	*selected;
Bin	*parseBin;
int	debug;

void
main(int argc, char *argv[])
{
	char *s, *t;
	int preauth, n;

	Binit(&bin, 0, OREAD);
	Binit(&bout, 1, OWRITE);

	preauth = 0;
	allowPass = 0;
	allowCR = 0;
	ARGBEGIN{
	case 'a':
		preauth = 1;
		break;
	case 'd':
		site = ARGF();
		break;
	case 'c':
		allowCR = 1;
		break;
	case 'p':
		allowPass = 1;
		break;
	case 'r':
		remote = ARGF();
		break;
	case 's':
		servername = ARGF();
		break;
	case 'v':
		debug = 1;
		debuglog("imap4d debugging enabled\n");
		break;
	default:
		fprint(2, "usage: ip/imap4d [-acpv] [-d site] [-r remotehost] [-s servername]\n");
		bye("usage");
		break;
	}ARGEND

	if(allowPass && allowCR){
		fprint(2, "%s: -c and -p are mutually exclusive\n", argv0);
		bye("usage");
	}

	if(preauth)
		setupuser(nil);

	if(servername == nil){
		servername = csquery("sys", sysname(), "dom");
		if(servername == nil)
			servername = sysname();
		if(servername == nil){
			fprint(2, "ip/imap4d can't find server name: %r\n");
			bye("can't find system name");
		}
	}
	if(site == nil){
		t = getenv("site");
		if(t == nil)
			site = servername;
		else{
			n = strlen(t);
			s = strchr(servername, '.');
			if(s == nil)
				s = servername;
			else
				s++;
			n += strlen(s) + 2;
			site = emalloc(n);
			snprint(site, n, "%s.%s", t, s);
		}
	}

	rfork(RFNOTEG|RFREND);

	atnotify(catcher, 1);
	qlock(&imaplock);
	atexit(cleaner);
	imap4(preauth);
}

static void
imap4(int preauth)
{
	char *volatile tg;
	char *volatile cmd;
	ParseCmd *st;

	if(preauth){
		Bprint(&bout, "* preauth %s IMAP4rev1 server ready user %s authenticated\r\n", servername, username);
		imapState = SAuthed;
	}else{
		Bprint(&bout, "* OK %s IMAP4rev1 server ready\r\n", servername);
		imapState = SNonAuthed;
	}
	if(Bflush(&bout) < 0)
		writeErr();

	chaled = 0;

	tg = nil;
	cmd = nil;
	if(setjmp(parseJmp)){
		if(tg == nil)
			Bprint(&bout, "* bad empty command line: %s\r\n", parseMsg);
		else if(cmd == nil)
			Bprint(&bout, "%s BAD no command: %s\r\n", tg, parseMsg);
		else
			Bprint(&bout, "%s BAD %s %s\r\n", tg, cmd, parseMsg);
		clearcmd();
		if(Bflush(&bout) < 0)
			writeErr();
		binfree(&parseBin);
	}
	for(;;){
		if(mbLocked())
			bye("internal error: mailbox lock held");
		tg = nil;
		cmd = nil;
		tg = tag();
		mustBe(' ');
		cmd = atom();

		/*
		 * note: outlook express is broken: it requires echoing the
		 * command as part of matching response
		 */
		for(st = imapState; st->name != nil; st++){
			if(cistrcmp(cmd, st->name) == 0){
				(*st->f)(tg, cmd);
				break;
			}
		}
		if(st->name == nil){
			clearcmd();
			Bprint(&bout, "%s BAD %s illegal command\r\n", tg, cmd);
		}

		if(Bflush(&bout) < 0)
			writeErr();
		binfree(&parseBin);
	}
}

void
bye(char *fmt, ...)
{
	va_list arg;

	va_start(arg, fmt);
	Bprint(&bout, "* bye ");
	Bvprint(&bout, fmt, arg);
	Bprint(&bout, "\r\n");
	Bflush(&bout);
exits("rob2");
	exits(0);
}

void
parseErr(char *msg)
{
	parseMsg = msg;
	longjmp(parseJmp, 1);
}

/*
 * an error occured while writing to the client
 */
void
writeErr(void)
{
	cleaner();
	_exits("connection closed");
}

static int
catcher(void *v, char *msg)
{
	USED(v);
	if(strstr(msg, "closed pipe") != nil)
		return 1;
	return 0;
}

/*
 * wipes out the idleCmd backgroung process if it is around.
 * this can only be called if the current proc has qlocked imaplock.
 * it must be the last piece of imap4d code executed.
 */
static void
cleaner(void)
{
	int i;

	if(idlepid < 0)
		return;
	exiting = 1;
	close(0);
	close(1);
	close(2);

	/*
	 * the other proc is either stuck in a read, a sleep,
	 * or is trying to lock imap4lock.
	 * get him out of it so he can exit cleanly
	 */
	qunlock(&imaplock);
	for(i = 0; i < 4; i++)
		postnote(PNGROUP, getpid(), "die");
}

/*
 * send any pending status updates to the client
 * careful: shouldn't exit, because called by idle polling proc
 *
 * can't always send pending info
 * in particular, can't send expunge info
 * in response to a fetch, store, or search command.
 * 
 * rfc2060 5.2:	server must send mailbox size updates
 * rfc2060 5.2:	server may send flag updates
 * rfc2060 5.5:	servers prohibited from sending expunge while fetch, store, search in progress
 * rfc2060 7:	in selected state, server checks mailbox for new messages as part of every command
 * 		sends untagged EXISTS and RECENT respsonses reflecting new size of the mailbox
 * 		should also send appropriate untagged FETCH and EXPUNGE messages if another agent
 * 		changes the state of any message flags or expunges any messages
 * rfc2060 7.4.1	expunge server response must not be sent when no command is in progress,
 * 		nor while responding to a fetch, stort, or search command (uid versions are ok)
 * 		command only "in progress" after entirely parsed.
 *
 * strategy for third party deletion of messages or of a mailbox
 *
 * deletion of a selected mailbox => act like all message are expunged
 *	not strictly allowed by rfc2180, but close to method 3.2.
 *
 * renaming same as deletion
 *
 * copy
 *	reject iff a deleted message is in the request
 *
 * search, store, fetch operations on expunged messages
 *	ignore the expunged messages
 *	return tagged no if referenced
 */
static void
status(int expungeable, int uids)
{
	int tell;

	if(!selected)
		return;
	tell = 0;
	if(expungeable)
		tell = expungeMsgs(selected, 1);
	if(selected->sendFlags)
		sendFlags(selected, uids);
	if(tell || selected->toldMax != selected->max){
		Bprint(&bout, "* %lud EXISTS\r\n", selected->max);
		selected->toldMax = selected->max;
	}
	if(tell || selected->toldRecent != selected->recent){
		Bprint(&bout, "* %lud RECENT\r\n", selected->recent);
		selected->toldRecent = selected->recent;
	}
	if(tell)
		closeImp(selected, checkBox(selected, 1));
}

/*
 * careful: can't exit, because called by idle polling proc
 */
static void
check(void)
{
	if(!selected)
		return;
	checkBox(selected, 0);
	status(1, 0);
}

static void
appendCmd(char *tg, char *cmd)
{
	char *mbox, head[128];
	ulong t, n, now;
	int flags, ok;

	mustBe(' ');
	mbox = astring();
	mustBe(' ');
	flags = 0;
	if(peekc() == '('){
		flags = flagList();
		mustBe(' ');
	}
	now = time(nil);
	if(peekc() == '"'){
		t = imap4DateTime(quoted());
		if(t == ~0)
			parseErr("illegal date format");
		mustBe(' ');
		if(t > now)
			t = now;
	}else
		t = now;
	n = litlen();

	mbox = mboxName(mbox);
	if(mbox == nil || !okMbox(mbox)){
		check();
		Bprint(&bout, "%s NO %s bad mailbox\r\n", tg, cmd);
		return;
	}
	if(!cdExists(mboxDir, mbox)){
		check();
		Bprint(&bout, "%s NO [TRYCREATE] %s mailbox does not exist\r\n", tg, cmd);
		return;
	}

	snprint(head, sizeof(head), "From %s %s", username, ctime(t));
	ok = appendSave(mbox, flags, head, &bin, n);
	crnl();
	check();
	if(ok)
		Bprint(&bout, "%s OK %s completed\r\n", tg, cmd);
	else
		Bprint(&bout, "%s NO %s message save failed\r\n", tg, cmd);
}

static void
authenticateCmd(char *tg, char *cmd)
{
	char *s, *t;

	mustBe(' ');
	s = atom();
	crnl();
	auth_freechal(chal);
	chal = nil;
	if(cistrcmp(s, "cram-md5") == 0){
		t = cramauth();
		if(t == nil){
			Bprint(&bout, "%s OK %s\r\n", tg, cmd);
			imapState = SAuthed;
		}else
			Bprint(&bout, "%s NO %s failed %s\r\n", tg, cmd, t);
	}else
		Bprint(&bout, "%s NO %s unsupported authentication protocol\r\n", tg, cmd);
}

static void
capabilityCmd(char *tg, char *cmd)
{
	crnl();
	check();
// nslocum's capabilities
//	Bprint(&bout, "* CAPABILITY IMAP4 IMAP4REV1 NAMESPACE IDLE SCAN SORT MAILBOX-REFERRALS LOGIN-REFERRALS AUTH=LOGIN THREAD=ORDEREDSUBJECT\r\n");
	Bprint(&bout, "* CAPABILITY IMAP4REV1 IDLE NAMESPACE AUTH=CRAM-MD5\r\n");
	Bprint(&bout, "%s OK %s\r\n", tg, cmd);
}

static void
closeCmd(char *tg, char *cmd)
{
	crnl();
	imapState = SAuthed;
	closeBox(selected, 1);
	selected = nil;
	Bprint(&bout, "%s OK %s mailbox closed, now in authenticated state\r\n", tg, cmd);
}

/*
 * note: message id's are before any pending expunges
 */
static void
copyCmd(char *tg, char *cmd)
{
	copyUCmd(tg, cmd, 0);
}

static void
copyUCmd(char *tg, char *cmd, int uids)
{
	MsgSet *ms;
	char *uid, *mbox;
	ulong max;
	int ok;

	mustBe(' ');
	ms = msgSet(uids);
	mustBe(' ');
	mbox = astring();
	crnl();

	uid = "";
	if(uids)
		uid = "uid ";

	mbox = mboxName(mbox);
	if(mbox == nil || !okMbox(mbox)){
		status(1, uids);
		Bprint(&bout, "%s NO %s%s bad mailbox\r\n", tg, uid, cmd);
		return;
	}
	if(cistrcmp(mbox, "inbox") == 0)
		mbox = "mbox";
	if(!cdExists(mboxDir, mbox)){
		check();
		Bprint(&bout, "%s NO [TRYCREATE] %s mailbox does not exist\r\n", tg, cmd);
		return;
	}

	max = selected->max;
	checkBox(selected, 0);
	ok = forMsgs(selected, ms, max, uids, copyCheck, nil);
	if(ok)
		ok = forMsgs(selected, ms, max, uids, copySave, mbox);

	status(1, uids);
	if(ok)
		Bprint(&bout, "%s OK %s%s completed\r\n", tg, uid, cmd);
	else
		Bprint(&bout, "%s NO %s%s failed\r\n", tg, uid, cmd);
}

static void
createCmd(char *tg, char *cmd)
{
	char *mbox, *m;
	int fd, slash;

	mustBe(' ');
	mbox = astring();
	crnl();
	check();

	m = strchr(mbox, '\0');
	slash = m != mbox && m[-1] == '/';
	mbox = mboxName(mbox);
	if(mbox == nil || !okMbox(mbox)){
		Bprint(&bout, "%s NO %s bad mailbox\r\n", tg, cmd);
		return;
	}
	if(cistrcmp(mbox, "inbox") == 0){
		Bprint(&bout, "%s NO %s cannot remotely create INBOX\r\n", tg, cmd);
		return;
	}
	if(access(mbox, AEXIST) >= 0){
		Bprint(&bout, "%s NO %s mailbox already exists\r\n", tg, cmd);
		return;
	}

	fd = createBox(mbox, slash);
	close(fd);
	if(fd < 0)
		Bprint(&bout, "%s NO %s cannot create mailbox %s\r\n", tg, cmd, mbox);
	else
		Bprint(&bout, "%s OK %s %s completed\r\n", tg, mbox, cmd);
}

static void
deleteCmd(char *tg, char *cmd)
{
	char *mbox, *imp;

	mustBe(' ');
	mbox = astring();
	crnl();
	check();

	mbox = mboxName(mbox);
	if(mbox == nil || !okMbox(mbox)){
		Bprint(&bout, "%s NO %s bad mailbox\r\n", tg, cmd);
		return;
	}

	imp = impName(mbox);
	if(cistrcmp(mbox, "inbox") == 0
	|| imp != nil && cdRemove(mboxDir, imp) < 0 && cdExists(mboxDir, imp)
	|| cdRemove(mboxDir, mbox) < 0)
		Bprint(&bout, "%s NO %s cannot delete mailbox %s\r\n", tg, cmd, mbox);
	else
		Bprint(&bout, "%s OK %s %s completed\r\n", tg, mbox, cmd);
}

static void
expungeCmd(char *tg, char *cmd)
{
	int ok;

	crnl();
	ok = deleteMsgs(selected);
	check();
	if(ok)
		Bprint(&bout, "%s OK %s messages erased\r\n", tg, cmd);
	else
		Bprint(&bout, "%s NO %s some messages not expunged\r\n", tg, cmd);
}

static void
fetchCmd(char *tg, char *cmd)
{
	fetchUCmd(tg, cmd, 0);
}

static void
fetchUCmd(char *tg, char *cmd, int uids)
{
	Fetch *f;
	MsgSet *ms;
	MbLock *ml;
	char *uid;
	ulong max;
	int ok;

	mustBe(' ');
	ms = msgSet(uids);
	mustBe(' ');
	f = fetchWhat();
	crnl();
	uid = "";
	if(uids)
		uid = "uid ";
	max = selected->max;
	ml = checkBox(selected, 1);
	if(ml != nil)
		forMsgs(selected, ms, max, uids, fetchSeen, f);
	closeImp(selected, ml);
	ok = ml != nil && forMsgs(selected, ms, max, uids, fetchMsg, f);
	status(uids, uids);
	if(ok)
		Bprint(&bout, "%s OK %s%s completed\r\n", tg, uid, cmd);
	else
		Bprint(&bout, "%s NO %s%s failed\r\n", tg, uid, cmd);
}

static void
idleCmd(char *tg, char *cmd)
{
	int c, pid;

	crnl();
	Bprint(&bout, "+ idling, waiting for done\r\n");
	if(Bflush(&bout) < 0)
		writeErr();

	if(idlepid < 0){
		pid = rfork(RFPROC|RFMEM|RFNOWAIT);
		if(pid == 0){
			for(;;){
				qlock(&imaplock);
				if(exiting)
					break;

				/*
				 * parent may have changed curDir, but it doesn't change our .
				 */
				resetCurDir();

				check();
				if(Bflush(&bout) < 0)
					writeErr();
				qunlock(&imaplock);
				sleep(15*1000);
				enableForwarding();
			}
_exits("rob3");
			_exits(0);
		}
		idlepid = pid;
	}

	qunlock(&imaplock);

	/*
	 * clear out the next line, which is supposed to contain (case-insensitive)
	 * done\n
	 * this is special code since it has to dance with the idle polling proc
	 * and handle exiting correctly.
	 */
	for(;;){
		c = getc();
		if(c < 0){
			qlock(&imaplock);
			if(!exiting)
				cleaner();
_exits("rob4");
			_exits(0);
		}
		if(c == '\n')
			break;
	}

	qlock(&imaplock);
	if(exiting)
{_exits("rob5");
		_exits(0);
}

	/*
	 * child may have changed curDir, but it doesn't change our .
	 */
	resetCurDir();

	check();
	Bprint(&bout, "%s OK %s terminated\r\n", tg, cmd);
}

static void
listCmd(char *tg, char *cmd)
{
	char *s, *t, *ss, *ref, *mbox;
	int n;

	mustBe(' ');
	s = astring();
	mustBe(' ');
	t = listmbox();
	crnl();
	check();
	ref = mutf7str(s);
	mbox = mutf7str(t);
	if(ref == nil || mbox == nil){
		Bprint(&bout, "%s BAD %s mailbox name not in modified utf-7\r\n", tg, cmd);
		return;
	}

	/*
	 * special request for hierarchy delimiter and root name
	 * root name appears to be name up to and including any delimiter,
	 * or the empty string, if there is no delimiter.
	 *
	 * this must change if the # namespace convention is supported.
	 */
	if(*mbox == '\0'){
		s = strchr(ref, '/');
		if(s == nil)
			ref = "";
		else
			s[1] = '\0';
		Bprint(&bout, "* %s (\\Noselect) \"/\" \"%s\"\r\n", cmd, ref);
		Bprint(&bout, "%s OK %s\r\n", tg, cmd);
		return;
	}


	/*
	 * massage the listing name:
	 * clean up the components individually,
	 * then rip off componenets from the ref to
	 * take care of leading ..'s in the mbox.
	 *
	 * the cleanup can wipe out * followed by a ..
	 * tough luck if such a stupid pattern is given.
	 */
	cleanname(mbox);
	if(strcmp(mbox, ".") == 0)
		*mbox = '\0';
	if(mbox[0] == '/')
		*ref = '\0';
	else if(*ref != '\0'){
		cleanname(ref);
		if(strcmp(ref, ".") == 0)
			*ref = '\0';
	}else
		*ref = '\0';
	while(*ref && isdotdot(mbox)){
		s = strrchr(ref, '/');
		if(s == nil)
			s = ref;
		if(isdotdot(s))
			break;
		*s = '\0';
		mbox += 2;
		if(*mbox == '/')
			mbox++;
	}
	if(*ref == '\0'){
		s = mbox;
		ss = s;
	}else{
		n = strlen(ref) + strlen(mbox) + 2;
		t = binalloc(&parseBin, n, 0);
		if(t == nil)
			parseErr("out of memory");
		snprint(t, n, "%s/%s", ref, mbox);
		s = t;
		ss = s + strlen(ref);
	}

	/*
	 * only allow activity in /mail/box
	 */
	if(s[0] == '/' || isdotdot(s)){
		Bprint(&bout, "%s NO illegal mailbox pattern\r\n", tg);
		return;
	}

	if(cistrcmp(cmd, "lsub") == 0)
		lsubBoxes(cmd, s, ss);
	else
		listBoxes(cmd, s, ss);
	Bprint(&bout, "%s OK %s completed\r\n", tg, cmd);
}

static char*
passCR(char*u, char*p)
{
	static char Ebadch[] = "can't get challenge";
	static char nchall[64];
	static char response[64];
	static Chalstate *ch = nil;
	AuthInfo *ai;

again:
	if (ch == nil){
		if(!(ch = auth_challenge("proto=p9cr role=server user=%q", u)))
			return Ebadch;
		snprint(nchall, 64, " encrypt challenge: %s", ch->chal);
		return nchall;
	} else {
		strncpy(response, p, 64);
		ch->resp = response;
		ch->nresp = strlen(response);
		ai = auth_response(ch);
		auth_freechal(ch);
		ch = nil;
		if (ai == nil)
			goto again;
		setupuser(ai);
		return nil;
	}
		
}

static void
loginCmd(char *tg, char *cmd)
{
	char *s, *t;
	AuthInfo *ai;
	char*r;
	mustBe(' ');
	s = astring();	/* uid */
	mustBe(' ');
	t = astring();	/* password */
	crnl();
	if(allowCR){
		if ((r = passCR(s, t)) == nil){
			Bprint(&bout, "%s OK %s succeeded\r\n", tg, cmd);
			imapState = SAuthed;
		} else {
			Bprint(&bout, "* NO [ALERT] %s\r\n", r);
			Bprint(&bout, "%s NO %s succeeded\r\n", tg, cmd);
		}
		return;
	}
	else if(allowPass){
		if(ai = passLogin(s, t)){
			setupuser(ai);
			Bprint(&bout, "%s OK %s succeeded\r\n", tg, cmd);
			imapState = SAuthed;
		}else
			Bprint(&bout, "%s NO %s failed check\r\n", tg, cmd);
		return;
	}
	Bprint(&bout, "%s NO %s plaintext passwords disallowed\r\n", tg, cmd);
}

/*
 * logout or x-exit, which doesn't expunge the mailbox
 */
static void
logoutCmd(char *tg, char *cmd)
{
	crnl();

	if(cmd[0] != 'x' && selected){
		closeBox(selected, 1);
		selected = nil;
	}
	Bprint(&bout, "* bye\r\n");
	Bprint(&bout, "%s OK %s completed\r\n", tg, cmd);
exits("rob6");
	exits(0);
}

static void
namespaceCmd(char *tg, char *cmd)
{
	crnl();
	check();

	/*
	 * personal, other users, shared namespaces
	 * send back nil or descriptions of (prefix heirarchy-delim) for each case
	 */
	Bprint(&bout, "* NAMESPACE ((\"\" \"/\")) nil nil\r\n");
	Bprint(&bout, "%s OK %s completed\r\n", tg, cmd);
}

static void
noopCmd(char *tg, char *cmd)
{
	crnl();
	check();
	Bprint(&bout, "%s OK %s completed\r\n", tg, cmd);
	enableForwarding();
}

/*
 * this is only a partial implementation
 * should copy files to other directories,
 * and copy & truncate inbox
 */
static void
renameCmd(char *tg, char *cmd)
{
	char *from, *to;
	int ok;

	mustBe(' ');
	from = astring();
	mustBe(' ');
	to = astring();
	crnl();
	check();

	to = mboxName(to);
	if(to == nil || !okMbox(to) || cistrcmp(to, "inbox") == 0){
		Bprint(&bout, "%s NO %s bad mailbox destination name\r\n", tg, cmd);
		return;
	}
	if(access(to, AEXIST) >= 0){
		Bprint(&bout, "%s NO %s mailbox already exists\r\n", tg, cmd);
		return;
	}
	from = mboxName(from);
	if(from == nil || !okMbox(from)){
		Bprint(&bout, "%s NO %s bad mailbox destination name\r\n", tg, cmd);
		return;
	}
	if(cistrcmp(from, "inbox") == 0)
		ok = copyBox(from, to, 0);
	else
		ok = moveBox(from, to);

	if(ok)
		Bprint(&bout, "%s OK %s completed\r\n", tg, cmd);
	else
		Bprint(&bout, "%s NO %s failed\r\n", tg, cmd);
}

static void
searchCmd(char *tg, char *cmd)
{
	searchUCmd(tg, cmd, 0);
}

static void
searchUCmd(char *tg, char *cmd, int uids)
{
	Search rock;
	Msg *m;
	char *uid;
	ulong id;

	mustBe(' ');
	rock.next = nil;
	searchKeys(1, &rock);
	crnl();
	uid = "";
	if(uids)
		uid = "uid ";
	if(rock.next != nil && rock.next->key == SKCharset){
		if(cistrcmp(rock.next->s, "utf-8") != 0
		&& cistrcmp(rock.next->s, "us-ascii") != 0){
			Bprint(&bout, "%s NO [BADCHARSET] (\"US-ASCII\" \"UTF-8\") %s%s failed\r\n",
				tg, uid, cmd);
			checkBox(selected, 0);
			status(uids, uids);
			return;
		}
		rock.next = rock.next->next;
	}
	Bprint(&bout, "* search");
	for(m = selected->msgs; m != nil; m = m->next)
		m->matched = searchMsg(m, rock.next);
	for(m = selected->msgs; m != nil; m = m->next){
		if(m->matched){
			if(uids)
				id = m->uid;
			else
				id = m->seq;
			Bprint(&bout, " %lud", id);
		}
	}
	Bprint(&bout, "\r\n");
	checkBox(selected, 0);
	status(uids, uids);
	Bprint(&bout, "%s OK %s%s completed\r\n", tg, uid, cmd);
}

static void
selectCmd(char *tg, char *cmd)
{
	Msg *m;
	char *s, *mbox;

	mustBe(' ');
	mbox = astring();
	crnl();

	if(selected){
		imapState = SAuthed;
		closeBox(selected, 1);
		selected = nil;
	}

	mbox = mboxName(mbox);
	if(mbox == nil || !okMbox(mbox)){
		Bprint(&bout, "%s NO %s bad mailbox\r\n", tg, cmd);
		return;
	}

	selected = openBox(mbox, "imap", cistrcmp(cmd, "select") == 0);
	if(selected == nil){
		Bprint(&bout, "%s NO %s can't open mailbox %s: %r\r\n", tg, cmd, mbox);
		return;
	}

	imapState = SSelected;

	Bprint(&bout, "* FLAGS (\\Seen \\Answered \\Flagged \\Deleted \\Draft)\r\n");
	Bprint(&bout, "* %lud EXISTS\r\n", selected->max);
	selected->toldMax = selected->max;
	Bprint(&bout, "* %lud RECENT\r\n", selected->recent);
	selected->toldRecent = selected->recent;
	for(m = selected->msgs; m != nil; m = m->next){
		if(!m->expunged && (m->flags & MSeen) != MSeen){
			Bprint(&bout, "* OK [UNSEEN %ld]\r\n", m->seq);
			break;
		}
	}
	Bprint(&bout, "* OK [PERMANENTFLAGS (\\Seen \\Answered \\Flagged \\Draft \\Deleted)]\r\n");
	Bprint(&bout, "* OK [UIDNEXT %ld]\r\n", selected->uidnext);
	Bprint(&bout, "* OK [UIDVALIDITY %ld]\r\n", selected->uidvalidity);
	s = "READ-ONLY";
	if(selected->writable)
		s = "READ-WRITE";
	Bprint(&bout, "%s OK [%s] %s %s completed\r\n", tg, s, cmd, mbox);
}

static NamedInt	statusItems[] =
{
	{"MESSAGES",	SMessages},
	{"RECENT",	SRecent},
	{"UIDNEXT",	SUidNext},
	{"UIDVALIDITY",	SUidValidity},
	{"UNSEEN",	SUnseen},
	{nil,		0}
};

static void
statusCmd(char *tg, char *cmd)
{
	Box *box;
	Msg *m;
	char *s, *mbox;
	ulong v;
	int si, i;

	mustBe(' ');
	mbox = astring();
	mustBe(' ');
	mustBe('(');
	si = 0;
	for(;;){
		s = atom();
		i = mapInt(statusItems, s);
		if(i == 0)
			parseErr("illegal status item");
		si |= i;
		if(peekc() == ')')
			break;
		mustBe(' ');
	}
	mustBe(')');
	crnl();

	mbox = mboxName(mbox);
	if(mbox == nil || !okMbox(mbox)){
		check();
		Bprint(&bout, "%s NO %s bad mailbox\r\n", tg, cmd);
		return;
	}

	box = openBox(mbox, "status", 1);
	if(box == nil){
		check();
		Bprint(&bout, "%s NO [TRYCREATE] %s can't open mailbox %s: %r\r\n", tg, cmd, mbox);
		return;
	}

	Bprint(&bout, "* STATUS %s (", mbox);
	s = "";
	for(i = 0; statusItems[i].name != nil; i++){
		if(si & statusItems[i].v){
			v = 0;
			switch(statusItems[i].v){
			case SMessages:
				v = box->max;
				break;
			case SRecent:
				v = box->recent;
				break;
			case SUidNext:
				v = box->uidnext;
				break;
			case SUidValidity:
				v = box->uidvalidity;
				break;
			case SUnseen:
				v = 0;
				for(m = box->msgs; m != nil; m = m->next)
					if((m->flags & MSeen) != MSeen)
						v++;
				break;
			default:
				Bprint(&bout, ")");
				bye("internal error: status item not implemented");
				break;
			}
			Bprint(&bout, "%s%s %lud", s, statusItems[i].name, v);
			s = " ";
		}
	}
	Bprint(&bout, ")\r\n");
	closeBox(box, 1);

	check();
	Bprint(&bout, "%s OK %s completed\r\n", tg, cmd);
}

static void
storeCmd(char *tg, char *cmd)
{
	storeUCmd(tg, cmd, 0);
}

static void
storeUCmd(char *tg, char *cmd, int uids)
{
	Store *st;
	MsgSet *ms;
	MbLock *ml;
	char *uid;
	ulong max;
	int ok;

	mustBe(' ');
	ms = msgSet(uids);
	mustBe(' ');
	st = storeWhat();
	crnl();
	uid = "";
	if(uids)
		uid = "uid ";
	max = selected->max;
	ml = checkBox(selected, 1);
	ok = ml != nil && forMsgs(selected, ms, max, uids, storeMsg, st);
	closeImp(selected, ml);
	status(uids, uids);
	if(ok)
		Bprint(&bout, "%s OK %s%s completed\r\n", tg, uid, cmd);
	else
		Bprint(&bout, "%s NO %s%s failed\r\n", tg, uid, cmd);
}

/*
 * minimal implementation of subscribe
 * all folders are automatically subscribed,
 * and can't be unsubscribed
 */
static void
subscribeCmd(char *tg, char *cmd)
{
	Box *box;
	char *mbox;
	int ok;

	mustBe(' ');
	mbox = astring();
	crnl();
	check();
	mbox = mboxName(mbox);
	ok = 0;
	if(mbox != nil && okMbox(mbox)){
		box = openBox(mbox, "subscribe", 0);
		if(box != nil){
			ok = subscribe(mbox, 's');
			closeBox(box, 1);
		}
	}
	if(!ok)
		Bprint(&bout, "%s NO %s bad mailbox\r\n", tg, cmd);
	else
		Bprint(&bout, "%s OK %s completed\r\n", tg, cmd);
}

static void
uidCmd(char *tg, char *cmd)
{
	char *sub;

	mustBe(' ');
	sub = atom();
	if(cistrcmp(sub, "copy") == 0)
		copyUCmd(tg, sub, 1);
	else if(cistrcmp(sub, "fetch") == 0)
		fetchUCmd(tg, sub, 1);
	else if(cistrcmp(sub, "search") == 0)
		searchUCmd(tg, sub, 1);
	else if(cistrcmp(sub, "store") == 0)
		storeUCmd(tg, sub, 1);
	else{
		clearcmd();
		Bprint(&bout, "%s BAD %s illegal uid command %s\r\n", tg, cmd, sub);
	}
}

static void
unsubscribeCmd(char *tg, char *cmd)
{
	char *mbox;

	mustBe(' ');
	mbox = astring();
	crnl();
	check();
	mbox = mboxName(mbox);
	if(mbox == nil || !okMbox(mbox) || !subscribe(mbox, 'u'))
		Bprint(&bout, "%s NO %s can't unsubscribe\r\n", tg, cmd);
	else
		Bprint(&bout, "%s OK %s completed\r\n", tg, cmd);
}

static void
badsyn(void)
{
	parseErr("bad syntax");
}

static void
clearcmd(void)
{
	int c;

	for(;;){
		c = getc();
		if(c < 0)
			bye("end of input");
		if(c == '\n')
			return;
	}
}

static void
crnl(void)
{
	int c;

	c = getc();
	if(c == '\n')
		return;
	if(c != '\r' || getc() != '\n')
		badsyn();
}

static void
mustBe(int c)
{
	if(getc() != c){
		ungetc();
		badsyn();
	}
}

/*
 * flaglist	: '(' ')' | '(' flags ')'
 */
static int
flagList(void)
{
	int f;

	mustBe('(');
	f = 0;
	if(peekc() != ')')
		f = flags();

	mustBe(')');
	return f;
}

/*
 * flags	: flag | flags ' ' flag
 * flag		: '\' atom | atom
 */
static int
flags(void)
{
	int ff, flags;
	char *s;
	int c;

	flags = 0;
	for(;;){
		c = peekc();
		if(c == '\\'){
			mustBe('\\');
			s = atomString(atomStop, "\\");
		}else if(strchr(atomStop, c) != nil)
			s = atom();
		else
			break;
		ff = mapFlag(s);
		if(ff == 0)
			parseErr("flag not supported");
		flags |= ff;
		if(peekc() != ' ')
			break;
		mustBe(' ');
	}
	if(flags == 0)
		parseErr("no flags given");
	return flags;
}

/*
 * storeWhat	: osign 'FLAGS' ' ' storeflags
 *		| osign 'FLAGS.SILENT' ' ' storeflags
 * osign	:
 *		| '+' | '-'
 * storeflags	: flagList | flags
 */
static Store*
storeWhat(void)
{
	int f;
	char *s;
	int c, w;

	c = peekc();
	if(c == '+' || c == '-')
		mustBe(c);
	else
		c = 0;
	s = atom();
	w = 0;
	if(cistrcmp(s, "flags") == 0)
		w = STFlags;
	else if(cistrcmp(s, "flags.silent") == 0)
		w = STFlagsSilent;
	else
		parseErr("illegal store attribute");
	mustBe(' ');
	if(peekc() == '(')
		f = flagList();
	else
		f = flags();
	return mkStore(c, w, f);
}

/*
 * fetchWhat	: "ALL" | "FULL" | "FAST" | fetchAtt | '(' fetchAtts ')'
 * fetchAtts	: fetchAtt | fetchAtts ' ' fetchAtt
 */
static char *fetchAtom	= "(){}%*\"\\[]";
static Fetch*
fetchWhat(void)
{
	Fetch *f;
	char *s;

	if(peekc() == '('){
		getc();
		f = nil;
		for(;;){
			s = atomString(fetchAtom, "");
			f = fetchAtt(s, f);
			if(peekc() == ')')
				break;
			mustBe(' ');
		}
		getc();
		return revFetch(f);
	}

	s = atomString(fetchAtom, "");
	if(cistrcmp(s, "all") == 0)
		f = mkFetch(FFlags, mkFetch(FInternalDate, mkFetch(FRfc822Size, mkFetch(FEnvelope, nil))));
	else if(cistrcmp(s, "fast") == 0)
		f = mkFetch(FFlags, mkFetch(FInternalDate, mkFetch(FRfc822Size, nil)));
	else if(cistrcmp(s, "full") == 0)
		f = mkFetch(FFlags, mkFetch(FInternalDate, mkFetch(FRfc822Size, mkFetch(FEnvelope, mkFetch(FBody, nil)))));
	else
		f = fetchAtt(s, nil);
	return f;
}

/*
 * fetchAtt	: "ENVELOPE" | "FLAGS" | "INTERNALDATE"
 *		| "RFC822" | "RFC822.HEADER" | "RFC822.SIZE" | "RFC822.TEXT"
 *		| "BODYSTRUCTURE"
 *		| "UID"
 *		| "BODY"
 *		| "BODY" bodysubs
 *		| "BODY.PEEK" bodysubs
 * bodysubs	: sect
 *		| sect '<' number '.' nz-number '>'
 * sect		: '[' sectSpec ']'
 * sectSpec	: sectMsgText
 *		| sectPart
 *		| sectPart '.' sectText
 * sectPart	: nz-number
 *		| sectPart '.' nz-number
 */
static Fetch*
fetchAtt(char *s, Fetch *f)
{
	NList *sect;
	int c;

	if(cistrcmp(s, "envelope") == 0)
		return mkFetch(FEnvelope, f);
	if(cistrcmp(s, "flags") == 0)
		return mkFetch(FFlags, f);
	if(cistrcmp(s, "internaldate") == 0)
		return mkFetch(FInternalDate, f);
	if(cistrcmp(s, "RFC822") == 0)
		return mkFetch(FRfc822, f);
	if(cistrcmp(s, "RFC822.header") == 0)
		return mkFetch(FRfc822Head, f);
	if(cistrcmp(s, "RFC822.size") == 0)
		return mkFetch(FRfc822Size, f);
	if(cistrcmp(s, "RFC822.text") == 0)
		return mkFetch(FRfc822Text, f);
	if(cistrcmp(s, "bodystructure") == 0)
		return mkFetch(FBodyStruct, f);
	if(cistrcmp(s, "uid") == 0)
		return mkFetch(FUid, f);

	if(cistrcmp(s, "body") == 0){
		if(peekc() != '[')
			return mkFetch(FBody, f);
		f = mkFetch(FBodySect, f);
	}else if(cistrcmp(s, "body.peek") == 0)
		f = mkFetch(FBodyPeek, f);
	else
		parseErr("illegal fetch attribute");

	mustBe('[');
	c = peekc();
	if(c >= '1' && c <= '9'){
		sect = mkNList(number(1), nil);
		while(peekc() == '.'){
			getc();
			c = peekc();
			if(c >= '1' && c <= '9'){
				sect = mkNList(number(1), sect);
			}else{
				break;
			}
		}
		f->sect = revNList(sect);
	}
	if(peekc() != ']')
		sectText(f, f->sect != nil);
	mustBe(']');

	if(peekc() != '<')
		return f;

	f->partial = 1;
	mustBe('<');
	f->start = number(0);
	mustBe('.');
	f->size = number(1);
	mustBe('>');
	return f;
}

/*
 * sectText	: sectMsgText | "MIME"
 * sectMsgText	: "HEADER"
 *		| "TEXT"
 *		| "HEADER.FIELDS" ' ' hdrList
 *		| "HEADER.FIELDS.NOT" ' ' hdrList
 * hdrList	: '(' hdrs ')'
 * hdrs:	: astring
 *		| hdrs ' ' astring
 */
static void
sectText(Fetch *f, int mimeOk)
{
	SList *h;
	char *s;

	s = atomString(fetchAtom, "");
	if(cistrcmp(s, "header") == 0){
		f->part = FPHead;
		return;
	}
	if(cistrcmp(s, "text") == 0){
		f->part = FPText;
		return;
	}
	if(mimeOk && cistrcmp(s, "mime") == 0){
		f->part = FPMime;
		return;
	}
	if(cistrcmp(s, "header.fields") == 0)
		f->part = FPHeadFields;
	else if(cistrcmp(s, "header.fields.not") == 0)
		f->part = FPHeadFieldsNot;
	else
		parseErr("illegal fetch section text");
	mustBe(' ');
	mustBe('(');
	h = nil;
	for(;;){
		h = mkSList(astring(), h);
		if(peekc() == ')')
			break;
		mustBe(' ');
	}
	mustBe(')');
	f->hdrs = revSList(h);
}

/*
 * searchWhat	: "CHARSET" ' ' astring searchkeys | searchkeys
 * searchkeys	: searchkey | searchkeys ' ' searchkey
 * searchkey	: "ALL" | "ANSWERED" | "DELETED" | "FLAGGED" | "NEW" | "OLD" | "RECENT"
 *		| "SEEN" | "UNANSWERED" | "UNDELETED" | "UNFLAGGED" | "DRAFT" | "UNDRAFT"
 *		| astrkey ' ' astring
 *		| datekey ' ' date
 *		| "KEYWORD" ' ' flag | "UNKEYWORD" flag
 *		| "LARGER" ' ' number | "SMALLER" ' ' number
 * 		| "HEADER" astring ' ' astring
 *		| set | "UID" ' ' set
 *		| "NOT" ' ' searchkey
 *		| "OR" ' ' searchkey ' ' searchkey
 *		| '(' searchkeys ')'
 * astrkey	: "BCC" | "BODY" | "CC" | "FROM" | "SUBJECT" | "TEXT" | "TO"
 * datekey	: "BEFORE" | "ON" | "SINCE" | "SENTBEFORE" | "SENTON" | "SENTSINCE"
 */
static NamedInt searchMap[] =
{
	{"ALL",		SKAll},
	{"ANSWERED",	SKAnswered},
	{"DELETED",	SKDeleted},
	{"FLAGGED",	SKFlagged},
	{"NEW",		SKNew},
	{"OLD",		SKOld},
	{"RECENT",	SKRecent},
	{"SEEN",	SKSeen},
	{"UNANSWERED",	SKUnanswered},
	{"UNDELETED",	SKUndeleted},
	{"UNFLAGGED",	SKUnflagged},
	{"DRAFT",	SKDraft},
	{"UNDRAFT",	SKUndraft},
	{"UNSEEN",	SKUnseen},
	{nil,		0}
};

static NamedInt searchMapStr[] =
{
	{"CHARSET",	SKCharset},
	{"BCC",		SKBcc},
	{"BODY",	SKBody},
	{"CC",		SKCc},
	{"FROM",	SKFrom},
	{"SUBJECT",	SKSubject},
	{"TEXT",	SKText},
	{"TO",		SKTo},
	{nil,		0}
};

static NamedInt searchMapDate[] =
{
	{"BEFORE",	SKBefore},
	{"ON",		SKOn},
	{"SINCE",	SKSince},
	{"SENTBEFORE",	SKSentBefore},
	{"SENTON",	SKSentOn},
	{"SENTSINCE",	SKSentSince},
	{nil,		0}
};

static NamedInt searchMapFlag[] =
{
	{"KEYWORD",	SKKeyword},
	{"UNKEYWORD",	SKUnkeyword},
	{nil,		0}
};

static NamedInt searchMapNum[] =
{
	{"SMALLER",	SKSmaller},
	{"LARGER",	SKLarger},
	{nil,		0}
};

static Search*
searchKeys(int first, Search *tail)
{
	Search *s;

	for(;;){
		if(peekc() == '('){
			getc();
			tail = searchKeys(0, tail);
			mustBe(')');
		}else{
			s = searchKey(first);
			tail->next = s;
			tail = s;
		}
		first = 0;
		if(peekc() != ' ')
			break;
		getc();
	}
	return tail;
}

static Search*
searchKey(int first)
{
	Search *sr, rock;
	Tm tm;
	char *a;
	int i, c;

	sr = binalloc(&parseBin, sizeof(Search), 1);
	if(sr == nil)
		parseErr("out of memory");

	c = peekc();
	if(c >= '0' && c <= '9'){
		sr->key = SKSet;
		sr->set = msgSet(0);
		return sr;
	}

	a = atom();
	if(i = mapInt(searchMap, a))
		sr->key = i;
	else if(i = mapInt(searchMapStr, a)){
		if(!first && i == SKCharset)
			parseErr("illegal search key");
		sr->key = i;
		mustBe(' ');
		sr->s = astring();
	}else if(i = mapInt(searchMapDate, a)){
		sr->key = i;
		mustBe(' ');
		c = peekc();
		if(c == '"')
			getc();
		a = atom();
		if(!imap4Date(&tm, a))
			parseErr("bad date format");
		sr->year = tm.year;
		sr->mon = tm.mon;
		sr->mday = tm.mday;
		if(c == '"')
			mustBe('"');
	}else if(i = mapInt(searchMapFlag, a)){
		sr->key = i;
		mustBe(' ');
		c = peekc();
		if(c == '\\'){
			mustBe('\\');
			a = atomString(atomStop, "\\");
		}else
			a = atom();
		i = mapFlag(a);
		if(i == 0)
			parseErr("flag not supported");
		sr->num = i;
	}else if(i = mapInt(searchMapNum, a)){
		sr->key = i;
		mustBe(' ');
		sr->num = number(0);
	}else if(cistrcmp(a, "HEADER") == 0){
		sr->key = SKHeader;
		mustBe(' ');
		sr->hdr = astring();
		mustBe(' ');
		sr->s = astring();
	}else if(cistrcmp(a, "UID") == 0){
		sr->key = SKUid;
		mustBe(' ');
		sr->set = msgSet(0);
	}else if(cistrcmp(a, "NOT") == 0){
		sr->key = SKNot;
		mustBe(' ');
		rock.next = nil;
		searchKeys(0, &rock);
		sr->left = rock.next;
	}else if(cistrcmp(a, "OR") == 0){
		sr->key = SKOr;
		mustBe(' ');
		rock.next = nil;
		searchKeys(0, &rock);
		sr->left = rock.next;
		mustBe(' ');
		rock.next = nil;
		searchKeys(0, &rock);
		sr->right = rock.next;
	}else
		parseErr("illegal search key");
	return sr;
}

/*
 * set	: seqno
 *	| seqno ':' seqno
 *	| set ',' set
 * seqno: nz-number
 *	| '*'
 *
 */
static MsgSet*
msgSet(int uids)
{
	MsgSet head, *last, *ms;
	ulong from, to;

	last = &head;
	head.next = nil;
	for(;;){
		from = uids ? uidNo() : seqNo();
		to = from;
		if(peekc() == ':'){
			getc();
			to = uids ? uidNo() : seqNo();
		}
		ms = binalloc(&parseBin, sizeof(MsgSet), 0);
		if(ms == nil)
			parseErr("out of memory");
		ms->from = from;
		ms->to = to;
		ms->next = nil;
		last->next = ms;
		last = ms;
		if(peekc() != ',')
			break;
		getc();
	}
	return head.next;
}

static ulong
seqNo(void)
{
	if(peekc() == '*'){
		getc();
		return ~0UL;
	}
	return number(1);
}

static ulong
uidNo(void)
{
	if(peekc() == '*'){
		getc();
		return ~0UL;
	}
	return number(0);
}

/*
 * 7 bit, non-ctl chars, no (){%*"\
 * NIL is special case for nstring or parenlist
 */
static char *
atom(void)
{
	return atomString(atomStop, "");
}

/*
 * like an atom, but no +
 */
static char *
tag(void)
{
	return atomString("+(){%*\"\\", "");
}

/*
 * string or atom allowing %*
 */
static char *
listmbox(void)
{
	int c;

	c = peekc();
	if(c == '{')
		return literal();
	if(c == '"')
		return quoted();
	return atomString("(){\"\\", "");
}

/*
 * string or atom
 */
static char *
astring(void)
{
	int c;

	c = peekc();
	if(c == '{')
		return literal();
	if(c == '"')
		return quoted();
	return atom();
}

/*
 * 7 bit, non-ctl chars, none from exception list
 */
static char *
atomString(char *disallowed, char *initial)
{
	char *s;
	int c, ns, as;

	ns = strlen(initial);
	s = binalloc(&parseBin, ns + StrAlloc, 0);
	if(s == nil)
		parseErr("out of memory");
	strcpy(s, initial);
	as = ns + StrAlloc;
	for(;;){
		c = getc();
		if(c <= ' ' || c >= 0x7f || strchr(disallowed, c) != nil){
			ungetc();
			break;
		}
		s[ns++] = c;
		if(ns >= as){
			s = bingrow(&parseBin, s, as, as + StrAlloc, 0);
			if(s == nil)
				parseErr("out of memory");
			as += StrAlloc;
		}
	}
	if(ns == 0)
		badsyn();
	s[ns] = '\0';
	return s;
}

/*
 * quoted: '"' chars* '"'
 * chars:	1-128 except \r and \n
 */
static char *
quoted(void)
{
	char *s;
	int c, ns, as;

	mustBe('"');
	s = binalloc(&parseBin, StrAlloc, 0);
	if(s == nil)
		parseErr("out of memory");
	as = StrAlloc;
	ns = 0;
	for(;;){
		c = getc();
		if(c == '"')
			break;
		if(c < 1 || c > 0x7f || c == '\r' || c == '\n')
			badsyn();
		if(c == '\\'){
			c = getc();
			if(c != '\\' && c != '"')
				badsyn();
		}
		s[ns++] = c;
		if(ns >= as){
			s = bingrow(&parseBin, s, as, as + StrAlloc, 0);
			if(s == nil)
				parseErr("out of memory");
			as += StrAlloc;
		}
	}
	s[ns] = '\0';
	return s;
}

/*
 * litlen: {number}\r\n
 */
static ulong
litlen(void)
{
	ulong v;

	mustBe('{');
	v = number(0);
	mustBe('}');
	crnl();
	return v;
}

/*
 * literal: litlen data<0:litlen>
 */
static char *
literal(void)
{
	char *s;
	ulong v;

	v = litlen();
	s = binalloc(&parseBin, v+1, 0);
	if(s == nil)
		parseErr("out of memory");
	Bprint(&bout, "+ Ready for literal data\r\n");
	if(Bflush(&bout) < 0)
		writeErr();
	if(v != 0 && Bread(&bin, s, v) != v)
		badsyn();
	s[v] = '\0';
	return s;
}

/*
 * digits; number is 32 bits
 */
static ulong
number(int nonzero)
{
	ulong v;
	int c, first;

	v = 0;
	first = 1;
	for(;;){
		c = getc();
		if(c < '0' || c > '9'){
			ungetc();
			if(first)
				badsyn();
			break;
		}
		if(nonzero && first && c == '0')
			badsyn();
		c -= '0';
		first = 0;
		if(v > UlongMax/10 || v == UlongMax/10 && c > UlongMax%10)
			parseErr("number out of range\r\n");
		v = v * 10 + c;
	}
	return v;
}

static int
getc(void)
{
	return Bgetc(&bin);
}

static void
ungetc(void)
{
	Bungetc(&bin);
}

static int
peekc(void)
{
	int c;

	c = Bgetc(&bin);
	Bungetc(&bin);
	return c;
}

