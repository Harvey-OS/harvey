#include "common.h"
#include "smtpd.h"
#include "smtp.h"
#include <ctype.h>
#include <ip.h>

/*
 * There's a bit of a problem with yahoo; they apparently have a vast
 * pool of machines that all run the same queue(s), so a 451 retry can
 * come from a different IP address for many, many retries, and it can
 * take ~5 hours for the same IP to call us back.  Various other goofballs,
 * notably the IEEE, try to send mail just before 9 AM, then refuse to try
 * again until after 5 PM.  Doh!
 */
enum {
	Nonspammax = 10*60*60,  /* must call back within this time if real */
};
static char whitelist[] = "/mail/lib/whitelist";

/*
 * matches ip addresses or subnets in whitelist against nci->rsys.
 * ignores comments and blank lines in /mail/lib/whitelist.
 */
static int
onwhitelist(void)
{
	int lnlen;
	char *line, *parse;
	char input[128];
	uchar ip[IPaddrlen], ipmasked[IPaddrlen];
	uchar mask4[IPaddrlen], addr4[IPaddrlen];
	uchar mask[IPaddrlen], addr[IPaddrlen], addrmasked[IPaddrlen];
	Biobuf *wl;
	static int beenhere;
	static allzero[IPaddrlen];

	if (!beenhere) {
		beenhere = 1;
		fmtinstall('I', eipfmt);
	}

	parseip(ip, nci->rsys);
	wl = Bopen(whitelist, OREAD);
	if (wl == nil)
		return 1;
	while ((line = Brdline(wl, '\n')) != nil) {
		if (line[0] == '#' || line[0] == '\n')
			continue;
		lnlen = Blinelen(wl);
		line[lnlen-1] = '\0';		/* clobber newline */

		/* default mask is /32 (v4) or /128 (v6) for bare IP */
		parse = line;
		if (strchr(line, '/') == nil) {
			strncpy(input, line, sizeof input - 5);
			if (strchr(line, '.') != nil)
				strcat(input, "/32");
			else
				strcat(input, "/128");
			parse = input;
		}
		/* sorry, dave; where's parsecidr for v4 or v6? */
		v4parsecidr(addr4, mask4, parse);
		v4tov6(addr, addr4);
		v4tov6(mask, mask4);

		maskip(addr, mask, addrmasked);
		maskip(ip, mask, ipmasked);
		if (memcmp(ipmasked, addrmasked, IPaddrlen) == 0)
			break;
	}
	Bterm(wl);
	return line != nil;
}

static int
tryaddgrey(char *file)
{
	int fd = create(file, OWRITE|OEXCL, 0444);

	if (fd >= 0) {
		close(fd);
		return 1;
	}
	return 0;
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

/* true if we added it and it wasn't already present */
static int
addgreylist(char *file)
{
	if (tryaddgrey(file))
		return 1;

	/* make presumed-missing intermediate directories */
	if (mkpdirs(file) < 0)
		return 0;

	/* retry the greylist entry */
	if (tryaddgrey(file))
		return 1;

	/* it's in the list now, we're a 2nd conn., so add to whitelist */
	return 0;
}

static int
recentcall(long mtime)
{
	return time(0) - mtime <= Nonspammax;
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
	int wasongrey;
	long calltm = 0;
	char *user;
	char file[256];
	Dir *ds;

	if (rcpt[0] == '\0' || strchr(rcpt, '/') != nil ||
	    strcmp(rcpt, ".") == 0 || strcmp(rcpt, "..") == 0)
		return 0;

	/* shorten names to fit pre-fossil or pre-9p2000 file servers */
	user = strrchr(rcpt, '!');
	if (user == nil)
		user = rcpt;
	else
		user++;

	snprint(file, sizeof file, "/mail/grey/%s/%s/%s",
		nci->lsys, nci->rsys, user);
	ds = dirstat(file);
	if (ds != nil)
		wasongrey = 1;
	else {
		wasongrey = !addgreylist(file);
		ds = dirstat(file);
	}

	if (ds != nil) {
		calltm = ds->mtime;
		free(ds);
	}

	/* if on greylist already and prior call was recent, add to whitelist */
	if (wasongrey && recentcall(calltm)) {
		syslog(0, "smtpd",
			"%s/%s was grey; adding IP to white", nci->rsys, rcpt);
		return 1;
	} else if (wasongrey)
		syslog(0, "smtpd", "call for %s/%s was too long ago",
			nci->rsys, rcpt);
	else
		syslog(0, "smtpd", "no registered call for %s/%s",
			nci->rsys, rcpt);
	return 0;
}

void
vfysenderhostok(void)
{
	int recent = 0;
	Link *l;

	if (onwhitelist())
		return;

	for (l = rcvers.first; l; l = l->next)
		if (isrcptrecent(s_to_c(l->p)))
			recent = 1;

	/* if on greylist already and prior call was recent, add to whitelist */
	if (recent) {
		int fd = create(whitelist, OWRITE, 0444|DMAPPEND);

		if (fd >= 0) {
			seek(fd, 0, 2);			/* paranoia */
			fprint(fd, "%s\n", nci->rsys);
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
