#include <u.h>
#include <libc.h>
#include <bio.h>

#include "modem.h"

int
faxsend(Modem *m, int argc, char *argv[])
{
	int c, count, r, flow;
	char buf[128];

	verbose("faxsend");
	if((r = initfaxmodem(m)) != Eok)
		return r;

	/* telco just does the dialing */
	r = response(m, 120);
	switch(r){
	case Rok:
		break;
	default:
		r = seterror(m, Enoanswer);
		return r;
		
	}

	xonoff(m, 1);
	verbose("sending");
	m->pageno = 1;
	while(argc--){
		if(m->pageno != 1)
			sleep(1000);	/* let the paper catch up */

		m->valid &= ~(Vfhng|Vfet|Vfpts|Vftsi|Vfdcs);
		if((r = openfaxfile(m, *argv)) != Eok)
			return r;

		verbose("sending geometry");
		sprint(buf, "AT+FDT=%ld,%ld,%ld,%ld", m->df, m->vr, m->wd, m->ln);
		if(command(m, buf) != Eok)
			goto buggery;
		if(response(m, 20) != Rconnect){
			r = seterror(m, Eincompatible);
			goto buggery;
		}

		/*
		 * Write the data, stuffing DLE's.
		 * After each bufferfull check if the remote
		 * sent us anything, e.g. CAN to abort.
		 * This also flushes out the ^S/^Q characters
		 * which the driver insists on sending us.
		 * (Could fix the driver, of course...).
		 */
		verbose("sending data");
		for(;;){
			flow = 0;
			count = 0;
			c = 0;
			while(count < sizeof(buf)-1){
				if((c = Bgetc(m->bp)) < 0)
					break;
				buf[count++] = c;
				if(c == '\020')
					buf[count++] = c;
			}
			verbose("sending %d bytes", count);
			if(count && write(m->fd, buf, count) < 0){
				verbose("write failed: %r");
				r = seterror(m, Esys);
				goto buggery;
			}
			/*
			 *  this does really rough flow control since the
			 *  avanstar is even worse
			 */
			verbose("flow control");
			while((r = rawmchar(m, buf)) == Eok || flow){
				if(r != Eok){
					if(flow-- == 0)
						break;
					sleep(250);
					continue;
				}
				switch(buf[0]){
				case '\030':
					verbose("%c", buf[0]);
					if(write(m->fd, "\020\003", 2) < 0){
						r = seterror(m, Esys);
						goto buggery;
					}
					goto okexit;
				case '\021':
					flow = 0;
					break;
				case '\023':
					flow = 4;
					break;
				case '\n':
					break;
				default:
					verbose("%c", buf[0]);
					r = seterror(m, Eproto);
					goto buggery;
				
				}
			}
			if(c < 0)
				break;
		}


		/*
		 * End of page, send DLE+ETX,
		 * get OK in response.
		 */
		verbose("sending end of page");
		if(write(m->fd, "\020\003", 2) < 0){
			r = seterror(m, Esys);
			goto buggery;
		}
		verbose("waiting for OK");
		if(response(m, 120) != Rok){
			r = seterror(m, Enoresponse);
			goto buggery;
		}

		/*
		 * Did you hear me? - IT'S THE END OF THE PAGE.
		 * Argument is 0 if more pages to follow.
		 * Should get back an FPTS with an indication
		 * as to whether the page was successfully
		 * transmitted or not.
		 */
		sprint(buf, "AT+FET=%d", argc == 0? 2: 0);
		if(command(m, buf) != Eok)
			goto buggery;
		switch(response(m, 20)){
		case Rok:
			break;
		case Rhangup:
			if(m->fhng == 0 && argc == 0)
				break;
			r = seterror(m, Eproto);
			goto buggery;
		default:
			r = seterror(m, Enoresponse);
			goto buggery;
		}

		if((m->valid & Vfpts) == 0 || m->fpts[0] != 1){
			r = seterror(m, Eproto);
			goto buggery;
		}

		Bterm(m->bp);
		m->pageno++;
		argv++;
	}
okexit:
	xonoff(m, 0);
	return Eok;

buggery:
	xonoff(m, 0);
	Bterm(m->bp);
	command(m, "AT+FK");
	response(m, 5);
	return r;
}
