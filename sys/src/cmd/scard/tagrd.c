#include <u.h>
#include <libc.h>
#include "plumb.h"
#include "icc.h"
#include "nfc.h"
#include "reader.h"


uchar atr[] = {
	0x3B, 0xBE, 0x96, 0x00, 0x00, 0x41, 0x03, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x90, 0x00
};

void
usage(void)
{
	fprint(2, "usage: tagrd [-D] /dev/cci*/rpc\n");
	exits("usage");
}

int iccdebug;
Plumbmsg m;
uchar buf[128];

char app[] = "appeared";
char disapp[] = "disappeared";

void
main(int argc, char *argv[])
{
	int nw, i, fd, pfd;
	int nbytes;
	char *s;
	Atq a;
	char *msg, *lastmsg, *disamsg;

	m.src = "tag";
	m.dst = nil;
	m.wdir = nil;
	m.type = "text";
	m.attr = nil;

	lastmsg = nil;

	ARGBEGIN{
	case 'D':
		iccdebug = 1;
		break;
	default:
		usage();
	}ARGEND

	if(argc != 1)
		usage();
	fd = open(*argv, ORDWR);
	if(fd < 0)
		sysfatal("tagrd: cannot open %s %r", argv[1]);

	fmtinstall('H',  encodefmt);

	pfd = plumbopen("send", OWRITE);
	if(fd < 0){
		fprint(2, "tagrd: can't open plumb file: %r\n");
		sysfatal("open");
	}

	nw = write(fd, atr, 19);
	if(nw < 0){
		sysfatal("tagrd: error writing to %s %r", argv[1]);
	}
	diprint(2, "tagrd: detected atr %d %r\n", nw);

	s = ttaggetfware(fd);
	if(s != nil)
		diprint(2, "tagrd: read fware %s\n", s);
	else
		diprint(2, "tagrd: error\n");
	setleds(fd, MskR|EndRon|MskRblink|MskGblink, 0, 0, 0, NoBuzz);
	antenna(fd, AOFF);

	for(;;){
		sleep(500);
		diprint(2, "tagrd: ---------------------\n");
		nbytes = tagrdread(fd, buf, sizeof buf, 0, &a);
		if(nbytes < 0){
			diprint(2, "tagrd: no tag %r\n");
			if(lastmsg){
				setleds(fd, EndRon|MskR|MskRblink|MskGblink, 0, 0, 0, NoBuzz);
				disamsg = malloc(strlen(lastmsg) + 1- strlen(app)+ strlen(disapp));
				snprint(disamsg, strlen(lastmsg) + 1- strlen(app), "%s", lastmsg);
				strcat(disamsg, disapp);
				diprint(2, "plumbing: [%s]\n", disamsg);
				m.data = disamsg;
				m.ndata = strlen(disamsg);
				if(plumbsend(pfd, &m) < 0){
					fprint(2, "tagrd: can't send message: %r\n");
				}
				free(disamsg);
				free(lastmsg);
				lastmsg = nil;
			}
			continue;
		}
		for(i = 0; i < nbytes; i++){
			if(buf[i] != 0)
				break;
		}
		if(i == nbytes){
			diprint(2, "tagrd: tag disappeared midways\n");
			continue;
		}
		diprint(2, "tagrd: autopoll read %d bytes\n", nbytes);
		diprint(2, "TAG");
		for(i = 0; i < nbytes; i++){
			diprint(2, "%2.2ux ", buf[i]);
		}
		diprint(2, "\n");
		for(i = 0; i < nbytes; i++){
			diprint(2, "%c", buf[i]);
		}
		diprint(2, "\n");

		msg = smprint("%23.23s UID:%.*H %s", (char *)buf+21, a.nuid, (char *)(a.uid), app);

		if((lastmsg == nil) || strcmp(msg, lastmsg) != 0){
			setleds(fd, MskR|MskG|EndGon|MskRblink|MskGblink, 2, 0, 0, NoBuzz);
			diprint(2, "plumbing: [%s]\n", msg);
			m.data = msg;
			m.ndata = strlen(msg);
			if(plumbsend(pfd, &m) < 0){
				fprint(2, "tagrd: can't send message: %r\n");
			}
		}
		
		free(lastmsg);
		lastmsg = msg;
	}
	/*	free(msg);
	 *	close(fd);
	 *	closde(pfd);
	 */
}
