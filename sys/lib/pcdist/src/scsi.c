#include <u.h>
#include <libc.h>
#include "dat.h"

typedef struct Chan Chan;
struct	Chan
{
	int	open;
	int	fcmd;
	int	fdata;
};

enum
{
	Sread		= 0,
	Swrite,
	Snone		= Swrite,
};

int
scsicmd(Chan c, uchar *cmd, int ccount, uchar *data, int dcount, int io)
{
	uchar resp[4];
	int n;
	long status;

	if(write(c.fcmd, cmd, ccount) != ccount)
		goto bad;
	if(io != Sread)
		n = write(c.fdata, data, dcount);
	else
		n = read(c.fdata, data, dcount);
	if(read(c.fcmd, resp, sizeof(resp)) != sizeof(resp))
		goto bad;

	status = (resp[0]<<24) | (resp[1]<<16) | (resp[2]<<8) | resp[3];
	if(status == 0x6000)
		return n;

bad:
	return -1;
}

int
unitready(Chan c)
{
	uchar cmd[6], resp[4];
	int status, i;

	for(i=0; i<3; i++) {
		memset(cmd, 0, sizeof(cmd));
		cmd[0] = 0x00;	/* unit ready */
		if(write(c.fcmd, cmd, sizeof(cmd)) != sizeof(cmd))
			goto bad;
		write(c.fdata, resp, 0);
		if(read(c.fcmd, resp, sizeof(resp)) != sizeof(resp))
			goto bad;
		status = (resp[0]<<24) | (resp[1]<<16) | (resp[2]<<8) | resp[3];
		if(status == 0x6000 || status == 0x6002)
			return 0;
	bad:;
	}
	return 1;
}

Chan
openscsi(int target)
{
	char scsiname[50];
	Chan c;

	c.open = 0;
	sprint(scsiname, "#S1/%d/cmd", target);
	c.fcmd = open(scsiname, ORDWR);
	if(c.fcmd < 0)
		return c;

	sprint(scsiname, "#S1/%d/data", target);
	c.fdata = open(scsiname, ORDWR);
	if(c.fdata < 0) {
		close(c.fcmd);
		return c;
	}
	if(unitready(c)) {
		close(c.fcmd);
		close(c.fdata);
		return c;
	}
	c.open = 1;
	return c;
}

void
closescsi(Chan c)
{
	if(c.open) {
		close(c.fcmd);
		close(c.fdata);
		c.open = 0;
	}
}

int
scsi(Chan c, uchar *cmd, int ccount, uchar *data, int dcount, int io)
{
	uchar req[6], sense[255];
	int code, key, n, f;

	f = 0;

loop:
	n = scsicmd(c, cmd, ccount, data, dcount, io);
	if(n >= 0)
		return n;

	/*
	 * request sense
	 */
	memset(req, 0, sizeof(req));
	req[0] = 0x03;
	req[4] = sizeof(sense);
	scsicmd(c, req, sizeof(req), sense, sizeof(sense), Sread);

	unitready(c);

	key = sense[2];
	code = sense[12];

	if(code == 0x17 || code == 0x18)	/* recovered errors */
		return dcount;
	if(code == 0x28 && cmd[0] == 0x43 && f == 0) {
		/* get info and media changed */
		f = 1;
		goto loop;
	}
/*
	print("scsi reqsense cmd #%.2x: key #%2.2ux code #%.2ux #%.2ux\n",
		cmd[0], key, code, sense[13]);
*/
	if(key == 0)
		return dcount;
	return -1;
}

void
probe(char *inventry[])
{
	Chan c;
	int t, n, f, ctlrno;
	uchar cmd[6], resp[255];

	ctlrno = 7;
	f = open("#S/scsiid", OREAD);
	if(f >= 0) {
		n = read(f, resp, sizeof(resp));
		if(n >= 0) {
			resp[n] = '\0';
			ctlrno = strtoul((char*)resp, 0, 0);
		}
		close(f);
	}

	for(t=0; t<8; t++) {
		if(t == ctlrno) {
			sprint(inventry[t], "%d: Controller", t);
			continue;
		}
		c = openscsi(t);
		if(c.open == 0) {
			snprint(inventry[t], NWIDTH-1, "%d: Select timeout", t);
			continue;
		}
		memset(cmd, 0, sizeof(cmd));
		memset(resp, 0, sizeof(resp));
		cmd[0] = 0x12;	/* inquiry */
		cmd[4] = sizeof(resp);
		n = scsi(c, cmd, sizeof(cmd), resp, sizeof(resp), 0);
		if(n >= 5)
			snprint(inventry[t], NWIDTH-1, "%d: %s", t, resp+8);
		else
			snprint(inventry[t], NWIDTH-1, "%d: %r", t);
			
		closescsi(c);
	}
	inventry[t] = 0;
}
