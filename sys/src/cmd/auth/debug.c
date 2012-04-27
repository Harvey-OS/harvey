/*
 * Test various aspects of the authentication setup.
 */

#include <u.h>
#include <libc.h>
#include <bio.h>
#include <ndb.h>
#include <auth.h>
#include <authsrv.h>

/* private copy with added debugging */
int
authdial(char *netroot, char *dom)
{
	char *p;
	int rv;
	
	if(dom != nil){
		/* look up an auth server in an authentication domain */
		p = csgetvalue(netroot, "authdom", dom, "auth", nil);

		/* if that didn't work, just try the IP domain */
		if(p == nil)
			p = csgetvalue(netroot, "dom", dom, "auth", nil);
		if(p == nil){
			werrstr("no auth server found for %s", dom);
			return -1;
		}
		print("\tdialing auth server %s\n",
			netmkaddr(p, netroot, "ticket"));
		rv = dial(netmkaddr(p, netroot, "ticket"), 0, 0, 0);
		free(p);
		return rv;
	} else
		/* look for one relative to my machine */
		return dial(netmkaddr("$auth", netroot, "ticket"), 0, 0, 0);
}

void
usage(void)
{
	fprint(2, "usage: auth/debug\n");
	exits("usage");
}

static char*
readcons(char *prompt, char *def, int raw, char *buf, int nbuf)
{
	int fdin, fdout, ctl, n, m;
	char line[10];

	fdin = open("/dev/cons", OREAD);
	if(fdin < 0)
		fdin = 0;
	fdout = open("/dev/cons", OWRITE);
	if(fdout < 0)
		fdout = 1;
	if(def != nil)
		fprint(fdout, "%s[%s]: ", prompt, def);
	else
		fprint(fdout, "%s: ", prompt);
	if(raw){
		ctl = open("/dev/consctl", OWRITE);
		if(ctl >= 0)
			write(ctl, "rawon", 5);
	} else
		ctl = -1;

	m = 0;
	for(;;){
		n = read(fdin, line, 1);
		if(n == 0){
			close(ctl);
			werrstr("readcons: EOF");
			return nil;
		}
		if(n < 0){
			close(ctl);
			werrstr("can't read cons");
			return nil;
		}
		if(line[0] == 0x7f)
			exits(0);
		if(n == 0 || line[0] == '\n' || line[0] == '\r'){
			if(raw){
				write(ctl, "rawoff", 6);
				write(fdout, "\n", 1);
				close(ctl);
			}
			buf[m] = '\0';
			if(buf[0]=='\0' && def)
				strcpy(buf, def);
			return buf;
		}
		if(line[0] == '\b'){
			if(m > 0)
				m--;
		}else if(line[0] == 0x15){	/* ^U: line kill */
			m = 0;
			if(def != nil)
				fprint(fdout, "%s[%s]: ", prompt, def);
			else
				fprint(fdout, "%s: ", prompt);
		}else{
			if(m >= nbuf-1){
				fprint(fdout, "line too long\n");
				m = 0;
				if(def != nil)
					fprint(fdout, "%s[%s]: ", prompt, def);
				else
					fprint(fdout, "%s: ", prompt);
			}else
				buf[m++] = line[0];
		}
	}
}

void authdialfutz(char*, char*);
void authfutz(char*, char*);

/* scan factotum for p9sk1 keys; check them */
void
debugfactotumkeys(void)
{
	char *s, *dom, *proto, *user;
	int found;
	Attr *a;
	Biobuf *b;

	b = Bopen("/mnt/factotum/ctl", OREAD);
	if(b == nil){
		fprint(2, "debug: cannot open /mnt/factotum/ctl\n");
		return;
	}
	found = 0;
	while((s = Brdstr(b, '\n', 1)) != nil){
		if(strncmp(s, "key ", 4) != 0){
			print("malformed ctl line: %s\n", s);
			free(s);
			continue;
		}
		a = _parseattr(s+4);
		free(s);
		proto = _strfindattr(a, "proto");
		if(proto==nil || strcmp(proto, "p9sk1")!=0)
			continue;
		dom = _strfindattr(a, "dom");
		if(dom == nil){
			print("p9sk1 key with no dom: %A\n", a);
			_freeattr(a);
			continue;
		}
		user = _strfindattr(a, "user");
		if(user == nil){
			print("p9sk1 key with no user: %A\n", a);
			_freeattr(a);
			continue;
		}
		print("p9sk1 key: %A\n", a);
		found = 1;
		authdialfutz(dom, user);
		_freeattr(a);
	}
	if(!found)
		print("no p9sk1 keys found in factotum\n");
}

void
authdialfutz(char *dom, char *user)
{
	int fd;
	char *server;
	char *addr;

	fd = authdial(nil, dom);
	if(fd >= 0){
		print("\tsuccessfully dialed auth server\n");
		close(fd);
		authfutz(dom, user);
		return;
	}
	print("\tcannot dial auth server: %r\n");
	server = csgetvalue(nil, "authdom", dom, "auth", nil);
	if(server){
		print("\tcsquery authdom=%q auth=%s\n", dom, server);
		free(server);
		return;
	}
	print("\tcsquery authdom=%q auth=* failed\n", dom);
	server = csgetvalue(nil, "dom", dom, "auth", nil);
	if(server){
		print("\tcsquery dom=%q auth=%q\n", dom, server);
		free(server);
		return;
	}
	print("\tcsquery dom=%q auth=*\n", dom);

	fd = dial(addr=netmkaddr("$auth", nil, "ticket"), 0, 0, 0);
	if(fd >= 0){
		print("\tdial %s succeeded\n", addr);
		close(fd);
		return;
	}
	print("\tdial %s failed: %r\n", addr);
}

void
authfutz(char *dom, char *user)
{
	int fd, nobootes;
	char pw[128], prompt[128], key[DESKEYLEN], booteskey[DESKEYLEN], tbuf[2*TICKETLEN],
		trbuf[TICKREQLEN];
	Ticket t;
	Ticketreq tr;

	snprint(prompt, sizeof prompt, "\tpassword for %s@%s [hit enter to skip test]", user, dom);
	readcons(prompt, nil, 1, pw, sizeof pw);
	if(pw[0] == '\0')
		return;
	passtokey(key, pw);

	fd = authdial(nil, dom);
	if(fd < 0){
		print("\tauthdial failed(!): %r\n");
		return;
	}

	/* try ticket request using just user key */
	tr.type = AuthTreq;
	strecpy(tr.authid, tr.authid+sizeof tr.authid, user);
	strecpy(tr.authdom, tr.authdom+sizeof tr.authdom, dom);
	strecpy(tr.hostid, tr.hostid+sizeof tr.hostid, user);
	strecpy(tr.uid, tr.uid+sizeof tr.uid, user);
	memset(tr.chal, 0xAA, sizeof tr.chal);
	convTR2M(&tr, trbuf);
	if(_asgetticket(fd, trbuf, tbuf) < 0){
		close(fd);
		print("\t_asgetticket failed: %r\n");
		return;
	}
	convM2T(tbuf, &t, key);
	if(t.num != AuthTc){
		print("\tcannot decrypt ticket1 from auth server (bad t.num=0x%.2ux)\n", t.num);
		print("\tauth server and you do not agree on key for %s@%s\n", user, dom);
		return;
	}
	if(memcmp(t.chal, tr.chal, sizeof tr.chal) != 0){
		print("\tbad challenge1 from auth server got %.*H wanted %.*H\n",
			sizeof t.chal, t.chal, sizeof tr.chal, tr.chal);
		print("\tauth server is rogue\n");
		return;
	}

	convM2T(tbuf+TICKETLEN, &t, key);
	if(t.num != AuthTs){
		print("\tcannot decrypt ticket2 from auth server (bad t.num=0x%.2ux)\n", t.num);
		print("\tauth server and you do not agree on key for %s@%s\n", user, dom);
		return;
	}
	if(memcmp(t.chal, tr.chal, sizeof tr.chal) != 0){
		print("\tbad challenge2 from auth server got %.*H wanted %.*H\n",
			sizeof t.chal, t.chal, sizeof tr.chal, tr.chal);
		print("\tauth server is rogue\n");
		return;
	}
	print("\tticket request using %s@%s key succeeded\n", user, dom);

	/* try ticket request using bootes key */
	snprint(prompt, sizeof prompt, "\tcpu server owner for domain %s ", dom);
	readcons(prompt, "bootes", 0, tr.authid, sizeof tr.authid);
	convTR2M(&tr, trbuf);
	if(_asgetticket(fd, trbuf, tbuf) < 0){
		close(fd);
		print("\t_asgetticket failed: %r\n");
		return;
	}
	convM2T(tbuf, &t, key);
	if(t.num != AuthTc){
		print("\tcannot decrypt ticket1 from auth server (bad t.num=0x%.2ux)\n", t.num);
		print("\tauth server and you do not agree on key for %s@%s\n", user, dom);
		return;
	}
	if(memcmp(t.chal, tr.chal, sizeof tr.chal) != 0){
		print("\tbad challenge1 from auth server got %.*H wanted %.*H\n",
			sizeof t.chal, t.chal, sizeof tr.chal, tr.chal);
		print("\tauth server is rogue\n");
		return;
	}

	snprint(prompt, sizeof prompt, "\tpassword for %s@%s [hit enter to skip test]", tr.authid, dom);
	readcons(prompt, nil, 1, pw, sizeof pw);
	if(pw[0] == '\0'){
		nobootes=1;
		goto Nobootes;
	}
	nobootes = 0;
	passtokey(booteskey, pw);

	convM2T(tbuf+TICKETLEN, &t, booteskey);
	if(t.num != AuthTs){
		print("\tcannot decrypt ticket2 from auth server (bad t.num=0x%.2ux)\n", t.num);
		print("\tauth server and you do not agree on key for %s@%s\n", tr.authid, dom);
		return;
	}
	if(memcmp(t.chal, tr.chal, sizeof tr.chal) != 0){
		print("\tbad challenge2 from auth server got %.*H wanted %.*H\n",
			sizeof t.chal, t.chal, sizeof tr.chal, tr.chal);
		print("\tauth server is rogue\n");
		return;
	}
	print("\tticket request using %s@%s key succeeded\n", tr.authid, dom);

Nobootes:;
	USED(nobootes);

	/* try p9sk1 exchange with local factotum to test that key is right */


	/*
	 * try p9sk1 exchange with factotum on
	 * auth server (assumes running cpu service)
	 * to test that bootes key is right over there
	 */

}

void
main(int argc, char **argv)
{
	quotefmtinstall();
	fmtinstall('A', _attrfmt);
	fmtinstall('H', encodefmt);

	ARGBEGIN{
	default:
		usage();
	}ARGEND

	if(argc != 0)
		usage();

	debugfactotumkeys();
}
