#include <u.h>
#include <libc.h>
#include <httpd.h>
#include <httpsrv.h>

#define MAXSZ	512

char	*name, *company, *contact, *choice;
HConnect *hc;

void
reg_ack(Hio *hout)
{
	hprint(hout, "<h2><tt>Registration Accepted %s</tt></h2>\n", ctime(hc->reqtime));
	hprint(hout, "<hr color=blue>\n%s<br>\n%s<br>\n%s<br>\n", name, company, contact);
	hprint(hout, "<hr color=blue>\n<p>\n<tt>Thank you for registering.\n");
	hprint(hout, "<br>You hereby have our permission to use the Spin software for commercial purposes.\n");
	hprint(hout, "<br>Let us know how you do! (spin_list@research.bell-labs.com)\n");
	hprint(hout, "<br><a href=\"http://cm.bell-labs.com/cm/cs/what/spin/Man/README.html\">\n");
	hprint(hout, "<font color=red>\n<p>\n");
	hprint(hout, "Click Here for the standard Spin Download Instructions.</a>\n");
}

char *
data_ok(void)
{
	HSPriv *p;
	char *s, *e;
	int fd;

	fd = open("/usr/gerard/spin_licenses", OWRITE);
	if(fd < 0)
		return "No Logfile";

	s = hc->xferbuf;
	e = &hc->xferbuf[sizeof hc->xferbuf];
	p = hc->private;
	s = seprint(s, e, "\ndate:	%s", ctime(hc->reqtime));
	s = seprint(s, e, "host:	%s\n", p->remotesys);
	if (name)	s = seprint(s, e, "name:	%s\n", name);
	if (company)	s = seprint(s, e, "company:	%s\n", company);
	if (contact)	s = seprint(s, e, "contact:	%s\n", contact);
	if (choice)	s = seprint(s, e, "choice:	%s\n", choice);
	if(write(fd, hc->xferbuf, s - hc->xferbuf) != s - hc->xferbuf){
		close(fd);
		return "Server logfile inaccessible";
	}
	close(fd);

	if (choice && strstr(choice, "Do Not Accept"))
		return "Not Accept Button Pressed";

	if (name && strlen(name) > 2 && strchr(name, ' ')
		 &&  company && strlen(company) > 2
		 &&  contact && strlen(contact) > 2 && strchr(contact, '@'))
		return "ok";
	return "Missing Data";
}

void
reg_nak(Hio *hout, char *s)
{
	hprint(hout, "<h2><tt>Sorry Registration Rejected: %s</tt></h2>\n", s);
	hprint(hout, "<hr color=blue>\n");
	if (name)	hprint(hout, "Name:	%s<br>\n", name);
	if (company)	hprint(hout, "Company:	%s<br>\n", company);
	if (contact)	hprint(hout, "Email:	%s<br>\n", contact);
	hprint(hout, "<hr color=blue>\n");
	hprint(hout, "<tt>Contact spin_list@research.bell-labs.com for more information.\n");
}

void
main(int argc, char **argv)
{
	HSPairs *q;
	Hio *hin, *hout;
	char *s, *t;

	hc = init(argc, argv);

	if(hparseheaders(hc, HSTIMEOUT) < 0)
		exits("failed");
	hout = &hc->hout;
	if(hc->head.expectother){
		hfail(hc, HExpectFail, nil);
		exits("failed");
	}
	if(hc->head.expectcont){
		hprint(hout, "100 Continue\r\n");
		hprint(hout, "\r\n");
		hflush(hout);
	}

	s = nil;
	if(strcmp(hc->req.meth, "POST") == 0){
		hin = hbodypush(&hc->hin, hc->head.contlen, hc->head.transenc);
		if(hin != nil){
			alarm(HSTIMEOUT);
			s = hreadbuf(hin, hin->pos);
			alarm(0);
		}
		if(s == nil){
			hfail(hc, HBadReq, nil);
			exits("failed");
		}
		t = strchr(s, '\n');
		if(t != nil)
			*t = '\0';
	}else if(strcmp(hc->req.meth, "GET") != 0 && strcmp(hc->req.meth, "HEAD") != 0){
		hunallowed(hc, "GET, HEAD, PUT");
		exits("unallowed");
	}else
		s = hc->req.search;
	if(s == nil){
		hfail(hc, HNoData, "save");
		exits("failed");
	}

	s = hurlunesc(hc, s);
	if(strlen(s) > MAXSZ)
		s[MAXSZ] = '\0';
	for(q = hparsequery(hc, s); q; q = q->next){
		if(strcmp(q->s, "YourName") == 0)
			name = q->t;
		else if(strcmp(q->s, "YourCompany") == 0)
			company = q->t;
		else if(strcmp(q->s, "YourContact") == 0)
			contact = q->t;
		else if(strcmp(q->s, "Button") == 0)
			choice = q->t;
	}

	if(hc->req.vermaj){
		hokheaders(hc);
		hprint(hout, "Content-type: text/html\r\n");
		hprint(hout, "\r\n");
	}

	hprint(hout, "<html>\n");
	hprint(hout, "<body bgcolor=\"#ffffff\" text=\"#000000\" ");
	hprint(hout, "link=\"#0033cc\" vlink=\"#330088\" alink=\"#ffffff\">\n");
	hprint(hout, "<p>\n");
	hprint(hout, "<title>Spin Registration Information</title>\n");

	s = data_ok();

	if (strcmp(s, "ok") == 0)
		reg_ack(hout);
	else
		reg_nak(hout, s);

	hprint(hout, "<hr color=blue>\n");
	hprint(hout, "</font></tt>\n");
	hprint(hout, "</html>\n");
	hflush(hout);

	exits(nil);
}
