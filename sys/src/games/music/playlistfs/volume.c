#include <u.h>
#include <libc.h>
#include <thread.h>
#include <fcall.h>
#include "pool.h"
#include "playlist.h"

int	minvolume, maxvolume;

void
volumeproc(void *)
{
	int fd, n, nf, i, nlines;
	static char buf[1024];
	char *lines[32];
	char *fields[8];
	char *subfields[9];
	int volume[8], nvolumes;

	threadsetname("volumeproc");
	close(srvfd[1]);
	fd = open("/dev/audioctl", OREAD);
	if (fd < 0)
		threadexits(nil);
	for(;;){
		n = read(fd, buf, sizeof buf -1);
		if (n == 0)
			continue;
		if (n < 0){
			fprint(2, "volumeproc: read: %r\n");
			threadexits("volumeproc");
		}
		buf[n] = '\0';
		nlines = getfields(buf, lines, nelem(lines), 1, "\n");
		for(i = 0; i < nlines; i++){
			nf = tokenize(lines[i], fields, nelem(fields));
			if (nf == 0)
				continue;
			if (nf != 6 || strcmp(fields[0], "volume") || strcmp(fields[1], "out"))
				continue;
			minvolume = strtol(fields[3], nil, 0);
			maxvolume = strtol(fields[4], nil, 0);
			if (minvolume >= maxvolume)
				continue;
			nvolumes = tokenize(fields[2], subfields, nelem(subfields));
			if (nvolumes <= 0 || nvolumes > 8)
				sysfatal("bad volume data");
			if (debug)
				fprint(2, "volume changed to '");
			for (i = 0; i < nvolumes; i++){
				volume[i] = strtol(subfields[i], nil, 0);
				volume[i]= 100*(volume[i]- minvolume)/(maxvolume-minvolume);
				if (debug)
					fprint(2, " %d", volume[i]);
			}
			if (debug)
				fprint(2, "'\n");
			while (i < 8)
				volume[i++] = Undef;
			send(volumechan, volume);
		}
	}
}

void
volumeset(int *v)
{
	int fd, i;
	char buf[256], *p;

	fd = open("/dev/audioctl", OWRITE);
	if (fd < 0){
		fd = open("/dev/volume", OWRITE);
		if (fd < 0){
			fprint(2, "Can't set volume: %r");
			return;
		}
		fprint(fd, "audio out %d",  v[0]);
		send(volumechan, v);
	} else {
		p = buf;
		for (i = 0; i < 8; i++){
			if (v[i] == Undef) break;
			p = seprint(p, buf+sizeof buf, (p==buf)?"volume out '%d":" %d",
				minvolume + v[i] * (maxvolume-minvolume) / 100);
		}
		p = seprint(p, buf+sizeof buf, "'\n");
		write(fd, buf, p-buf);
	}
	close(fd);
}
