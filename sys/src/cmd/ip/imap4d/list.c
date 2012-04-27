#include <u.h>
#include <libc.h>
#include <bio.h>
#include <auth.h>
#include "imap4d.h"

#define SUBSCRIBED	"imap.subscribed"

static int	matches(char *ref, char *pat, char *name);
static int	mayMatch(char *pat, char *name, int star);
static int	checkMatch(char *cmd, char *ref, char *pat, char *mbox, long mtime, int isdir);
static int	listAll(char *cmd, char *ref, char *pat, char *mbox, long mtime);
static int	listMatch(char *cmd, char *ref, char *pat, char *mbox, char *mm);
static int	mkSubscribed(void);

static long
listMtime(char *file)
{
	Dir *d;
	long mtime;

	d = cdDirstat(mboxDir, file);
	if(d == nil)
		return 0;
	mtime = d->mtime;
	free(d);
	return mtime;
}

/*
 * check for subscribed mailboxes
 * each line is either a comment starting with #
 * or is a subscribed mailbox name
 */
int
lsubBoxes(char *cmd, char *ref, char *pat)
{
	MbLock *mb;
	Dir *d;
	Biobuf bin;
	char *s;
	long mtime;
	int fd, ok, isdir;

	mb = mbLock();
	if(mb == nil)
		return 0;
	fd = cdOpen(mboxDir, SUBSCRIBED, OREAD);
	if(fd < 0)
		fd = mkSubscribed();
	if(fd < 0){
		mbUnlock(mb);
		return 0;
	}
	ok = 0;
	Binit(&bin, fd, OREAD);
	while(s = Brdline(&bin, '\n')){
		s[Blinelen(&bin) - 1] = '\0';
		if(s[0] == '#')
			continue;
		isdir = 1;
		if(cistrcmp(s, "INBOX") == 0){
			if(access("msgs", AEXIST) == 0)
				mtime = listMtime("msgs");
			else
				mtime = listMtime("mbox");
			isdir = 0;
		}else{
			d = cdDirstat(mboxDir, s);
			if(d != nil){
				mtime = d->mtime;
				if(!(d->mode & DMDIR))
					isdir = 0;
				free(d);
			}else
				mtime = 0;
		}
		ok |= checkMatch(cmd, ref, pat, s, mtime, isdir);
	}
	Bterm(&bin);
	close(fd);
	mbUnlock(mb);
	return ok;
}

static int
mkSubscribed(void)
{
	int fd;

	fd = cdCreate(mboxDir, SUBSCRIBED, ORDWR, 0664);
	if(fd < 0)
		return -1;
	fprint(fd, "#imap4 subscription list\nINBOX\n");
	seek(fd, 0, 0);
	return fd;
}

/*
 * either subscribe or unsubscribe to a mailbox
 */
int
subscribe(char *mbox, int how)
{
	MbLock *mb;
	char *s, *in, *ein;
	int fd, tfd, ok, nmbox;

	if(cistrcmp(mbox, "inbox") == 0)
		mbox = "INBOX";
	mb = mbLock();
	if(mb == nil)
		return 0;
	fd = cdOpen(mboxDir, SUBSCRIBED, ORDWR);
	if(fd < 0)
		fd = mkSubscribed();
	if(fd < 0){
		mbUnlock(mb);
		return 0;
	}
	in = readFile(fd);
	if(in == nil){
		mbUnlock(mb);
		return 0;
	}
	nmbox = strlen(mbox);
	s = strstr(in, mbox);
	while(s != nil && (s != in && s[-1] != '\n' || s[nmbox] != '\n'))
		s = strstr(s+1, mbox);
	ok = 0;
	if(how == 's' && s == nil){
		if(fprint(fd, "%s\n", mbox) > 0)
			ok = 1;
	}else if(how == 'u' && s != nil){
		ein = strchr(s, '\0');
		memmove(s, &s[nmbox+1], ein - &s[nmbox+1]);
		ein -= nmbox+1;
		tfd = cdOpen(mboxDir, SUBSCRIBED, OWRITE|OTRUNC);
		if(tfd >= 0 && seek(fd, 0, 0) >= 0 && write(fd, in, ein-in) == ein-in)
			ok = 1;
		if(tfd > 0)
			close(tfd);
	}else
		ok = 1;
	close(fd);
	mbUnlock(mb);
	return ok;
}

/*
 * stupidly complicated so that % doesn't read entire directory structure
 * yet * works
 * note: in most places, inbox is case-insensitive,
 * but here INBOX is checked for a case-sensitve match.
 */
int
listBoxes(char *cmd, char *ref, char *pat)
{
	int ok;

	ok = checkMatch(cmd, ref, pat, "INBOX", listMtime("mbox"), 0);
	return ok | listMatch(cmd, ref, pat, ref, pat);
}

/*
 * look for all messages which may match the pattern
 * punt when a * is reached
 */
static int
listMatch(char *cmd, char *ref, char *pat, char *mbox, char *mm)
{
	Dir *dir, *dirs;
	char *mdir, *m, *mb, *wc;
	long mode;
	int c, i, nmb, nmdir, nd, ok, fd;

	mdir = nil;
	for(m = mm; c = *m; m++){
		if(c == '%' || c == '*'){
			if(mdir == nil){
				fd = cdOpen(mboxDir, ".", OREAD);
				if(fd < 0)
					return 0;
				mbox = "";
				nmdir = 0;
			}else{
				*mdir = '\0';
				fd = cdOpen(mboxDir, mbox, OREAD);
				*mdir = '/';
				nmdir = mdir - mbox + 1;
				if(fd < 0)
					return 0;
				dir = dirfstat(fd);
				if(dir == nil){
					close(fd);
					return 0;
				}
				mode = dir->mode;
				free(dir);
				if(!(mode & DMDIR))
					break;
			}
			wc = m;
			for(; c = *m; m++)
				if(c == '/')
					break;
			nmb = nmdir + strlen(m) + MboxNameLen + 3;
			mb = emalloc(nmb);
			strncpy(mb, mbox, nmdir);
			ok = 0;
			while((nd = dirread(fd, &dirs)) > 0){
				for(i = 0; i < nd; i++){
					if(strcmp(mbox, "") == 0 &&
					    !okMbox(dirs[i].name))
						continue;
					/* Safety: ignore message dirs */
					if(strstr(dirs[i].name, "mails") != 0 ||
					   strcmp(dirs[i].name, "out") == 0 ||
					   strcmp(dirs[i].name, "obox") == 0 ||
					   strcmp(dirs[i].name, "ombox") == 0)
						continue;
					if(strcmp(dirs[i].name, "msgs") == 0)
						dirs[i].mode &= ~DMDIR;
					if(*wc == '*' && dirs[i].mode & DMDIR &&
					    mayMatch(mm, dirs[i].name, 1)){
						snprint(mb+nmdir, nmb-nmdir,
							"%s", dirs[i].name);
						ok |= listAll(cmd, ref, pat, mb,
							dirs[i].mtime);
					}else if(mayMatch(mm, dirs[i].name, 0)){
						snprint(mb+nmdir, nmb-nmdir,
							"%s%s", dirs[i].name, m);
						if(*m == '\0')
							ok |= checkMatch(cmd,
								ref, pat, mb,
								dirs[i].mtime,
								dirs[i].mode &
								DMDIR);
						else if(dirs[i].mode & DMDIR)
							ok |= listMatch(cmd,
								ref, pat, mb, mb
								+ nmdir + strlen(
								dirs[i].name));
					}
				}
				free(dirs);
			}
			close(fd);
			free(mb);
			return ok;
		}
		if(c == '/'){
			mdir = m;
			mm = m + 1;
		}
	}
	m = mbox;
	if(*mbox == '\0')
		m = ".";
	dir = cdDirstat(mboxDir, m);
	if(dir == nil)
		return 0;
	ok = checkMatch(cmd, ref, pat, mbox, dir->mtime, (dir->mode & DMDIR) == DMDIR);
	free(dir);
	return ok;
}

/*
 * too hard: recursively list all files rooted at mbox,
 * and list checkMatch figure it out
 */
static int
listAll(char *cmd, char *ref, char *pat, char *mbox, long mtime)
{
	Dir *dirs;
	char *mb;
	int i, nmb, nd, ok, fd;

	ok = checkMatch(cmd, ref, pat, mbox, mtime, 1);
	fd = cdOpen(mboxDir, mbox, OREAD);
	if(fd < 0)
		return ok;

	nmb = strlen(mbox) + MboxNameLen + 2;
	mb = emalloc(nmb);
	while((nd = dirread(fd, &dirs)) > 0){
		for(i = 0; i < nd; i++){
			snprint(mb, nmb, "%s/%s", mbox, dirs[i].name);
			/* safety: do not recurr */
			if(0 && dirs[i].mode & DMDIR)
				ok |= listAll(cmd, ref, pat, mb, dirs[i].mtime);
			else
				ok |= checkMatch(cmd, ref, pat, mb, dirs[i].mtime, 0);
		}
		free(dirs);
	}
	close(fd);
	free(mb);
	return ok;
}

static int
mayMatch(char *pat, char *name, int star)
{
	Rune r;
	int i, n;

	for(; *pat && *pat != '/'; pat += n){
		r = *(uchar*)pat;
		if(r < Runeself)
			n = 1;
		else
			n = chartorune(&r, pat);

		if(r == '*' || r == '%'){
			pat += n;
			if(r == '*' && star || *pat == '\0' || *pat == '/')
				return 1;
			while(*name){
				if(mayMatch(pat, name, star))
					return 1;
				name += chartorune(&r, name);
			}
			return 0;
		}
		for(i = 0; i < n; i++)
			if(name[i] != pat[i])
				return 0;
		name += n;
	}
	if(*name == '\0')
		return 1;
	return 0;
}

/*
 * mbox is a mailbox name which might match pat.
 * verify the match
 * generates response
 */
static int
checkMatch(char *cmd, char *ref, char *pat, char *mbox, long mtime, int isdir)
{
	char *s, *flags;

	if(!matches(ref, pat, mbox) || !okMbox(mbox))
		return 0;
	if(strcmp(mbox, ".") == 0)
		mbox = "";

	if(isdir)
		flags = "(\\Noselect)";
	else{
		s = impName(mbox);
		if(s != nil && listMtime(s) < mtime)
			flags = "(\\Noinferiors \\Marked)";
		else
			flags = "(\\Noinferiors)";
	}

	s = strmutf7(mbox);
	if(s != nil)
		Bprint(&bout, "* %s %s \"/\" \"%s\"\r\n", cmd, flags, s);
	return 1;
}

static int
matches(char *ref, char *pat, char *name)
{
	Rune r;
	int i, n;

	while(ref != pat)
		if(*name++ != *ref++)
			return 0;
	for(; *pat; pat += n){
		r = *(uchar*)pat;
		if(r < Runeself)
			n = 1;
		else
			n = chartorune(&r, pat);

		if(r == '*'){
			pat += n;
			if(*pat == '\0')
				return 1;
			while(*name){
				if(matches(pat, pat, name))
					return 1;
				name += chartorune(&r, name);
			}
			return 0;
		}
		if(r == '%'){
			pat += n;
			while(*name && *name != '/'){
				if(matches(pat, pat, name))
					return 1;
				name += chartorune(&r, name);
			}
			pat -= n;
			continue;
		}
		for(i = 0; i < n; i++)
			if(name[i] != pat[i])
				return 0;
		name += n;
	}
	if(*name == '\0')
		return 1;
	return 0;
}
