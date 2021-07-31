#include	"all.h"

static
struct
{
	char*	name;
	int	type;
	int	targ;
} probe[] =
{
	"SEAGATE ST42400N",		Ddisk,	Ctlrtarg,
	"SEAGATE ST41520N",		Ddisk,	Ctlrtarg,
	"SEAGATE ST410800N",		Ddisk,	Ctlrtarg,
	"IMS     CDD521/10",		Dphil,	Ctlrtarg,
	"NEC     CD-ROM DRIVE:5001.0",	Dnec,	Ctlrtarg,
	"NEC     CD-ROM DRIVE:8411.0",	Dnec,	Ctlrtarg,
	"PLEXTOR CD-ROM PX-4XCS",	Dplex,	Ctlrtarg,
	"TOSHIBA CD-ROM DRIVE:XM",	Dtosh,	Ctlrtarg,
	0,
};
static	int	probbed;

void
doprobe(int type, Chan *d)
{
	Chan c;
	uchar cmd[6], resp[255];
	int t, n, i, f;

	if(probbed == 0) {
		for(t=0; t<8; t++) {
			if(t == Ctlrtarg)
				continue;
			c = openscsi(t);
			if(c.open == 0)
				continue;
			memset(cmd, 0, sizeof(cmd));
			memset(resp, 0, sizeof(resp));
			cmd[0] = 0x12;	/* inquiry */
			cmd[4] = sizeof(resp);
			f = 0;
			n = scsi(c, cmd, sizeof(cmd), resp, sizeof(resp), 0);
			if(n >= 5)
				for(i=0; probe[i].name; i++)
					if(strstr((char*)resp+8, probe[i].name)) {
						probe[i].targ = t;
						print("%d %s\n", probe[i].targ, probe[i].name);
						f = 1;
					}
			closescsi(c);
			if(f == 0)
				print("%d %s ** (unknown)\n", t, resp+8);
		}
		probbed = 1;
	}
	for(i=0; probe[i].name; i++)
		if(probe[i].type == type && probe[i].targ != Ctlrtarg) {
			c = openscsi(probe[i].targ);
			if(c.open) {
				*d = c;
				return;
			}
		}
}
