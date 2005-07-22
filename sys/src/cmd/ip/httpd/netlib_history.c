#include <u.h>
#include <libc.h>
#include <bio.h>
#include "httpd.h"
#include "httpsrv.h"

Hio *HO;
int diffb;

enum{ DAY = 24*60*60 };

void
lastbefore(ulong t, char *f, char *b)
{
	Tm *tm;
	Dir *dir;
	int try;
	ulong t0, mtime;

	t0 = t;
	for(try=0; try<10; try++) {
		tm = localtime(t);
		t -= DAY;
		sprint(b,"%.4d/%.2d%.2d/netlib/pub/%s",tm->year+1900,tm->mon+1,tm->mday,f);
		dir = dirstat(b);
		if(dir == nil)
			continue;
		mtime = dir->mtime;
		free(dir);
		if(mtime > t0)
			continue;
		return;
	}
	strcpy(b, "filenotfound");
}

// create explicit file for diff, which otherwise would create a
// mode 0600 file that it couldn't read (because running as none)
void
gunzip(char *f, char *tmp)
{
	int fd = open(tmp, OWRITE);

	if(fd < 0)  // can't happen
		return;
	switch(fork()){
	case 0:
		dup(fd, 1);
		close(fd);
		close(0);
		execl("/bin/gunzip", "gunzip", "-c", f, nil);
		hprint(HO, "can't exec gunzip: %r\n");
		break;
	case -1:
		hprint(HO, "fork failed: %r\n");
	default:
		while(waitpid() != -1)
			;
		break;
	}
	close(fd);
}

void
netlibhistory(char *file)
{
	char buf[500], pair[2][500], tmpf[2][30], *f;
	int toggle = 0, started = 0, limit;
	Dir *dir;
	ulong otime, dt;
	int i, fd, tmpcnt;

	if(strncmp(file, "../", 3) == 0 || strstr(file, "/../") ||
		strlen(file) >= sizeof(buf) - strlen("1997/0204/netlib/pub/0"))
		return;
	limit = 50;
	if(diffb){
		limit = 10;
		// create two tmp files for gunzip
		for(i = 0, tmpcnt = 0; i < 2 && tmpcnt < 20; tmpcnt++){
			snprint(tmpf[i], sizeof(tmpf[0]), "/tmp/d%x", tmpcnt);
			if(access(buf, AEXIST) == 0)
				continue;
			fd = create(tmpf[i], OWRITE, 0666);
			if(fd < 0)
				goto done;
			close(fd);
			i++;
		}
	}
	otime = time(0);
	hprint(HO,"<UL>\n");
	while(limit--){
		lastbefore(otime, file, buf);
		dir = dirstat(buf);
		if(dir == nil)
			goto done;
		dt = DAY/2;
		while(otime <= dir->mtime){
			lastbefore(otime-dt, file, buf);
			free(dir);
			dir = dirstat(buf);
			if(dir == nil)
				goto done;
			dt += DAY/2;
		}
		f = pair[toggle];
		strcpy(f, buf);
		if(diffb && strcmp(f+strlen(f)-3, ".gz") == 0){
			gunzip(f, tmpf[toggle]);
			strcpy(f, tmpf[toggle]);
		}
		if(diffb && started){
			hprint(HO, "<PRE>\n");
			hflush(HO);
			switch(fork()){
			case 0:
				execl("/bin/diff", "diff", "-nb",
					pair[1-toggle], pair[toggle], nil);
				hprint(HO, "can't exec diff: %r\n");
				break;
			case -1:
				hprint(HO, "fork failed: %r\n");
				break;
			default:
				while(waitpid() != -1)
					;
				break;
			}
			hprint(HO, "</PRE>\n");
		}
		hprint(HO,"<LI><A HREF=\"/historic/%s\">%s</A> %lld bytes\n",
			buf, 4+asctime(gmtime(dir->mtime)), dir->length);
		if(diffb)
			hprint(HO," <FONT SIZE=-1>(%s)</FONT>\n", pair[toggle]);
		toggle = 1-toggle;
		started = 1;
		otime = dir->mtime;
		free(dir);
	}
	hprint(HO,"<LI>...\n");
done:
	hprint(HO,"</UL>\n");
	if(diffb){
		remove(tmpf[0]);
		remove(tmpf[1]);
	}
}

int
send(HConnect *c)
{
	char *file, *s;
	HSPairs *q;

	if(strcmp(c->req.meth, "GET") != 0 && strcmp(c->req.meth, "HEAD") != 0)
		return hunallowed(c, "GET, HEAD");
	if(c->head.expectother || c->head.expectcont)
		return hfail(c, HExpectFail, nil);
	if(c->req.search == nil || !*c->req.search)
		return hfail(c, HNoData, "netlib_history");
	s = c->req.search;
	while((s = strchr(s, '+')) != nil)
		*s++ = ' ';
	file = nil;
	for(q = hparsequery(c, hstrdup(c, c->req.search)); q; q = q->next){
		if(strcmp(q->s, "file") == 0)
			file = q->t;
		else if(strcmp(q->s, "diff") == 0)
			diffb = 1;
	}
	if(file == nil)
		return hfail(c, HNoData, "netlib_history missing file field");
	logit(c, "netlib_hist %s%s", file, diffb?" DIFF":"");

	if(c->req.vermaj){
		hokheaders(c);
		hprint(HO, "Content-type: text/html\r\n");
		hprint(HO, "\r\n");
	}
	if(strcmp(c->req.meth, "HEAD") == 0){
		writelog(c, "Reply: 200 netlib_history 0\n");
		hflush(HO);
		exits(nil);
	}

	hprint(HO, "<HEAD><TITLE>%s history</TITLE></HEAD>\n<BODY>\n",file);
	hprint(HO, "<H2>%s history</H2>\n",file);
	hprint(HO, "<I>Netlib's copy of %s was changed\n", file);
	hprint(HO, "on the dates shown.  <BR>Click on the date link\n");
	hprint(HO, "to retrieve the corresponding version.</I>\n");
	if(diffb){
		hprint(HO, "<BR><I>Lines beginning with &lt; are for the\n");
		hprint(HO, "newer of the two versions.</I>\n");
	}

	if(chdir("/usr/web/historic") < 0)
		hprint(HO, "chdir failed: %r\n");
	netlibhistory(file);

	hprint(HO, "<BR><A HREF=\"http://cm.bell-labs.com/who/ehg\">Eric Grosse</A>\n");
	hprint(HO, "</BODY></HTML>\n");
	hflush(HO);
	writelog(c, "Reply: 200 netlib_history %ld %ld\n", HO->seek, HO->seek);
	return 1;
}

void
main(int argc, char **argv)
{
	HConnect *c;

	c = init(argc, argv);
	HO = &c->hout;
	if(hparseheaders(c, HSTIMEOUT) >= 0)
		send(c);
	exits(nil);
}
