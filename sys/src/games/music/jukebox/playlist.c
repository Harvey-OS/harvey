#include <u.h>
#include <libc.h>
#include <thread.h>
#include <draw.h>
#include <keyboard.h>
#include <mouse.h>
#include <control.h>
#include "playlist.h"
#include "../debug.h"

char *playlistfile = "/mnt/playlist";
char *playctlfile = "/mnt/playctl";
char *playvolfile = "/mnt/playvol";
char *volumefile = "/dev/audioctl";

Playlist	playlist;
int		playctlfd;

void
playlistproc(void*)
{
	int fd, m, n, nf;
	static char buf[8192+1];
	char *p, *q, *fields[4];

	threadsetname("playlistproc");
	fd = open(playlistfile, OREAD);
	if(fd < 0)
		sysfatal("%s: %r", playlistfile);
	p = buf;
	n = 0;
	if(debug & DBGPLAY)
		fprint(2, "playlistproc: starting\n");
	for(;;){
		m = read(fd, buf+n, sizeof buf - 1 - n);
		if(m == 0){
			if(debug & DBGPLAY)
				fprint(2, "playlistproc: empty read\n");
			continue;
		}
		if(m < 0){
			rerrstr(buf, sizeof(buf));
			if(strcmp(buf, "reading past eof"))
				sysfatal("%s: %r", playlistfile);
			for(n = 0; n < playlist.nentries; n++){
				free(playlist.entry[n].file);
				free(playlist.entry[n].onum);
			}
			if(debug & DBGPLAY)
				fprint(2, "playlistproc: trunc\n");
			playlist.nentries = 0;
			free(playlist.entry);
			playlist.entry = nil;
			updateplaylist(1);
			seek(fd, 0, 0);
			p = buf;
			n = 0;
			continue;
		}
		if(debug & DBGPLAY)
			fprint(2, "playlistproc: read %d bytes\n", m);
		n += m;
		p[n] = '\0';
		while(q = strchr(p, '\n')){
			*q = 0;
			nf = tokenize(p, fields, nelem(fields));
			if(nf){
				playlist.entry = realloc(playlist.entry, (playlist.nentries+1)*sizeof playlist.entry[0]);
				if(playlist.entry == nil)
					sysfatal("realloc %r");
				playlist.entry[playlist.nentries].file = strdup(fields[0]);
				if(nf > 1){
					playlist.entry[playlist.nentries].onum = strdup(fields[1]);
					if(debug & DBGPLAY)
						fprint(2, "playlistproc: [%d]: %q %q\n", playlist.nentries,
							playlist.entry[playlist.nentries].file,
							playlist.entry[playlist.nentries].onum);
				}else{
					playlist.entry[playlist.nentries].onum = nil;
					if(debug & DBGPLAY)
						fprint(2, "playlistproc: [%d]: %q nil\n", playlist.nentries,
							playlist.entry[playlist.nentries].file);
				}
				updateplaylist(0);	// this will also update nentries
			}
			q++;
			n -= q-p;
			p = q;
		}
		if(n)
			memmove(buf, p, n);
		p = buf;
	}
}

void
sendplaylist(char *file, char *onum)
{
	static int fd = -1;
	char *b;

	if(file == nil){
		if(fd >= 0)
			close(fd);
		fd = open(playlistfile, OWRITE|OTRUNC);
		if(fd < 0)
			sysfatal("%s: truncate: %r", playlistfile);
		return;
	}
	if(fd < 0){
		fd = open(playlistfile, OWRITE);
		if(fd < 0)
			sysfatal("%s: %r", playlistfile);
	}
	b = smprint("%q	%q\n", file, onum);
	if(debug & DBGPLAY)
		fprint(2, "sendplaylist @%s@\n", b);
	if(write(fd , b, strlen(b)) != strlen(b))
		sysfatal("sendplaylist: %r");
}

void
playctlproc(void*a)
{
	int fd, n, nf;
	static char buf[512+1];
	char *fields[4];
	Channel *chan;

	threadsetname("playctlproc");
	chan = a;
	fd = open(playctlfile, OREAD);
	if(fd < 0)
		sysfatal("%s: %r", playctlfile);
	for(;;){
		n = read(fd, buf, sizeof buf -1);
		if(n == 0)
			continue;
		if(n < 0)
			sysfatal("%s: %r", playctlfile);
		buf[n] = '\0';
		nf = tokenize(buf, fields, nelem(fields));
		if(nf == 0)
			continue;
		switch (nf){
		default:
			sysfatal("playctlproc: [%d]: %s", nf, fields[0]);
		case 3:
			chanprint(chan, "playctlproc: error %lud %q", strtoul(fields[1], nil, 0), fields[2]);
			if(strcmp(fields[0], "error") == 0)
				break;
			// fall through
		case 2:
			chanprint(chan, "playctlproc: %s %lud", fields[0], strtoul(fields[1], nil, 0));
		}
	}
}

void
sendplayctl(char *fmt, ...)
{
	va_list arg;
	static int fd = -1;

	va_start(arg, fmt);
	if(debug & DBGPLAY){
		fprint(2, "sendplayctl: fmt=%s: ", fmt);
		fprint(2, fmt, arg);
		fprint(2, "\n");
	}
	fprint(fd, fmt, arg);
	va_end(arg);
}

void
setvolume(char *volume)
{
	static int fd;

	if(fd == 0){
		fd = open(playvolfile, OWRITE);
		if(fd < 0){
			fprint(2, "can't open %s, (%r) opening %s instead\n", playvolfile, "/dev/volume");
			if((fd = open("/dev/volume", OWRITE)) < 0){
				fprint(2, "setvolume: open: %r\n");
				return;
			}
		}
	}
	if(fd < 0)
		return;
	fprint(fd, "volume %s", volume);
}

void
volumeproc(void *arg)
{
	int fd, n, nf, nnf, i, nlines;
	static char buf[1024];
	char *lines[32];
	char *fields[8];
	char *subfields[8];
	Channel *ctl;
	int volume, minvolume, maxvolume, nvolume;

	ctl = arg;
	threadsetname("volumeproc");
	fd = open(volumefile, OREAD);
	if(fd < 0){
		fprint(2, "%s: %r\n", volumefile);
		threadexits(nil);
	}
	for(;;){
		n = read(fd, buf, sizeof buf -1);
		if(n == 0)
			continue;
		if(n < 0){
			fprint(2, "volumeproc: read: %r\n");
			threadexits("volumeproc");
		}
		buf[n] = '\0';
		nlines = getfields(buf, lines, nelem(lines), 1, "\n");
		for(i = 0; i < nlines; i++){
			nf = tokenize(lines[i], fields, nelem(fields));
			if(nf == 0)
				continue;
			if(nf != 6 || strcmp(fields[0], "volume") || strcmp(fields[1], "out"))
				continue;
			minvolume = strtol(fields[3], nil, 0);
			maxvolume = strtol(fields[4], nil, 0);
			if(minvolume >= maxvolume)
				continue;
			nnf = tokenize(fields[2], subfields, nelem(subfields));
			if(nnf <= 0 || nnf > 8){
				fprint(2, "volume format error\n");
				threadexits(nil);
			}
			volume = 0;
			nvolume = 0;
			for(i = 0; i < nnf; i++){
				volume += strtol(subfields[i], nil, 0);
				nvolume++;
			}
			volume /= nvolume;
			volume = 100*(volume - minvolume)/(maxvolume-minvolume);
			chanprint(ctl, "volume value %d", volume);
		}
	}
}

void
playvolproc(void*a)
{
	int fd, n, nf, volume, nvolume, i;
	static char buf[256+1];
	static errors;
	char *fields[3], *subfields[9];
	Channel *chan;

	threadsetname("playvolproc");
	chan = a;
	fd = open(playvolfile, OREAD);
	if(fd < 0)
		sysfatal("%s: %r", playvolfile);
	for(;;){
		n = read(fd, buf, sizeof buf -1);
		if(n == 0)
			continue;
		if(n < 0){
			fprint(2, "%s: %r\n", playvolfile);
			threadexits("playvolproc");
		}
		buf[n] = '\0';
		if(debug) fprint(2, "volumestring: %s\n", buf);
		nf = tokenize(buf, fields, nelem(fields));
		if(nf == 0)
			continue;
		if(nf != 2 || strcmp(fields[0], "volume")){
			fprint(2, "playvolproc: [%d]: %s\n", nf, fields[0]);
			if(errors++ > 32)
				threadexits("playvolproc");
			continue;
		}
		nvolume = tokenize(fields[1], subfields, nelem(subfields));
		if(nvolume <= 0 || nvolume > 8){
			fprint(2, "volume format error\n");
			if(errors++ > 32)
				threadexits("playvolproc");
			continue;
		}
		volume = 0;
		for(i = 0; i < nvolume; i++)
			volume += strtol(subfields[i], nil, 0);
		volume /= nvolume;
		chanprint(chan, "volume value %d", volume);
	}
}
