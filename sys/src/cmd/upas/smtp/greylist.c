/*
 * greylisting is the practice of making unknown callers call twice, with
 * a pause between them, before accepting their mail and adding them to a
 * whitelist of known callers.
 *
 * There's a bit of a problem with yahoo and other large sources of mail;
 * they have a vast pool of machines that all run the same queue(s), so a
 * 451 retry can come from a different IP address for many, many retries,
 * and it can take ~5 hours for the same IP to call us back.  To cope
 * better with this, we immediately accept mail from any system on the
 * same class C subnet (IPv4 /24) as anybody on our whitelist, since the
 * mail-sending machines tend to be clustered within a class C subnet.
 *
 * Various other goofballs, notably the IEEE, try to send mail just
 * before 9 AM, then refuse to try again until after 5 PM. D'oh!
 */
#include "common.h"
#include "smtpd.h"
#include "smtp.h"
#include <ctype.h>
#include <ip.h>
#include <ndb.h>

enum {
	Nonspammax = 14*60*60,  /* must call back within this time if real */
	Nonspammin = 5*60,	/* must wait this long to retry */
};

typedef struct {
	int	existed;	/* these two are distinct to cope with errors */
	int	created;
	int	noperm;
	long	mtime;		/* mod time, iff it already existed */
} Greysts;

static char whitelist[] = "/mail/grey/whitelist";

/*
 * matches ip addresses or subnets in whitelist against nci->rsys.
 * ignores comments and blank lines in /mail/grey/whitelist.
 */
static int
onwhitelist(void)
{
	int lnlen;
	char *line, *parse, *p;
	char input[128];
	uchar *mask;
	uchar mask4[IPaddrlen], addr4[IPaddrlen];
	uchar rmask[IPaddrlen], addr[IPaddrlen];
	uchar ipmasked[IPaddrlen], addrmasked[IPaddrlen];
	Biobuf *wl;

	wl = Bopen(whitelist, OREAD);
	if (wl == nil)
		return 1;
	while ((line = Brdline(wl, '\n')) != nil) {
		lnlen = Blinelen(wl);
		line[lnlen-1] = '\0';		/* clobber newline */

		p = strpbrk(line, " \t");
		if (p)
			*p = 0;
		if (line[0] == '#' || line[0] == 0)
			continue;

		/* default mask is /24 (v4) or /128 (v6) for bare IP */
		parse = line;
		if (strchr(line, '/') == nil) {
			strecpy(input, input + sizeof input - 5, line);
			if (strchr(line, ':') != nil)	/* v6? */
				strcat(input, "/128");
			else if (strchr(line, '.') != nil)
				strcat(input, "/24");	/* was /32 */
			parse = input;
		}
		mask = rmask;
		if (strchr(line, ':') != nil) {		/* v6? */
			parseip(addr, parse);
			p = strchr(parse, '/');
			if (p != nil)
				parseipmask(mask, p);
			else
				mask = IPallbits;
		} else {
			v4parsecidr(addr4, mask4, parse);
			v4tov6(addr, addr4);
			v4tov6(mask, mask4);
		}
		maskip(addr, mask, addrmasked);
		maskip(rsysip, mask, ipmasked);
		if (equivip6(ipmasked, addrmasked))
			break;
	}
	Bterm(wl);
	return line != nil;
}

static int mkdirs(char *);

/*
 * if any directories leading up to path don't exist, create them.
 * modifies but restores path.
 */
static int
mkpdirs(char *path)
{
	int rv = 0;
	char *sl = strrchr(path, '/');

	if (sl != nil) {
		*sl = '\0';
		rv = mkdirs(path);
		*sl = '/';
	}
	return rv;
}

/*
 * if path or any directories leading up to it don't exist, create them.
 * modifies but restores path.
 */
static int
mkdirs(char *path)
{
	int fd;

	if (access(path, AEXIST) >= 0)
		return 0;

	/* make presumed-missing intermediate directories */
	if (mkpdirs(path) < 0)
		return -1;

	/* make final directory */
	fd = create(path, OREAD, 0777|DMDIR);
	if (fd < 0)
		/*
		 * we may have lost a race; if the directory now exists,
		 * it's okay.
		 */
		return access(path, AEXIST) < 0? -1: 0;
	close(fd);
	return 0;
}

static long
getmtime(char *file)
{
	int fd;
	long mtime = -1;
	Dir *ds;

	fd = open(file, ORDWR);
	if (fd < 0)
		return mtime;
	ds = dirfstat(fd);
	if (ds != nil) {
		mtime = ds->mtime;
		/*
		 * new twist: update file's mtime after reading it,
		 * so each call resets the future time after which
		 * we'll accept calls.  thus spammers who keep pounding
		 * us lose, but just pausing for a few minutes and retrying
		 * will succeed.
		 */
		if (0) {
			/*
			 * apparently none can't do this wstat
			 * (permission denied);
			 * more undocumented whacky none behaviour.
			 */
			ds->mtime = time(0);
			if (dirfwstat(fd, ds) < 0)
				syslog(0, "smtpd", "dirfwstat %s: %r", file);
		}
		free(ds);
		write(fd, "x", 1);
	}
	close(fd);
	return mtime;
}

static void
tryaddgrey(char *file, Greysts *gsp)
{
	int fd = create(file, OWRITE|OEXCL, 0666);

	gsp->created = (fd >= 0);
	if (fd >= 0) {
		close(fd);
		gsp->existed = 0;  /* just created; couldn't have existed */
		gsp->mtime = time(0);
	} else {
		/*
		 * why couldn't we create file? it must have existed
		 * (or we were denied perm on parent dir.).
		 * if it existed, fill in gsp->mtime; otherwise
		 * make presumed-missing intermediate directories.
		 */
		gsp->existed = access(file, AEXIST) >= 0;
		if (gsp->existed)
			gsp->mtime = getmtime(file);
		else if (mkpdirs(file) < 0)
			gsp->noperm = 1;
	}
}

static void
addgreylist(char *file, Greysts *gsp)
{
	tryaddgrey(file, gsp);
	if (!gsp->created && !gsp->existed && !gsp->noperm)
		/* retry the greylist entry with parent dirs created */
		tryaddgrey(file, gsp);
}

static int
recentcall(Greysts *gsp)
{
	long delay = time(0) - gsp->mtime;

	if (!gsp->existed)
		return 0;
	/* reject immediate call-back; spammers are doing that now */
	return delay >= Nonspammin && delay <= Nonspammax;
}

/*
 * policy: if (caller-IP, my-IP, rcpt) is not on the greylist,
 * reject this message as "451 temporary failure".  if the caller is real,
 * he'll retry soon, otherwise he's a spammer.
 * at the first rejection, create a greylist entry for (my-ip, caller-ip,
 * rcpt, time), where time is the file's mtime.  if they call back and there's
 * already a greylist entry, and it's within the allowed interval,
 * add their IP to the append-only whitelist.
 *
 * greylist files can be removed at will; at worst they'll cause a few
 * extra retries.
 */

static int
isrcptrecent(char *rcpt)
{
	char *user;
	char file[256];
	Greysts gs;
	Greysts *gsp = &gs;

	if (rcpt[0] == '\0' || strchr(rcpt, '/') != nil ||
	    strcmp(rcpt, ".") == 0 || strcmp(rcpt, "..") == 0)
		return 0;

	/* shorten names to fit pre-fossil or pre-9p2000 file servers */
	user = strrchr(rcpt, '!');
	if (user == nil)
		user = rcpt;
	else
		user++;

	/* check & try to update the grey list entry */
	snprint(file, sizeof file, "/mail/grey/tmp/%s/%s/%s",
		nci->lsys, nci->rsys, user);
	memset(gsp, 0, sizeof *gsp);
	addgreylist(file, gsp);

	/* if on greylist already and prior call was recent, add to whitelist */
	if (gsp->existed && recentcall(gsp)) {
		syslog(0, "smtpd",
			"%s/%s was grey; adding IP to white", nci->rsys, rcpt);
		return 1;
	} else if (gsp->existed)
		syslog(0, "smtpd", "call for %s/%s was just minutes ago "
			"or long ago", nci->rsys, rcpt);
	else
		syslog(0, "smtpd", "no call registered for %s/%s; registering",
			nci->rsys, rcpt);
	return 0;
}

void
vfysenderhostok(void)
{
	char *fqdn;
	int recent = 0;
	Link *l;

	if (onwhitelist())
		return;

	for (l = rcvers.first; l; l = l->next)
		if (isrcptrecent(s_to_c(l->p)))
			recent = 1;

	/* if on greylist already and prior call was recent, add to whitelist */
	if (recent) {
		int fd = create(whitelist, OWRITE, 0666|DMAPPEND);

		if (fd >= 0) {
			seek(fd, 0, 2);			/* paranoia */
			fqdn = csgetvalue(nci->root, "ip", nci->rsys, "dom",
				nil);
			if (fqdn != nil)
				fprint(fd, "%s %s\n", nci->rsys, fqdn);
			else
				fprint(fd, "%s\n", nci->rsys);
			free(fqdn);
			close(fd);
		}
	} else {
		syslog(0, "smtpd",
	"no recent call from %s for a rcpt; rejecting with temporary failure",
			nci->rsys);
		reply("451 please try again soon from the same IP.\r\n");
		exits("no recent call for a rcpt");
	}
}
