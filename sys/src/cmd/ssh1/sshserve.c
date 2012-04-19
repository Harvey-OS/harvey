#include "ssh.h"

char *cipherlist = "blowfish rc4 3des";
char *authlist = "tis";

void fromnet(Conn*);
void startcmd(Conn*, char*, int*, int*);
int maxmsg = 256*1024;

Cipher *allcipher[] = {
	&cipherrc4,
	&cipherblowfish,
	&cipher3des,
	&cipherdes,
	&ciphernone,
	&ciphertwiddle,
};

Authsrv *allauthsrv[] = {
	&authsrvpassword,
	&authsrvtis,
};

Cipher*
findcipher(char *name, Cipher **list, int nlist)
{
	int i;

	for(i=0; i<nlist; i++)
		if(strcmp(name, list[i]->name) == 0)
			return list[i];
	error("unknown cipher %s", name);
	return nil;
}

Authsrv*
findauthsrv(char *name, Authsrv **list, int nlist)
{
	int i;

	for(i=0; i<nlist; i++)
		if(strcmp(name, list[i]->name) == 0)
			return list[i];
	error("unknown authsrv %s", name);
	return nil;
}

void
usage(void)
{
	fprint(2, "usage: sshserve [-A authlist] [-c cipherlist] client-ip-address\n");
	exits("usage");
}

void
main(int argc, char **argv)
{
	char *f[16];
	int i;
	Conn c;

	fmtinstall('B', mpfmt);
	fmtinstall('H', encodefmt);
	atexit(atexitkiller);
	atexitkill(getpid());

	memset(&c, 0, sizeof c);

	ARGBEGIN{
	case 'D':
		debuglevel = atoi(EARGF(usage()));
		break;
	case 'A':
		authlist = EARGF(usage());
		break;
	case 'c':
		cipherlist = EARGF(usage());
		break;
	default:
		usage();
	}ARGEND

	if(argc != 1)
		usage();
	c.host = argv[0];

	sshlog("connect from %s", c.host);

	/* limit of 768 bits in remote host key? */
	c.serverpriv = rsagen(768, 6, 0);
	if(c.serverpriv == nil)
		sysfatal("rsagen failed: %r");
	c.serverkey = &c.serverpriv->pub;

	c.nokcipher = getfields(cipherlist, f, nelem(f), 1, ", ");
	c.okcipher = emalloc(sizeof(Cipher*)*c.nokcipher);
	for(i=0; i<c.nokcipher; i++)
		c.okcipher[i] = findcipher(f[i], allcipher, nelem(allcipher));

	c.nokauthsrv = getfields(authlist, f, nelem(f), 1, ", ");
	c.okauthsrv = emalloc(sizeof(Authsrv*)*c.nokauthsrv);
	for(i=0; i<c.nokauthsrv; i++)
		c.okauthsrv[i] = findauthsrv(f[i], allauthsrv, nelem(allauthsrv));

	sshserverhandshake(&c);

	fromnet(&c);
}

void
fromnet(Conn *c)
{
	int infd, kidpid, n;
	char *cmd;
	Msg *m;

	infd = kidpid = -1;
	for(;;){
		m = recvmsg(c, -1);
		if(m == nil)
			exits(nil);
		switch(m->type){
		default:
			//badmsg(m, 0);
			sendmsg(allocmsg(c, SSH_SMSG_FAILURE, 0));
			break;

		case SSH_MSG_DISCONNECT:
			sysfatal("client disconnected");

		case SSH_CMSG_REQUEST_PTY:
			sendmsg(allocmsg(c, SSH_SMSG_SUCCESS, 0));
			break;

		case SSH_CMSG_X11_REQUEST_FORWARDING:
			sendmsg(allocmsg(c, SSH_SMSG_FAILURE, 0));
			break;

		case SSH_CMSG_MAX_PACKET_SIZE:
			maxmsg = getlong(m);
			sendmsg(allocmsg(c, SSH_SMSG_SUCCESS, 0));
			break;

		case SSH_CMSG_REQUEST_COMPRESSION:
			sendmsg(allocmsg(c, SSH_SMSG_FAILURE, 0));
			break;

		case SSH_CMSG_EXEC_SHELL:
			startcmd(c, nil, &kidpid, &infd);
			goto InteractiveMode;

		case SSH_CMSG_EXEC_CMD:
			cmd = getstring(m);
			startcmd(c, cmd, &kidpid, &infd);
			goto InteractiveMode;
		}
		free(m);
	}

InteractiveMode:
	for(;;){
		free(m);
		m = recvmsg(c, -1);
		if(m == nil)
			exits(nil);
		switch(m->type){
		default:
			badmsg(m, 0);

		case SSH_MSG_DISCONNECT:
			postnote(PNGROUP, kidpid, "hangup");
			sysfatal("client disconnected");

		case SSH_CMSG_STDIN_DATA:
			if(infd != 0){
				n = getlong(m);
				write(infd, getbytes(m, n), n);
			}
			break;

		case SSH_CMSG_EOF:
			close(infd);
			infd = -1;
			break;

		case SSH_CMSG_EXIT_CONFIRMATION:
			/* sent by some clients as dying breath */
			exits(nil);
	
		case SSH_CMSG_WINDOW_SIZE:
			/* we don't care */
			break;
		}
	}
}

void
copyout(Conn *c, int fd, int mtype)
{
	char buf[8192];
	int n, max, pid;
	Msg *m;

	max = sizeof buf;
	if(max > maxmsg - 32)	/* 32 is an overestimate of packet overhead */
		max = maxmsg - 32;
	if(max <= 0)
		sysfatal("maximum message size too small");
	
	switch(pid = rfork(RFPROC|RFMEM|RFNOWAIT)){
	case -1:
		sysfatal("fork: %r");
	case 0:
		break;
	default:
		atexitkill(pid);
		return;
	}

	while((n = read(fd, buf, max)) > 0){
		m = allocmsg(c, mtype, 4+n);
		putlong(m, n);
		putbytes(m, buf, n);
		sendmsg(m);
	}
	exits(nil);
}

void
startcmd(Conn *c, char *cmd, int *kidpid, int *kidin)
{
	int i, pid, kpid;
	int pfd[3][2];
	char *dir;
	char *sysname, *tz;
	Msg *m;
	Waitmsg *w;

	for(i=0; i<3; i++)
		if(pipe(pfd[i]) < 0)
			sysfatal("pipe: %r");

	sysname = getenv("sysname");
	tz = getenv("timezone");

	switch(pid = rfork(RFPROC|RFMEM|RFNOWAIT)){
	case -1:
		sysfatal("fork: %r");
	case 0:
		switch(kpid = rfork(RFPROC|RFNOTEG|RFENVG|RFFDG)){
		case -1:
			sysfatal("fork: %r");
		case 0:
			for(i=0; i<3; i++){
				if(dup(pfd[i][1], i) < 0)
					sysfatal("dup: %r");
				close(pfd[i][0]);
				close(pfd[i][1]);
			}
			putenv("user", c->user);
			if(sysname)
				putenv("sysname", sysname);
			if(tz)
				putenv("tz", tz);
	
			dir = smprint("/usr/%s", c->user);
			if(dir == nil || chdir(dir) < 0)
				chdir("/");
			if(cmd){
				putenv("service", "rx");
				execl("/bin/rc", "rc", "-lc", cmd, nil);
				sysfatal("cannot exec /bin/rc: %r");
			}else{
				putenv("service", "con");
				execl("/bin/ip/telnetd", "telnetd", "-tn", nil);
				sysfatal("cannot exec /bin/ip/telnetd: %r");
			}
		default:
			*kidpid = kpid;
			rendezvous(kidpid, 0);
			for(;;){
				if((w = wait()) == nil)
					sysfatal("wait: %r");
				if(w->pid == kpid)
					break;
				free(w);
			}
			if(w->msg[0]){
				m = allocmsg(c, SSH_MSG_DISCONNECT, 4+strlen(w->msg));
				putstring(m, w->msg);
				sendmsg(m);
			}else{
				m = allocmsg(c, SSH_SMSG_EXITSTATUS, 4);
				putlong(m, 0);
				sendmsg(m);
			}
			for(i=0; i<3; i++)
				close(pfd[i][0]);
			free(w);
			exits(nil);	
			break;
		}
	default:
		atexitkill(pid);
		rendezvous(kidpid, 0);
		break;
	}

	for(i=0; i<3; i++)
		close(pfd[i][1]);

	copyout(c, pfd[1][0], SSH_SMSG_STDOUT_DATA);
	copyout(c, pfd[2][0], SSH_SMSG_STDERR_DATA);
	*kidin = pfd[0][0];
}
