#include "mothra.h"

ulong	modtime;

void	httpheader(Url*, char*);
int	httpresponse(char*);

static	ulong	cracktime(char*);

/*
 * Given a url, return a file descriptor on which caller can
 * read an http document.  As a side effect, we parse the
 * http header and fill in some fields in the url.
 * The caller is responsible for processing redirection loops.
 * Method can be either GET or POST.  If method==post, body
 * is the text to be posted.
 */
int
http(Url *url, int method, char *body)
{
	char *addr, *com;
	int fd, n, nnl, len;
	int pfd[2];
	char buf[1024], *bp, *ebp;
	char line[1024+1], *lp, *elp;
	char authstr[128], *urlname;
	int gotresponse;
	int response;
	int port;
	static char *proxyserver;
	static int getproxy = 1;

	if(getproxy) {
		proxyserver = getenv("httpproxy");
		getproxy = 0;
	}
	*authstr = 0;

backforauth:
	if(proxyserver && proxyserver[0] != '\0'){
		addr = strchr(proxyserver, ':');
		if(addr) {
			*addr++ = 0;
			port = atoi(addr);
		} else
			port = 80;
		addr = proxyserver;
		urlname = url->fullname;
	} else {
		addr = url->ipaddr;
		port = url->port;
		urlname = url->reltext;
	}

	fd = tcpconnect("tcp", addr, port, 0);
	if(fd == -1)
		goto ErrReturn;
	com = emalloc(strlen(urlname)+200);
	switch(method) {
	case GET:
		n = sprint(com,
			"GET %s HTTP/1.0\r\n%s"
			"Host: %s\r\n"
			"Accept: */*\r\n"
			"User-agent: mothra/%s\r\n"
			"\r\n",
				urlname, authstr, url->ipaddr, version);
		if(debug)
			fprint(2, "-> %s", com);
		if(write(fd, com, n) != n) {
			free(com);
			goto fdErrReturn;
		}
		break;

	case POST:
		len = strlen(body);
		n = sprint(com,
			"POST %s HTTP/1.0\r\n%s"
			"Content-type: application/x-www-form-urlencoded\r\n"
			"Content-length: %d\r\n"
			"User-agent: mothra/%s\r\n"
			"\r\n",
				urlname, authstr, len, version);
		if(debug)
			fprint(2, "-> %s", com);
		if(write(fd, com, n) != n ||
		   write(fd, body, len) != len) {
			free(com);
			goto fdErrReturn;
		}
		break;
	}

	free(com);
	if(pipe(pfd) == -1)
		goto fdErrReturn;
	n = readn(fd, buf, sizeof(buf));

	if(n <= 0) {
	EarlyEof:
		if(n == 0) {
			fprint(2, "%s: EOF in header\n", url->fullname);
			werrstr("EOF in header");
		}

	pfdErrReturn:
		close(pfd[0]);
		close(pfd[1]);

	fdErrReturn:
		close(fd);

	ErrReturn:
		return -1;
	}

	bp = buf;
	ebp = buf+n;
	url->type = 0;
	if(strncmp(buf, "HTTP/", 5) == 0) {	/* hack test for presence of header */
		SET(response);
		gotresponse = 0;
		url->redirname[0] = '\0';
		nnl = 0;
		lp = line;
		elp = line+1024;
		while(nnl != 2) {
			if(bp==ebp) {
				n = read(fd, buf, 1024);
				if(n <= 0)
					goto EarlyEof;
				ebp = buf+n;
				bp = buf;
			}
			if(*bp != '\r'){
				if(nnl==1 &&
				  (!gotresponse || (*bp!=' ' && *bp!='\t'))) {
					*lp = '\0';
					if(gotresponse)
						httpheader(url, line);
					else {
						response = httpresponse(line);
						gotresponse = 1;
					}
					lp = line;
				}
				if(*bp == '\n')
					nnl++;
				else {
					nnl = 0;
					if(lp != elp)
						*lp++ = *bp;
				}
			}
			bp++;
		}

		if(gotresponse)
		switch(response) {
		case 200:	/* OK */
		case 201:	/* Created */
		case 202:	/* Accepted */
			break;

		case 204:	/* No Content */
			werrstr("URL has no content");
			goto pfdErrReturn;

		case 301:	/* Moved Permanently */
		case 302:	/* Moved Temporarily */
			if(url->redirname[0]){
				url->type = FORWARD;
				werrstr("URL forwarded");
				goto pfdErrReturn;
			}
			break;

		case 304:	/* Not Modified */
			break;

		case 400:	/* Bad Request */
			werrstr("Bad Request to server");
			goto pfdErrReturn;

		case 401:	/* Unauthorized */
		case 402:	/* ??? */
			if(*authstr == 0){
				close(pfd[0]);
				close(pfd[1]);
				close(fd);
				if(auth(url, authstr, sizeof(authstr)) == 0)
					goto backforauth;
				goto ErrReturn;
			}
			break;

		case 403:	/* Forbidden */
			werrstr("Forbidden by server");
			goto pfdErrReturn;

		case 404:	/* Not Found */
			werrstr("Not found on server");
			goto pfdErrReturn;

		case 500:	/* Internal server error */
			werrstr("Server choked");
			goto pfdErrReturn;

		case 501:	/* Not implemented */
			werrstr("Server can't do it!");
			goto pfdErrReturn;

		case 502:	/* Bad gateway */
			werrstr("Bad gateway");
			goto pfdErrReturn;

		case 503:	/* Service unavailable */
			werrstr("Service unavailable");
			goto pfdErrReturn;
		}
	}
	if(url->type == 0)
		url->type = suffix2type(url->fullname);
	switch(fork()) {
	case -1:
		werrstr("Can't fork");
		goto pfdErrReturn;

	case 0:
		close(pfd[0]);
		if(bp != ebp)
			write(pfd[1], bp, ebp-bp);
		while((n = read(fd, buf, 1024)) > 0)
			write(pfd[1], buf, n);
		errstr(buf);
		if(n < 0 && strcmp(buf, "hung up") != 0)
			fprint(2, "%s: %r\n", url->fullname);
		_exits(0);

	default:
		close(pfd[1]);
		close(fd);
		return pfd[0];
	}
}

/*
 * Process a header line for this url
 */
void
httpheader(Url *url, char *line)
{
	char *name, *arg, *s, *arg2;

	name = line;
	if(debug)
		fprint(2, "<- %s\n", line);
	for(s=line; *s!=':'; s++)
		if(*s=='\0')
			return;
	*s++ = '\0';
	while(*s==' ' || *s=='\t')
		s++;
	arg = s;
	while(*s!=' ' && *s!='\t' && *s!=';' && *s!='\0')
		s++;
	if(*s == ' ' || *s == '\t')
		arg2 = s+1;
	else
		arg2 = s;
	*s = '\0';

	if(cistrcmp(name, "Content-Type") == 0)
		url->type |= content2type(arg);
	else
	if(cistrcmp(name, "Content-Encoding") == 0)
		url->type |= encoding2type(arg);
	else
	if(cistrcmp(name, "Last-Modified") == 0)
		modtime = cracktime(arg2);
	else
	if(cistrcmp(name, "WWW-authenticate") == 0){
		strncpy(url->authtype, arg, sizeof(url->authtype));
		strncpy(url->autharg, arg2, sizeof(url->autharg));
	} else
	if(cistrcmp(name, "URI") == 0){
		if(*arg != '<')
			return;
		arg++;
		for(s=arg; *s!='>'; s++)
			if(*s == '\0')
				return;
		*s = '\0';
		strncpy(url->redirname, arg, sizeof(url->redirname));
	} else
	if(cistrcmp(name, "Location") == 0)
		strncpy(url->redirname, arg, sizeof(url->redirname));

}

int
httpresponse(char *line)
{
	if(debug)
		fprint(2, "<- %s\n", line);
	while(*line!=' ' && *line!='\t' && *line!='\0')
		line++;
	return atoi(line);
}

/*
 *  decode the time fields, return seconds since epoch began
 */
char*	monthchars = "janfebmaraprmayjunjulaugsepoctnovdec";
static	Tm	now;

static ulong
cracktime(char *str)
{
	char *month, *day, *yr, *hms;
	char *fields[5];
	Tm tm;
	int i;
	char *p;

	i = getfields(str, fields, 5, 1, " \t");
	if(i < 4)
		return time(0);
	day = fields[0];
	month = fields[1];
	yr = fields[2];
	hms = fields[3];

	/* default time */
	if(now.year == 0)
		now = *p9gmtime(time(0));
	tm = now;

	/* convert ascii month to a number twixt 1 and 12 */
	if(*month >= '0' && *month <= '9'){
		tm.mon = atoi(month) - 1;
		if(tm.mon < 0 || tm.mon > 11)
			tm.mon = 5;
	} else {
		for(p = month; *p; p++)
			*p = tolower(*p);
		for(i = 0; i < 12; i++)
			if(strncmp(&monthchars[i*3], month, 3) == 0){
				tm.mon = i;
				break;
			}
	}

	tm.mday = atoi(day);

	if(hms) {
		tm.hour = strtoul(hms, &p, 10);
		if(*p == ':') {
			p++;
			tm.min = strtoul(p, &p, 10);
			if(*p == ':') {
				p++;
				tm.sec = strtoul(p, &p, 10);
			}
		}
		if(tolower(*p) == 'p')
			tm.hour += 12;
	}

	if(yr) {
		tm.year = atoi(yr);
		if(tm.year >= 1900)
			tm.year -= 1900;
	} else {
		if(tm.mon > now.mon || (tm.mon == now.mon && tm.mday > now.mday+1))
			tm.year--;
	}

	strcpy(tm.zone, "GMT");
	/* convert to epoch seconds */
	return tm2sec(&tm);
}
