#include <u.h>
#include <libc.h>

#include "../ssh.h"
#include "server.h"

#define min(a,b) (((a)>(b))?(b):(a))

Waitmsg w;

void
setenv(char *var, char *val)
{
	int fd;

	fd = create(var, OWRITE, 0644);
	if(fd < 0)
		print("init: can't open %s\n", var);
	else{
		fprint(fd, val);
		close(fd);
	}
}

static void
dopackets(int fdin)
{
	char buf[1024];
	ulong hdr[2];
	int m, n;
	Packet *packet;

	for (;;) {
		if ((n = read(fdin, hdr, 8)) == 0) {
			debug(DBG_PROC, "Dopackets exiting\n");
			exits(0);
		}
		if (n != 8) {
			if (n < 0)
				syserror("dopackets: read: %r");
			else
				syserror("dopackets: programming bug #13");
		}
		n = hdr[0];
		m = readn(fdin, &buf[hdr[0]-n], n);
		if(m != n)
			syserror("dopackets: read: %r");
		packet = (Packet *)mmalloc(sizeof(*packet)
				+4		/* crc size */
				+4+hdr[0]);	/* string length */
		packet->type = hdr[1];
		packet->length = 0;
		packet->pos = packet->data;
		switch(packet->type) {
		case SSH_SMSG_EXITSTATUS:
			debug(DBG_PROTO, "Sending exit status, dopackets exiting\n");
			putlong(packet, 0);
			debug(DBG_PROC, "Exit status = 0\n");
			break;
		case SSH_MSG_DISCONNECT:
			debug(DBG_PROC|DBG_PROTO,
			    "Sending disconnect, dopackets exiting\n");
			putstring(packet, buf, hdr[0]);
			putpacket(packet, s2c);
			exits(0);
		default:
			debug(DBG_IO, "Sending %d bytes of data\n",
				hdr[0]);
			putstring(packet, buf, hdr[0]);
		}
		putpacket(packet, s2c);
	}
}

static void
dooutput(int msgtype, int fdin, int fdout)
{
	uchar buf[1024+8];
	ulong *hdr;
	int n;

	while ((n = read(fdin, &buf[8], 1024)) > 0) {
		hdr = (ulong *)buf;
		hdr[0] = n;
		hdr[1] = msgtype;
		write(fdout, buf, n + 8);
	}
	debug(DBG_PROC, "Dooutput exiting\n");
	exits("0");
}

static void
doinput(int fdout)
{
	Packet *packet;
	ulong n;

	while(1) {
		packet = getpacket(c2s);
		if (packet == 0) {
			debug(DBG_PROC, "Doinput exiting\n");
			exits(0);
		}
		switch (packet->type) {
		case SSH_CMSG_STDIN_DATA:
			n = getlong(packet);
			/* Test for cntrl D
			if (*packet->pos == 4) {
				close(fdout);
				fdout = -1;
				break;
			} */
			if (fdout >= 0)
				write(fdout, packet->pos, n);
			break;
		case SSH_CMSG_EOF:
			close(fdout);
			fdout = -1;
			break;
		case SSH_CMSG_EXIT_CONFIRMATION:
			/* Last message sent by client */
			debug(DBG_PROC,
			    "SSH_CMSG_EXIT_CONFIRMATION: Doinput exiting\n");
			exits(0);
		case SSH_CMSG_WINDOW_SIZE:
			continue;
		default:
			syserror("doinput: message type %d", packet->type);
		}
		mfree(packet);
	}
}

void
run_cmd(char *cmd) {
	char *sysname, *tz;
	uchar buf[20];
	ulong *hdr;
	int pipestdin[2], pipestdout[2], pipestderr[2], pipestdall[2];
	int pidshell;
	int pid;

	if (pipe(pipestdin) < 0 ||
	    pipe(pipestdout)< 0 ||
	    pipe(pipestderr)< 0 ||
	    pipe(pipestdall)< 0)
		syserror("run_cmd: pipe: %r");
	
	/* set up a new environment */
	sysname = getenv("sysname");
	tz = getenv("timezone");
	if ((pidshell = rfork(RFFDG|RFPROC|RFNOTEG|RFENVG)) < 0)
		syserror("run_cmd: fork: %r");
	if (pidshell == 0) {
		char direct[2*NAMELEN];

		/* Child */
		if(dup(pipestdin[0], 0) < 0
		|| dup(pipestdout[1], 1) < 0
		|| dup(pipestderr[1], 2) < 0)
			syserror("run_cmd: dup: %r");

		close(pipestdin[0]);
		close(pipestdin[1]);
		close(pipestdout[0]);
		close(pipestdout[1]);
		close(pipestderr[0]);
		close(pipestderr[1]);
		close(pipestdall[0]);
		close(pipestdall[1]);

		setenv("#e/user", user);
		if(sysname != nil)
			setenv("#e/sysname", sysname);
		if(tz != nil)
			setenv("#e/timezone", tz);
	
		/* go to new home directory */
		snprint(direct, sizeof(buf), "/usr/%s", user);

		if(chdir(direct) < 0) chdir("/");
	
		if (cmd) {
			setenv("#e/service", "rx");
			execl("/bin/rc", "rc", "-lc", cmd, 0);
			syserror("Can't execute /bin/rc -c %s", cmd);
		} else {
			setenv("#e/service", "con");
			execl("/bin/ip/telnetd", "telnetd", "-tn", 0);
			syserror("Can't execute /bin/ip/telnetd");
			exits(0);
		}
	}

	close(pipestdin[0]);
	close(pipestderr[1]);
	close(pipestdout[1]);

	switch(rfork(RFFDG|RFPROC|RFNOWAIT)){
	case -1:
		syserror("dopackets: fork: %r");
		break;
	case 0:
		close(0);
		close(pipestdin[1]);
		close(pipestdout[0]);
		close(pipestderr[0]);
		close(pipestdall[1]);
		dopackets(pipestdall[0]);
		exits(0);
	}

	switch(rfork(RFFDG|RFPROC|RFNOWAIT)){
	case -1:
		syserror("dooutput: fork: %r");
		break;
	case 0:
		close(0);
		close(1);
		close(pipestdin[1]);
		close(pipestderr[0]);
		close(pipestdall[0]);
		dooutput(SSH_SMSG_STDOUT_DATA, pipestdout[0], pipestdall[1]);
		exits(0);
	}

	switch(rfork(RFFDG|RFPROC|RFNOWAIT)){
	case -1:
		syserror("dooutput: fork: %r");
		break;
	case 0:
		close(0);
		close(1);
		close(pipestdin[1]);
		close(pipestdout[0]);
		close(pipestdall[0]);
		dooutput(SSH_SMSG_STDERR_DATA, pipestderr[0], pipestdall[1]);
		exits(0);
	}
	debug(DBG_PROC, "Starting %s\n", cmd);

	close(pipestdall[0]);
	close(pipestderr[0]);
	close(pipestdout[0]);

	switch(rfork(RFFDG|RFPROC|RFNOWAIT)){
	case -1:
		syserror("doinput: fork: %r");
		break;
	case 0:
		close(pipestdall[1]);
		doinput(pipestdin[1]);
		exits(0);
	}

	close(pipestdin[1]);

	debug(DBG_PROC, "Waiting for exit status\n");
	do {
		if ((pid = wait(&w)) < 0)
			syserror("run_cmd: wait");
	} while(pid != pidshell);
	hdr = (ulong *)buf;
	if (w.msg[0]) {
		hdr[0] = min(strlen(w.msg),sizeof(w.msg));
		hdr[1] = SSH_MSG_DISCONNECT;
		memcpy(buf+8, w.msg, sizeof(w.msg));
		write(pipestdall[1], buf, 8+hdr[0]);
	} else {
		hdr[0] = 0;
		hdr[1] = SSH_SMSG_EXITSTATUS;
		write(pipestdall[1], buf, 8);
	}
	debug(DBG_PROC, "Exit status: %s\n", w.msg[0]?w.msg:"(0)");
	close(pipestdall[1]);
	exits("0");
}
