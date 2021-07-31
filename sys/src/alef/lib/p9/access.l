#include <alef.h>
#include <a.out.h>

intern
byte omode[] =
{
	0,
	OEXEC,
	OWRITE,
	ORDWR,
	OREAD,
	ORDWR,
	ORDWR,
	ORDWR
};

union Hdr
{
	Exec;
	byte	bmagic[4];
};

int
access(byte *name, int mode)
{
	Hdr h;
	uint magic;
	int fd, i, n, c;
	byte db[DIRLEN];

	if(mode == OREAD){
		if(stat(name, db) >= 0)
			return 0;
		return -1;
	}
	mode &= 7;
	fd = open(name, omode[mode]);
	if(fd < 0)
		return -1;

	if(mode != OEXEC) {
		close(fd);
		return 0;
	}

	n = read(fd, &h, sizeof(h));
	close(fd);
	if(n < 0)
		return -1;

	if(h.bmagic[0] == '#' && h.bmagic[1] == '!') {
		for(i = 2; i < n; i++) {
			c = h.bmagic[i];
			if(c == ' ' || c == '\n')
				break;
		}
		h.bmagic[i] = '\0';
		return access(h.bmagic+2, OEXEC);
	}

	magic  = h.bmagic[0]<<24;
	magic |= h.bmagic[1]<<16;
	magic |= h.bmagic[2]<<8;
	magic |= h.bmagic[3];

	if(magic == MAGIC)
		return 0;

	werrstr("exec header invalid");
	return -1;
}
