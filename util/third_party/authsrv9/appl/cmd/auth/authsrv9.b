implement Authsrv9;

include "sys.m";
	sys: Sys;
	sprint: import sys;
include "draw.m";
include "arg.m";
include "daytime.m";
	daytime: Daytime;
include "string.m";
	str: String;
include "keyring.m";
include "security.m";
	random: Random;
include "auth9.m";
	auth9: Auth9;
	Ticketreq, Ticket, Passwordreq: import auth9;

Authsrv9: module {
	init:	fn(nil: ref Draw->Context, args: list of string);
};

dflag := 1;
timerpid := -1;
authdir: con "/services/authsrv9/";
logfd: ref Sys->FD;

init(nil: ref Draw->Context, args: list of string)
{
	sys = load Sys Sys->PATH;
	arg := load Arg Arg->PATH;
	daytime = load Daytime Daytime->PATH;
	str = load String String->PATH;
	auth9 = load Auth9 Auth9->PATH;
	auth9->init();
	random = load Random Random->PATH;

	arg->init(args);
	arg->setusage(arg->progname()+" [-d]");
	while((c := arg->opt()) != 0)
		case c {
		'd' =>	dflag++;
		* =>	arg->usage();
		}
	args = arg->argv();
	if(len args != 0)
		arg->usage();

	logfd = sys->open("/services/logs/authsrv9", Sys->OWRITE);
	if(logfd == nil)
		logfd = sys->fildes(2);
	else
		sys->seek(logfd, big 0, Sys->SEEKEND);

	pid := sys->pctl(0, nil);
	spawn timer(pid, 2*60, pidc := chan of int);
	timerpid = <-pidc;

	for(;;)
		transact();

	kill(timerpid);
}

timer(pid: int, n: int, pidc: chan of int)
{
	pidc <-= sys->pctl(0, nil);
	sys->sleep(n*1000);
	kill(pid);
}

ewrite(buf: array of byte)
{
	if(sys->write(sys->fildes(1), buf, len buf) != len buf)
		fail(sprint("write: %r"));
}

autherror(fatal: int, s: string)
{
	say(sprint("autherror, fatal %d: %s", fatal, s));

	tresp := array[1+Auth9->AERRLEN] of {* => byte 0};
	tresp[0] = byte Auth9->AuthErr;
	err := array of byte s;
	err = err[:min(len err, Auth9->AERRLEN)];
	tresp[1:] = err;
	ewrite(tresp);
	if(fatal) {
		kill(timerpid);
		raise "fail:fatal autherror";
	}
}

readfile(p: string): array of byte
{
	fd := sys->open(p, Sys->OREAD);
	if(fd == nil)
		raise sprint("io:open: %r");
	buf := array[128] of byte;
	n := sys->readn(fd, buf, len buf);
	if(n < 0)
		raise sprint("io:read: %r");
	if(n == len buf)
		raise sprint("io:file too large");
	return buf[:n];
}

writefile(p: string, d: array of byte): string
{
	fd := sys->open(p, Sys->OWRITE|Sys->OTRUNC);
	if(fd == nil)
		return sprint("open: %r");
	if(sys->write(fd, d, len d) != len d)
		return sprint("write: %r");
	return nil;
}

getinfo(): (string, string)
{
	{
		authid := string readfile(authdir+"authid");
		authdom := string readfile(authdir+"authdom");
		return (authid, authdom);
	} exception e {
	"io:*" =>
		warn("getinfo: "+e);
		autherror(1, "malconfigured");
		raise "fail:fatal";  # autherror will raise error
	}
}

User: adt {
	keyok:	int;
	key:	array of byte;
	status:	int; # 0 disabled, 1 ok
	expire:	int;
};

getuserinfo(uid: string): ref User
{
	{
		pre := authdir+"users/"+uid;
		u := ref User;
		u.key = readfile(pre+"/key");
		u.keyok = 1;
		if(len u.key != Auth9->DESKEYLEN) {
			u.key = nil;
			u.keyok = 0;
		}
		statusstr := string readfile(pre+"/status");
		u.status = 0;
		case statusstr {
		"ok" =>		u.status = 1;
		"disabled" =>	;
		* =>	warn(sprint("bad status %q for user %q", statusstr, uid));
		}
		expirestr := string readfile(pre+"/expire");
		u.expire = 1;
		if(expirestr == "never") {
			u.expire = 0;
		} else {
			rem: string;
			(u.expire, rem) = str->toint(expirestr, 10);
			if(rem != nil) {
				u.expire = 1;
				warn(sprint("bad expire %q for user %q, treating as expired", expirestr, uid));
			}
		}
		return u;
	} exception {
	"io:*" =>
		return ref User (0, nil, 0, 0);
	}
}

genkey(): array of byte
{
	return random->randombuf(Random->ReallyRandom, Auth9->DESKEYLEN);
}

clear(d: array of byte)
{
	for(i := 0; i < len d; i++)
		d[i] = byte 0;
}

clearstr(s: string)
{
	for(i := 0; i < len s; i++)
		s[i] = 0;
}

clearpr(pr: ref Passwordreq)
{
	clear(pr.old);
	clear(pr.new);
	clear(pr.secret);
}

allowed(uid: string): int
{
	path := authdir+"badusers";
	{
		s := string readfile(path);
		for(l := sys->tokenize(s, "\n").t1; l != nil; l = tl l)
			if(hd l == uid)
				return 0;
		return 1;
	} exception e {
	"io:*" =>
		warn(sprint("open %q: %s", path, e[len "io:":]));
		return 0;
	}
}

authtreq(tr: ref Ticketreq)
{
	# C->A: AuthTreq, IDs, DN, CHs, IDc, IDr

	(authid, authdom) := getinfo();
	au := getuserinfo(authid);
	now := daytime->now();
	say(sprint("authid, keyok %d, status %d, expire %d, %d", au.keyok, au.status, au.expire, now));
	ks := genkey();
        sok := tr.authid == authid && tr.authdom == authdom && au.keyok && au.status && (au.expire == 0 || now > au.expire);

	u := getuserinfo(tr.hostid);
	say(sprint("hostid %#q, user '%s', keyok %d, status %d, expire %d, now %d", tr.hostid, tr.uid, u.keyok, u.status, u.expire, now));
	kc := genkey();
	cok := u.keyok && u.status && (u.expire == 0 || u.expire > now);
	if(sok && cok) {
		ks = au.key;
		kc = u.key;
	}

	# A->C: AuthOK, Kc{AuthTc, CHs, IDc, IDr, Kn}, Ks{AuthTs, CHs, IDc, IDr, Kn} 
	if(tr.hostid == tr.uid || (tr.hostid == authid && allowed(tr.uid)))
		idr := tr.uid;
	else
		msg := ", but hostid does not speak for uid";
	status := "bad";
	if(sok && cok)
		status = "ok";
	warn(sprint("ticketreq %s: authid %s, remote hostid %s, uid %s%s", status, authid, tr.hostid, tr.uid, msg));
	kn := genkey();
	tc := ref Ticket (Auth9->AuthTc, tr.chal, tr.hostid, idr, kn);
	tcbuf := tc.pack(kc);
	ts := ref Ticket (Auth9->AuthTs, tr.chal, tr.hostid, idr, kn);
	tsbuf := ts.pack(ks);

	clear(kc);
	clear(ks);
	clear(kn);
	clear(au.key);
	clear(u.key);

	tresp := array[1+len tcbuf+len tsbuf] of byte;
	tresp[0] = byte Auth9->AuthOK;
	tresp[1:] = tcbuf;
	tresp[1+len tcbuf:] = tsbuf;
	ewrite(tresp);
}

checkpass(pw: string): string
{
	d := array of byte pw;
	if(len d < 8)
		return "too small";
	if(len d > Auth9->ANAMELEN)
		return "too long";
	return nil;
}

authpass(tr: ref Ticketreq)
{
	# C->A: AuthPass, IDc, DN, CHc, IDc, IDc
	# (request to change pass for tr.uid)
	u := getuserinfo(tr.uid);

	# A->C: Kc{AuthTp, CHc, IDc, IDc, Kn}
	now := daytime->now();
	say(sprint("authpass for user %s, u.keyok %d, u.status %d, u.expire %d, now %d", tr.uid, u.keyok, u.status, u.expire, now));
	kc := genkey();
	if(u.keyok && u.status && (u.expire == 0 || now > u.expire))
		kc[:] = u.key;
	clear(u.key);
	kn := genkey();
	tp := ref Ticket (Auth9->AuthTp, tr.chal, tr.uid, tr.uid, kn);
	tpbuf := tp.pack(kc);
	tresp := array[1+Auth9->TICKETLEN] of {* => byte 0};
	tresp[0] = byte Auth9->AuthOK;
	tresp[1:] = tpbuf;
	ewrite(tresp);
	clear(tpbuf);
	clear(tresp);

	for(;;) {
		# C->A: Kn{AuthPass, old, new, changesecret, secret}

		prbuf := array[Auth9->PASSREQLEN] of byte;
		n := sys->readn(sys->fildes(0), prbuf, len prbuf);
		if(n < 0)
			fail(sprint("read passwordreq: %r"));
		if(n != len prbuf)
			fail(sprint("short read for passwordreq, want %d, got %d", len prbuf, n));
		(nil, pr) := Passwordreq.unpack(prbuf, kn);
		clear(prbuf);

		if(pr.num != Auth9->AuthPass) {
                        warn(sprint("pass change: for uid %s: wrong message type, want %d, saw %d (wrong password used)", tr.uid, Auth9->AuthPass, pr.num));
			clearpr(pr);
			clear(kc);
			autherror(1, "wrong message type for Passreq");
		}
		if(pr.changesecret) {
                        warn(sprint("pass change: uid %s tried to change apop secret, not supported", tr.uid));
			autherror(0, "changing apop secret not supported");
			clearpr(pr);
			continue;
		}

		oldpw := cstr(pr.old);
		newpw := cstr(pr.new);
		okey := auth9->passtokey(oldpw);
		clearstr(oldpw);
		nkey := auth9->passtokey(newpw);
		if(!eq(kc, okey)) {
			clearstr(newpw);
			clearpr(pr);
			clear(okey);
			clear(nkey);
                        warn(sprint("pass change: uid %s gave bad old password", tr.uid));
                        autherror(0, "bad old password");
			continue;
		}

		badpw := checkpass(newpw);
		clearstr(newpw);
		if(badpw != nil) {
			clearpr(pr);
			clear(okey);
			clear(nkey);
                        warn(sprint("pass change: uid %s gave bad new password: %s", tr.uid, badpw));
                        autherror(0, badpw);
			continue;
		}

		keypath := authdir+sprint("users/%s/key", tr.uid);
		err := writefile(keypath, nkey);
		if(err != nil) {
			clearpr(pr);
                        clear(okey);
                        clear(nkey);
			clear(kc);
			warn(sprint("pass change: storing new key for user %s failed: %s", tr.uid, err));
			autherror(1, "storing new key failed");
		}

		clearpr(pr);
		clear(okey);
		clear(nkey);
		clear(kc);
		warn(sprint("pass change: password changed for user %s", tr.uid));

		# A->C: AuthOK or AuthErr, 64byte error message
		tresp = array[] of {byte Auth9->AuthOK};
		ewrite(tresp);
		return;
	}
}

transact()
{
	say("reading ticketrequest");

	treq := array[Auth9->TICKREQLEN] of byte;
	n := sys->readn(sys->fildes(0), treq, len treq);
	if(n < 0)
		fail(sprint("read ticketreq: %r"));
	if(n == 0) {
		kill(timerpid);
		raise "fail:";
	}
	if(n != len treq)
		fail(sprint("short read for ticketreq, wanted %d, got %d", len treq, n));
	(nil, tr) := Ticketreq.unpack(treq);
	say(sprint("ticketreq, rtype %d, authid %q authdom %q, chal %q hostid %q uid %q", tr.rtype, tr.authid, tr.authdom, hex(tr.chal), tr.hostid, tr.uid));

	case tr.rtype {
	Auth9->AuthTreq =>
		authtreq(tr);
	Auth9->AuthPass =>
		authpass(tr);
	* =>
		autherror(1, "not supported");
	}
}

cstr(d: array of byte): string
{
	for(i := 0; i < len d; i++)
		if(d[i] == byte 0)
			break;
	return string d[:i];
}

eq(a, b: array of byte): int
{
	if(len a != len b)
		return 0;
	for(i := 0; i < len a; i++)
		if(a[i] != b[i])
			return 0;
	return 1;
}

kill(pid: int)
{
	fd := sys->open(sprint("/prog/%d/ctl", pid), Sys->OWRITE);
	if(fd != nil)
		sys->fprint(fd, "kill");
}

min(a, b: int): int
{
	if(a < b)
		return a;
	return b;
}

hex(d: array of byte): string
{
	s := "";
	for(i := 0; i < len d; i++)
		s += sprint("%02x", int d[i]);
	return s;
}

warn(s: string)
{
	sys->fprint(logfd, "%s\n", s);
}

say(s: string)
{
	if(dflag)
		warn(s);
}

fail(s: string)
{
	warn(s);
	if(timerpid >= 0)
		kill(timerpid);
	raise "fail:"+s;
}
