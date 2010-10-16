#include <u.h>
#include <libc.h>
#include <bio.h>
#include <auth.h>
#include "imap4d.h"

static NamedInt	flagChars[NFlags] =
{
	{"s",	MSeen},
	{"a",	MAnswered},
	{"f",	MFlagged},
	{"D",	MDeleted},
	{"d",	MDraft},
	{"r",	MRecent},
};

static	int	fsCtl = -1;

static	void	boxFlags(Box *box);
static	int	createImp(Box *box, Qid *qid);
static	void	fsInit(void);
static	void	mboxGone(Box *box);
static	MbLock	*openImp(Box *box, int new);
static	int	parseImp(Biobuf *b, Box *box);
static	int	readBox(Box *box);
static	ulong	uidRenumber(Msg *m, ulong uid, int force);
static	int	impFlags(Box *box, Msg *m, char *flags);

/*
 * strategy:
 * every mailbox file has an associated .imp file
 * which maps upas/fs message digests to uids & message flags.
 *
 * the .imp files are locked by /mail/fs/usename/L.mbox.
 * whenever the flags can be modified, the lock file
 * should be opened, thereby locking the uid & flag state.
 * for example, whenever new uids are assigned to messages,
 * and whenever flags are changed internally, the lock file
 * should be open and locked.  this means the file must be
 * opened during store command, and when changing the \seen
 * flag for the fetch command.
 *
 * if no .imp file exists, a null one must be created before
 * assigning uids.
 *
 * the .imp file has the following format
 * imp		: "imap internal mailbox description\n"
 * 			uidvalidity " " uidnext "\n"
 *			messageLines
 *
 * messageLines	:
 *		| messageLines digest " " uid " " flags "\n"
 *
 * uid, uidnext, and uidvalidity are 32 bit decimal numbers
 * printed right justified in a field NUid characters long.
 * the 0 uid implies that no uid has been assigned to the message,
 * but the flags are valid. note that message lines are in mailbox
 * order, except possibly for 0 uid messages.
 *
 * digest is an ascii hex string NDigest characters long.
 *
 * flags has a character for each of NFlag flag fields.
 * if the flag is clear, it is represented by a "-".
 * set flags are represented as a unique single ascii character.
 * the currently assigned flags are, in order:
 *	MSeen		s
 *	MAnswered	a
 *	MFlagged	f
 *	MDeleted	D
 *	MDraft		d
 */
Box*
openBox(char *name, char *fsname, int writable)
{
	Box *box;
	MbLock *ml;
	int n, new;

	if(cistrcmp(name, "inbox") == 0)
		if(access("msgs", AEXIST) == 0)
			name = "msgs";
		else
			name = "mbox";
	fsInit();
	debuglog("imap4d open %s %s\n", name, fsname);

	if(fprint(fsCtl, "open '/mail/box/%s/%s' %s", username, name, fsname) < 0){
//ZZZ
		char err[ERRMAX];

		rerrstr(err, sizeof err);
		if(strstr(err, "file does not exist") == nil)
			fprint(2,
		"imap4d at %lud: upas/fs open %s/%s as %s failed: '%s' %s",
			time(nil), username, name, fsname, err,
			ctime(time(nil)));  /* NB: ctime result ends with \n */
		fprint(fsCtl, "close %s", fsname);
		return nil;
	}

	/*
	 * read box to find all messages
	 * each one has a directory, and is in numerical order
	 */
	box = MKZ(Box);
	box->writable = writable;

	n = strlen(name) + 1;
	box->name = emalloc(n);
	strcpy(box->name, name);

	n += STRLEN(".imp");
	box->imp = emalloc(n);
	snprint(box->imp, n, "%s.imp", name);

	n = strlen(fsname) + 1;
	box->fs = emalloc(n);
	strcpy(box->fs, fsname);

	n = STRLEN("/mail/fs/") + strlen(fsname) + 1;
	box->fsDir = emalloc(n);
	snprint(box->fsDir, n, "/mail/fs/%s", fsname);

	box->uidnext = 1;
	new = readBox(box);
	if(new >= 0){
		ml = openImp(box, new);
		if(ml != nil){
			closeImp(box, ml);
			return box;
		}
	}
	closeBox(box, 0);
	return nil;
}

/*
 * check mailbox
 * returns fd of open .imp file if imped.
 * otherwise, return value is insignificant
 *
 * careful: called by idle polling proc
 */
MbLock*
checkBox(Box *box, int imped)
{
	MbLock *ml;
	Dir *d;
	int new;

	if(box == nil)
		return nil;

	/*
	 * if stat fails, mailbox must be gone
	 */
	d = cdDirstat(box->fsDir, ".");
	if(d == nil){
		mboxGone(box);
		return nil;
	}
	new = 0;
	if(box->qid.path != d->qid.path || box->qid.vers != d->qid.vers
	|| box->mtime != d->mtime){
		new = readBox(box);
		if(new < 0){
			free(d);
			return nil;
		}
	}
	free(d);
	ml = openImp(box, new);
	if(ml == nil)
		box->writable = 0;
	else if(!imped){
		closeImp(box, ml);
		ml = nil;
	}
	return ml;
}

/*
 * mailbox is unreachable, so mark all messages expunged
 * clean up .imp files as well.
 */
static void
mboxGone(Box *box)
{
	Msg *m;

	if(cdExists(mboxDir, box->name) < 0)
		cdRemove(mboxDir, box->imp);
	for(m = box->msgs; m != nil; m = m->next)
		m->expunged = 1;
	box->writable = 0;
}

/*
 * read messages in the mailbox
 * mark message that no longer exist as expunged
 * returns -1 for failure, 0 if no new messages, 1 if new messages.
 */
static int
readBox(Box *box)
{
	Msg *msgs, *m, *last;
	Dir *d;
	char *s;
	long max, id;
	int i, nd, fd, new;

	fd = cdOpen(box->fsDir, ".", OREAD);
	if(fd < 0){
		syslog(0, "mail",
		    "imap4d at %lud: upas/fs stat of %s/%s aka %s failed: %r",
			time(nil), username, box->name, box->fsDir);
		mboxGone(box);
		return -1;
	}

	/*
	 * read box to find all messages
	 * each one has a directory, and is in numerical order
	 */
	d = dirfstat(fd);
	if(d == nil){
		close(fd);
		return -1;
	}
	box->mtime = d->mtime;
	box->qid = d->qid;
	last = nil;
	msgs = box->msgs;
	max = 0;
	new = 0;
	free(d);
	while((nd = dirread(fd, &d)) > 0){
		for(i = 0; i < nd; i++){
			s = d[i].name;
			id = strtol(s, &s, 10);
			if(id <= max || *s != '\0'
			|| (d[i].mode & DMDIR) != DMDIR)
				continue;

			max = id;

			while(msgs != nil){
				last = msgs;
				msgs = msgs->next;
				if(last->id == id)
					goto continueDir;
				last->expunged = 1;
			}

			new = 1;
			m = MKZ(Msg);
			m->id = id;
			m->fsDir = box->fsDir;
			m->fs = emalloc(2 * (MsgNameLen + 1));
			m->efs = seprint(m->fs, m->fs + (MsgNameLen + 1), "%lud/", id);
			m->size = ~0UL;
			m->lines = ~0UL;
			m->prev = last;
			m->flags = MRecent;
			if(!msgInfo(m))
				freeMsg(m);
			else{
				if(last == nil)
					box->msgs = m;
				else
					last->next = m;
				last = m;
			}
	continueDir:;
		}
		free(d);
	}
	close(fd);
	for(; msgs != nil; msgs = msgs->next)
		msgs->expunged = 1;

	/*
	 * make up the imap message sequence numbers
	 */
	id = 1;
	for(m = box->msgs; m != nil; m = m->next){
		if(m->seq && m->seq != id)
			bye("internal error assigning message numbers");
		m->seq = id++;
	}
	box->max = id - 1;

	return new;
}

/*
 * read in the .imp file, or make one if it doesn't exist.
 * make sure all flags and uids are consistent.
 * return the mailbox lock.
 */
#define IMPMAGIC	"imap internal mailbox description\n"
static MbLock*
openImp(Box *box, int new)
{
	Qid qid;
	Biobuf b;
	MbLock *ml;
	int fd;
//ZZZZ
	int once;

	ml = mbLock();
	if(ml == nil)
		return nil;
	fd = cdOpen(mboxDir, box->imp, OREAD);
	once = 0;
ZZZhack:
	if(fd < 0 || fqid(fd, &qid) < 0){
		if(fd < 0){
			char buf[ERRMAX];

			errstr(buf, sizeof buf);
			if(cistrstr(buf, "does not exist") == nil)
				fprint(2, "imap4d at %lud: imp open failed: %s\n", time(nil), buf);
			if(!once && cistrstr(buf, "locked") != nil){
				once = 1;
				fprint(2, "imap4d at %lud: imp %s/%s %s locked when it shouldn't be; spinning\n", time(nil), username, box->name, box->imp);
				fd = openLocked(mboxDir, box->imp, OREAD);
				goto ZZZhack;
			}
		}
		if(fd >= 0)
			close(fd);
		fd = createImp(box, &qid);
		if(fd < 0){
			mbUnlock(ml);
			return nil;
		}
		box->dirtyImp = 1;
		if(box->uidvalidity == 0)
			box->uidvalidity = box->mtime;
		box->impQid = qid;
		new = 1;
	}else if(qid.path != box->impQid.path || qid.vers != box->impQid.vers){
		Binit(&b, fd, OREAD);
		if(!parseImp(&b, box)){
			box->dirtyImp = 1;
			if(box->uidvalidity == 0)
				box->uidvalidity = box->mtime;
		}
		Bterm(&b);
		box->impQid = qid;
		new = 1;
	}
	if(new)
		boxFlags(box);
	close(fd);
	return ml;
}

/*
 * close the .imp file, after writing out any changes
 */
void
closeImp(Box *box, MbLock *ml)
{
	Msg *m;
	Qid qid;
	Biobuf b;
	char buf[NFlags+1];
	int fd;

	if(ml == nil)
		return;
	if(!box->dirtyImp){
		mbUnlock(ml);
		return;
	}

	fd = cdCreate(mboxDir, box->imp, OWRITE, 0664);
	if(fd < 0){
		mbUnlock(ml);
		return;
	}
	Binit(&b, fd, OWRITE);

	box->dirtyImp = 0;
	Bprint(&b, "%s", IMPMAGIC);
	Bprint(&b, "%.*lud %.*lud\n", NUid, box->uidvalidity, NUid, box->uidnext);
	for(m = box->msgs; m != nil; m = m->next){
		if(m->expunged)
			continue;
		wrImpFlags(buf, m->flags, strcmp(box->fs, "imap") == 0);
		Bprint(&b, "%.*s %.*lud %s\n", NDigest, m->info[IDigest], NUid, m->uid, buf);
	}
	Bterm(&b);

	if(fqid(fd, &qid) >= 0)
		box->impQid = qid;
	close(fd);
	mbUnlock(ml);
}

void
wrImpFlags(char *buf, int flags, int killRecent)
{
	int i;

	for(i = 0; i < NFlags; i++){
		if((flags & flagChars[i].v)
		&& (flagChars[i].v != MRecent || !killRecent))
			buf[i] = flagChars[i].name[0];
		else
			buf[i] = '-';
	}
	buf[i] = '\0';
}

int
emptyImp(char *mbox)
{
	Dir *d;
	long mode;
	int fd;

	fd = cdCreate(mboxDir, impName(mbox), OWRITE, 0664);
	if(fd < 0)
		return -1;
	d = cdDirstat(mboxDir, mbox);
	if(d == nil){
		close(fd);
		return -1;
	}
	fprint(fd, "%s%.*lud %.*lud\n", IMPMAGIC, NUid, d->mtime, NUid, 1UL);
	mode = d->mode & 0777;
	nulldir(d);
	d->mode = mode;
	dirfwstat(fd, d);
	free(d);
	return fd;
}

/*
 * try to match permissions with mbox
 */
static int
createImp(Box *box, Qid *qid)
{
	Dir *d;
	long mode;
	int fd;

	fd = cdCreate(mboxDir, box->imp, OREAD, 0664);
	if(fd < 0)
		return -1;
	d = cdDirstat(mboxDir, box->name);
	if(d != nil){
		mode = d->mode & 0777;
		nulldir(d);
		d->mode = mode;
		dirfwstat(fd, d);
		free(d);
	}
	if(fqid(fd, qid) < 0){
		close(fd);
		return -1;
	}

	return fd;
}

/*
 * read or re-read a .imp file.
 * this is tricky:
 *	messages can be deleted by another agent
 *	we might still have a Msg for an expunged message,
 *		because we haven't told the client yet.
 *	we can have a Msg without a .imp entry.
 *	flag information is added at the end of the .imp by copy & append
 *	there can be duplicate messages (same digests).
 *
 * look up existing messages based on uid.
 * look up new messages based on in order digest matching.
 *
 * note: in the face of duplicate messages, one of which is deleted,
 * two active servers may decide different ones are valid, and so return
 * different uids for the messages.  this situation will stablize when the servers exit.
 */
static int
parseImp(Biobuf *b, Box *box)
{
	Msg *m, *mm;
	char *s, *t, *toks[3];
	ulong uid, u;
	int match, n;

	m = box->msgs;
	s = Brdline(b, '\n');
	if(s == nil || Blinelen(b) != STRLEN(IMPMAGIC)
	|| strncmp(s, IMPMAGIC, STRLEN(IMPMAGIC)) != 0)
		return 0;

	s = Brdline(b, '\n');
	if(s == nil || Blinelen(b) != 2*NUid + 2)
		return 0;
	s[2*NUid + 1] = '\0';
	u = strtoul(s, &t, 10);
	if(u != box->uidvalidity && box->uidvalidity != 0)
		return 0;
	box->uidvalidity = u;
	if(*t != ' ' || t != s + NUid)
		return 0;
	t++;
	u = strtoul(t, &t, 10);
	if(box->uidnext > u)
		return 0;
	box->uidnext = u;
	if(t != s + 2*NUid+1 || box->uidnext == 0)
		return 0;

	uid = ~0;
	while(m != nil){
		s = Brdline(b, '\n');
		if(s == nil)
			break;
		n = Blinelen(b) - 1;
		if(n != NDigest + NUid + NFlags + 2
		|| s[NDigest] != ' ' || s[NDigest + NUid + 1] != ' ')
			return 0;
		toks[0] = s;
		s[NDigest] = '\0';
		toks[1] = s + NDigest + 1;
		s[NDigest + NUid + 1] = '\0';
		toks[2] = s + NDigest + NUid + 2;
		s[n] = '\0';
		t = toks[1];
		u = strtoul(t, &t, 10);
		if(*t != '\0' || uid != ~0 && (uid >= u && u || u && !uid))
			return 0;
		uid = u;

		/*
		 * zero uid => added by append or copy, only flags valid
		 * can only match messages without uids, but this message
		 * may not be the next one, and may have been deleted.
		 */
		if(!uid){
			for(; m != nil && m->uid; m = m->next)
				;
			for(mm = m; mm != nil; mm = mm->next){
				if(mm->info[IDigest] != nil &&
				    strcmp(mm->info[IDigest], toks[0]) == 0){
					if(!mm->uid)
						mm->flags = 0;
					if(!impFlags(box, mm, toks[2]))
						return 0;
					m = mm->next;
					break;
				}
			}
			continue;
		}

		/*
		 * ignore expunged messages,
		 * and messages already assigned uids which don't match this uid.
		 * such messages must have been deleted by another imap server,
		 * which updated the mailbox and .imp file since we read the mailbox,
		 * or because upas/fs got confused by consecutive duplicate messages,
		 * the first of which was deleted by another imap server.
		 */
		for(; m != nil && (m->expunged || m->uid && m->uid < uid); m = m->next)
			;
		if(m == nil)
			break;

		/*
		 * only check for digest match on the next message,
		 * since it comes before all other messages, and therefore
		 * must be in the .imp file if they should be.
		 */
		match = m->info[IDigest] != nil &&
			strcmp(m->info[IDigest], toks[0]) == 0;
		if(uid && (m->uid == uid || !m->uid && match)){
			if(!match)
				bye("inconsistent uid");

			/*
			 * wipe out recent flag if some other server saw this new message.
			 * it will be read from the .imp file if is really should be set,
			 * ie the message was only seen by a status command.
			 */
			if(!m->uid)
				m->flags = 0;

			if(!impFlags(box, m, toks[2]))
				return 0;
			m->uid = uid;
			m = m->next;
		}
	}
	return 1;
}

/*
 * parse .imp flags
 */
static int
impFlags(Box *box, Msg *m, char *flags)
{
	int i, f;

	f = 0;
	for(i = 0; i < NFlags; i++){
		if(flags[i] == '-')
			continue;
		if(flags[i] != flagChars[i].name[0])
			return 0;
		f |= flagChars[i].v;
	}

	/*
	 * recent flags are set until the first time message's box is selected or examined.
	 * it may be stored in the file as a side effect of a status or subscribe command;
	 * if so, clear it out.
	 */
	if((f & MRecent) && strcmp(box->fs, "imap") == 0)
		box->dirtyImp = 1;
	f |= m->flags & MRecent;

	/*
	 * all old messages with changed flags should be reported to the client
	 */
	if(m->uid && m->flags != f){
		box->sendFlags = 1;
		m->sendFlags = 1;
	}
	m->flags = f;
	return 1;
}

/*
 * assign uids to any new messages
 * which aren't already in the .imp file.
 * sum up totals for flag values.
 */
static void
boxFlags(Box *box)
{
	Msg *m;

	box->recent = 0;
	for(m = box->msgs; m != nil; m = m->next){
		if(m->uid == 0){
			box->dirtyImp = 1;
			box->uidnext = uidRenumber(m, box->uidnext, 0);
		}
		if(m->flags & MRecent)
			box->recent++;
	}
}

static ulong
uidRenumber(Msg *m, ulong uid, int force)
{
	for(; m != nil; m = m->next){
		if(!force && m->uid != 0)
			bye("uid renumbering with a valid uid");
		m->uid = uid++;
	}
	return uid;
}

void
closeBox(Box *box, int opened)
{
	Msg *m, *next;

	/*
	 * make sure to leave the mailbox directory so upas/fs can close the mailbox
	 */
	myChdir(mboxDir);

	if(box->writable){
		deleteMsgs(box);
		if(expungeMsgs(box, 0))
			closeImp(box, checkBox(box, 1));
	}

	if(fprint(fsCtl, "close %s", box->fs) < 0 && opened)
		bye("can't talk to mail server");
	for(m = box->msgs; m != nil; m = next){
		next = m->next;
		freeMsg(m);
	}
	free(box->name);
	free(box->fs);
	free(box->fsDir);
	free(box->imp);
	free(box);
}

int
deleteMsgs(Box *box)
{
	Msg *m;
	char buf[BufSize], *p, *start;
	int ok;

	if(!box->writable)
		return 0;

	/*
	 * first pass: delete messages; gang the writes together for speed.
	 */
	ok = 1;
	start = seprint(buf, buf + sizeof(buf), "delete %s", box->fs);
	p = start;
	for(m = box->msgs; m != nil; m = m->next){
		if((m->flags & MDeleted) && !m->expunged){
			m->expunged = 1;
			p = seprint(p, buf + sizeof(buf), " %lud", m->id);
			if(p + 32 >= buf + sizeof(buf)){
				if(write(fsCtl, buf, p - buf) < 0)
					bye("can't talk to mail server");
				p = start;
			}
		}
	}
	if(p != start && write(fsCtl, buf, p - buf) < 0)
		bye("can't talk to mail server");

	return ok;
}

/*
 * second pass: remove the message structure,
 * and renumber message sequence numbers.
 * update messages counts in mailbox.
 * returns true if anything changed.
 */
int
expungeMsgs(Box *box, int send)
{
	Msg *m, *next, *last;
	ulong n;

	n = 0;
	last = nil;
	for(m = box->msgs; m != nil; m = next){
		m->seq -= n;
		next = m->next;
		if(m->expunged){
			if(send)
				Bprint(&bout, "* %lud expunge\r\n", m->seq);
			if(m->flags & MRecent)
				box->recent--;
			n++;
			if(last == nil)
				box->msgs = next;
			else
				last->next = next;
			freeMsg(m);
		}else
			last = m;
	}
	if(n){
		box->max -= n;
		box->dirtyImp = 1;
	}
	return n;
}

static void
fsInit(void)
{
	if(fsCtl >= 0)
		return;
	fsCtl = open("/mail/fs/ctl", ORDWR);
	if(fsCtl < 0)
		bye("can't open mail file system");
	if(fprint(fsCtl, "close mbox") < 0)
		bye("can't initialize mail file system");
}

static char *stoplist[] =
{
	"mbox",
	"pipeto",
	"forward",
	"names",
	"pipefrom",
	"headers",
	"imap.ok",
	0
};

enum {
	Maxokbytes	= 4096,
	Maxfolders	= Maxokbytes / 4,
};

static char *folders[Maxfolders];
static char *folderbuff;

static void
readokfolders(void)
{
	int fd, nr;

	fd = open("imap.ok", OREAD);
	if(fd < 0)
		return;
	folderbuff = malloc(Maxokbytes);
	if(folderbuff == nil) {
		close(fd);
		return;
	}
	nr = read(fd, folderbuff, Maxokbytes-1);	/* once is ok */
	close(fd);
	if(nr < 0){
		free(folderbuff);
		folderbuff = nil;
		return;
	}
	folderbuff[nr] = 0;
	tokenize(folderbuff, folders, nelem(folders));
}

/*
 * reject bad mailboxes based on mailbox name
 */
int
okMbox(char *path)
{
	char *name;
	int i;

	if(folderbuff == nil && access("imap.ok", AREAD) == 0)
		readokfolders();
	name = strrchr(path, '/');
	if(name == nil)
		name = path;
	else
		name++;
	if(folderbuff != nil){
		for(i = 0; i < nelem(folders) && folders[i] != nil; i++)
			if(cistrcmp(folders[i], name) == 0)
				return 1;
		return 0;
	}
	if(strlen(name) + STRLEN(".imp") >= MboxNameLen)
		return 0;
	for(i = 0; stoplist[i]; i++)
		if(strcmp(name, stoplist[i]) == 0)
			return 0;
	if(isprefix("L.", name) || isprefix("imap-tmp.", name)
	|| issuffix(".imp", name)
	|| strcmp("imap.subscribed", name) == 0
	|| isdotdot(name) || name[0] == '/')
		return 0;
	return 1;
}
