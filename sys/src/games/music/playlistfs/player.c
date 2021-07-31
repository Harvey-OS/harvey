#include <u.h>
#include <libc.h>
#include <thread.h>
#include <fcall.h>
#include "pool.h"
#include "playlist.h"

enum {
	Pac,
	Mp3,
	Pcm,
	Ogg,
};

typedef struct Playfd Playfd;

struct Playfd {
	/* Describes a file to play for starting up pac4dec/mp3,... */
	char	*filename;	/* mallocated */
	int	fd;		/* filedesc to use */
	int	cfd;		/* fildesc to close */
};

Channel *full, *empty, *playout, *spare;
Channel	*playc, *pacc;

char *playprog[] = {
[Pac] = "/bin/games/pac4dec",
[Mp3] = "/bin/games/mp3dec",
[Pcm] = "/bin/cp",
[Ogg] = "/bin/games/vorbisdec",
};

ulong totbytes, totbuffers;

static char curfile[8192];

void
pac4dec(void *a)
{
	Playfd *pfd;
	Pacbuf *pb;
	int fd, type;
	char *ext, buf[256];
	static char args[6][32];
	char *argv[6] = {args[0], args[1], args[2], args[3], args[4], args[5]};

	threadsetname("pac4dec");
	pfd = a;
	close(pfd->cfd);	/* read fd */
	ext = strrchr(pfd->filename, '.');
	fd = open(pfd->filename, OREAD);
	if (fd < 0 && ext == nil){
		// Try the alternatives
		ext = buf + strlen(pfd->filename);
		snprint(buf, sizeof buf, "%s.pac", pfd->filename);
		fd = open(buf, OREAD);
		if (fd < 0){
			snprint(buf, sizeof buf, "%s.mp3", pfd->filename);
			fd = open(buf, OREAD);
		}
		if (fd < 0){
			snprint(buf, sizeof buf, "%s.ogg", pfd->filename);
			fd = open(buf, OREAD);
		}
		if (fd < 0){
			snprint(buf, sizeof buf, "%s.pcm", pfd->filename);
			fd = open(buf, OREAD);
		}
	}
	if (fd < 0){
		if (debug & DbgPlayer)
			fprint(2, "pac4dec: %s: %r", pfd->filename);
		pb = nbrecvp(spare);
		pb->cmd = Error;
		pb->off = 0;
		pb->len = snprint(pb->data, sizeof(pb->data), "startplay: %s: %r", pfd->filename);
		sendp(full, pb);
		threadexits("open");
	}
	dup(pfd->fd, 1);
	close(pfd->fd);
	if(ext == nil || strcmp(ext, ".pac") == 0){
		type = Pac;
		snprint(args[0], sizeof args[0], "pac4dec");
		snprint(args[1], sizeof args[1], "/fd/%d", fd);
		snprint(args[2], sizeof args[2], "/fd/1");
		argv[3] = nil;
	}else if(strcmp(ext, ".mp3") == 0){
		type = Mp3;
		snprint(args[0], sizeof args[0], "mp3dec");
		snprint(args[1], sizeof args[1], "-q");
		snprint(args[2], sizeof args[1], "-s");
		snprint(args[3], sizeof args[1], "/fd/%d", fd);
		argv[4] = nil;
	}else if(strcmp(ext, ".ogg") == 0){
		type = Ogg;
		snprint(args[0], sizeof args[0], "vorbisdec");
		argv[1] = nil;
		argv[2] = nil;
		argv[3] = nil;
		dup(fd, 0);
	}else{
		type = Pcm;
		snprint(args[0], sizeof args[0], "cat");
		snprint(args[1], sizeof args[1], "/fd/%d", fd);
		argv[2] = nil;
		argv[3] = nil;
	}
	free(pfd->filename);
	free(pfd);
	if (debug & DbgPlayer)
		fprint(2, "procexecl %s %s %s %s\n",
			playprog[type], argv[0], argv[1], argv[2]);
	procexec(nil, playprog[type], argv);
	if((pb = nbrecvp(spare)) == nil)
		pb = malloc(sizeof(Pacbuf));
	pb->cmd = Error;
	pb->off = 0;
	pb->len = snprint(pb->data, sizeof(pb->data), "startplay: %s: exec", playprog[type]);
	sendp(full, pb);
	threadexits(playprog[type]);
}

static int
startplay(ushort n)
{
	int fd[2];
	Playfd *pfd;
	char *file;

	file = getplaylist(n);
	if(file == nil)
		return Undef;
	if (debug & DbgPlayer)
		fprint(2, "startplay: file is `%s'\n", file);
	if(pipe(fd) < 0)
		sysfatal("pipe: %r");
	pfd = malloc(sizeof(Playfd));
	pfd->filename = file;	/* mallocated already */
	pfd->fd = fd[1];
	pfd->cfd = fd[0];
	procrfork(pac4dec, pfd, 4096, RFFDG);
	close(fd[1]);	/* write fd, for pac4dec */
	return fd[0];	/* read fd */
}

static void
rtsched(void)
{
	int fd;
	char *ctl;

	ctl = smprint("/proc/%ud/ctl", getpid());
	if((fd = open(ctl, ORDWR)) < 0) 
		sysfatal("%s: %r", ctl);
	if(fprint(fd, "period 20ms") < 0)
		sysfatal("%s: %r", ctl);
	if(fprint(fd, "cost 100µs") < 0)
		sysfatal("%s: %r", ctl);
	if(fprint(fd, "sporadic") < 0)
		sysfatal("%s: %r", ctl);
	if(fprint(fd, "admit") < 0)
		sysfatal("%s: %r", ctl);
	close(fd);
	free(ctl);
}

void
pacproc(void*)
{
	Pmsg playstate, newstate;
	int fd;
	Pacbuf *pb;
	Alt a[3] = {
		{empty, &pb, CHANNOP},
		{playc, &newstate.m, CHANRCV},
		{nil, nil, CHANEND},
	};

	threadsetname("pacproc");
	close(srvfd[1]);
	newstate.cmd = playstate.cmd = Stop;
	newstate.off = playstate.off = 0;
	fd = -1;
	for(;;){
		switch(alt(a)){
		case 0:
			/* Play out next buffer (pb points to one already) */
			assert(fd >= 0);	/* Because we must be in Play mode */
			pb->m = playstate.m;
			pb->len = read(fd, pb->data, sizeof pb->data);
			if(pb->len > 0){
				sendp(full, pb);
				break;
			}
			if(pb->len < 0){
				if(debug & DbgPlayer)
					fprint(2, "pac, error: %d\n", playstate.off);
				pb->cmd = Error;
				pb->len = snprint(pb->data, sizeof pb->data, "%s: %r", curfile);
				sendp(full, pb);
			}else{
				/* Simple end of file */
				sendp(empty, pb); /* Don't need buffer after all */
			}
			close(fd);
			fd = -1;
			if(debug & DbgPlayer)
				fprint(2, "pac, eof: %d\n", playstate.off);
			/* End of file, do next by falling through */
			newstate.cmd = playstate.cmd;
			newstate.off = playstate.off + 1;
		case 1:
			if((debug & DbgPac) && newstate.cmd)
				fprint(2, "Pacproc: newstate %s-%d, playstate %s-%d\n",
					statetxt[newstate.cmd], newstate.off,
					statetxt[playstate.cmd], playstate.off);
			/* Deal with an incoming command */
			if(newstate.cmd == Pause || newstate.cmd == Resume){
				/* Just pass them on, don't change local state */
				pb = recvp(spare);
				pb->m = newstate.m;
				sendp(full, pb);
				break;
			}
			/* Stop whatever we're doing */
			if(fd >= 0){
				if(debug & DbgPlayer)
					fprint(2, "pac, stop\n");
				/* Stop any active (pac) decoders */
				close(fd);
				fd = -1;
			}
			a[0].op = CHANNOP;
			switch(newstate.cmd){
			default:
				sysfatal("pacproc: unexpected newstate %d", newstate.cmd);
			case Stop:
				/* Wait for state to change */
				break;
			case Skip:
			case Play:
				fd = startplay(newstate.off);
				if(fd >=0){
					playstate = newstate;
					a[0].op = CHANRCV;
					continue;	/* Start reading */
				}
				newstate.cmd = Stop;
			}
			pb = recvp(spare);
			pb->m = newstate.m;
			sendp(full, pb);
			playstate = newstate;
		}
	}
}

void
pcmproc(void*)
{
	Pmsg localstate, newstate, prevstate;
	int fd, n;
	Pacbuf *pb, *b;
	Alt a[3] = {
		{full, &pb, CHANRCV},
		{playout, &pb, CHANRCV},
		{nil, nil, CHANEND},
	};

	/*
	 * This is the real-time proc.
	 * It gets its input from two sources, full data/control buffers from the pacproc
	 * which mixes decoded data with control messages, and data buffers from the pcmproc's
	 * (*this* proc's) own internal playout buffer.
	 * When a command is received on the `full' channel containing a command that warrants
	 * an immediate change of audio source (e.g., to silence or to another number), we just
	 * toss everything in the pipeline -- i.e., the playout channel
	 * Finally, we report all state changes using `playupdate' (another message channel)
	 */
	threadsetname("pcmproc");
	close(srvfd[1]);
	fd = -1;
	localstate.cmd = 0;	/* Force initial playupdate */
	newstate.cmd = Stop;
	newstate.off = 0;
//	rtsched();
	for(;;){
		if(newstate.m != localstate.m){
			playupdate(newstate, nil);
			localstate = newstate;
		}
		switch(alt(a)){
		case 0:
			/* buffer received from pacproc */
			if((debug & DbgPcm) && localstate.m != prevstate.m){
				fprint(2, "pcm, full: %s-%d, local state is %s-%d\n",
					statetxt[pb->cmd], pb->off,
					statetxt[localstate.cmd], localstate.off);
				prevstate.m = localstate.m;
			}
			switch(pb->cmd){
			default:
				sysfatal("pcmproc: unknown newstate: %s-%d", statetxt[pb->cmd], pb->off);
			case Resume:
				a[1].op = CHANRCV;
				newstate.cmd = Play;
				break;
			case Pause:
				a[1].op = CHANNOP;
				newstate.cmd = Pause;
				if(fd >= 0){
					close(fd);
					fd = -1;
				}
				break;
			case Stop:
				/* Dump all data in the buffer */
				while(b = nbrecvp(playout))
					if(b->cmd == Error){
						playupdate(b->Pmsg, b->data);
						sendp(spare, b);
					}else
						sendp(empty, b);
				newstate.m = pb->m;
				a[1].op = CHANRCV;
				if(fd >= 0){
					close(fd);
					fd = -1;
				}
				break;
			case Skip:
				/* Dump all data in the buffer, then fall through */
				while(b = nbrecvp(playout))
					if(b->cmd == Error){
						playupdate(pb->Pmsg, pb->data);
						sendp(spare, pb);
					}else
						sendp(empty, b);
				a[1].op = CHANRCV;
				newstate.cmd = Play;
			case Error:
			case Play:
				/* deal with at playout, just requeue */
				sendp(playout, pb);
				pb = nil;
				localstate = newstate;
				break;
			}
			/* If we still have a buffer, free it */
			if(pb)
				sendp(spare, pb);
			break;
		case 1:
			/* internal buffer */
			if((debug & DbgPlayer) && localstate.m != prevstate.m){
				fprint(2, "pcm, playout: %s-%d, local state is %s-%d\n",
					statetxt[pb->cmd], pb->off,
					statetxt[localstate.cmd], localstate.off);
				prevstate.m = localstate.m;
			}
			switch(pb->cmd){
			default:
				sysfatal("pcmproc: unknown newstate: %s-%d", statetxt[pb->cmd], pb->off);
			case Error:
				playupdate(pb->Pmsg, pb->data);
				localstate = newstate;
				sendp(spare, pb);
				break;
			case Play:
				if(fd < 0 && (fd = open("/dev/audio", OWRITE)) < 0){
					a[1].op = CHANNOP;
					newstate.cmd = Pause;
					pb->cmd = Error;
					snprint(pb->data, sizeof(pb->data),
						"/dev/audio: %r");
					playupdate(pb->Pmsg, pb->data);
					sendp(empty, pb);
					break;
				}
				/* play out this buffer */
				totbytes += pb->len;
				totbuffers++;
				n = write(fd, pb->data, pb->len);
				if (n != pb->len){
					if (debug & DbgPlayer)
						fprint(2, "pcmproc: file %d: %r\n", pb->off);
					if (n < 0)
						sysfatal("pcmproc: write: %r");
				}
				newstate.m = pb->m;
				sendp(empty, pb);
				break;
			}
			break;
		}
	}
}

void
playinit(void)
{
	int i;

	full = chancreate(sizeof(Pacbuf*), 1);
	empty = chancreate(sizeof(Pacbuf*), NPacbuf);
	spare = chancreate(sizeof(Pacbuf*), NSparebuf);
	playout = chancreate(sizeof(Pacbuf*), NPacbuf+NSparebuf);
	for(i = 0; i < NPacbuf; i++)
		sendp(empty, malloc(sizeof(Pacbuf)));
	for(i = 0; i < NSparebuf; i++)
		sendp(spare, malloc(sizeof(Pacbuf)));

	playc = chancreate(sizeof(Pmsg), 1);
	procrfork(pacproc, nil, 32*1024, RFFDG);
	procrfork(pcmproc, nil, 32*1024, RFFDG);
}

char *
getplaystat(char *p, char *e)
{
	p = seprint(p, e, "empty buffers %d of %d\n", empty->n, empty->s);
	p = seprint(p, e, "full buffers %d of %d\n", full->n, full->s);
	p = seprint(p, e, "playout buffers %d of %d\n", playout->n, playout->s);
	p = seprint(p, e, "spare buffers %d of %d\n", spare->n, spare->s);
	p = seprint(p, e, "bytes %lud / buffers %lud played\n", totbytes, totbuffers);
	return p;
}
