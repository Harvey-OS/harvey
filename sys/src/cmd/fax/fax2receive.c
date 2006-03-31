#include <u.h>
#include <libc.h>
#include <bio.h>

#include "modem.h"

static char buf[102400];

static int
page(Modem *m, char *spool)
{
	int count, r;
	char c;

	/*
	 * Start data reception. We should receive CONNECT in response
	 * to +FDR, then data reception starts when we send DC2.
	 */
	m->valid &= ~(Vfhng|Vfet|Vfpts);
	if(command(m, "AT+FDR") != Eok)
		return Esys;

	switch(response(m, 30)){

	case Rconnect:
		m->phase = 'C';
		if((r = createfaxfile(m, spool)) != Eok)
			return r;
		if((r = putmchar(m, "\022")) != Eok)
			return r;
		break;

	case Rhangup:
		return Eok;

	default:
		return seterror(m, Eattn);
	}

	/*
	 * Receive data.
	 */
	verbose("starting page %d", m->pageno);
	count = 0;
	while((r = getmchar(m, &c, 6)) == Eok){
		if(c == '\020'){
			if((r = getmchar(m, &c, 3)) != Eok)
				break;
			if(c == '\003')
				break;
			if(c != '\020'){
				verbose("B%2.2ux", c);
				continue;
			}
		}
		buf[count++] = c;
		if(count >= sizeof(buf)){
			if(write(m->pagefd, buf, count) < 0){
				close(m->pagefd);
				return seterror(m, Esys);
			}
			count = 0;
		}	
	}
	verbose("page %d done, count %d", m->pageno, count);
	if(count && write(m->pagefd, buf, count) < 0){
		close(m->pagefd);
		return seterror(m, Esys);
	}
	if(r != Eok)
		return r;

	/*
	 * Wait for either OK or ERROR.
	 */
	switch(r = response(m, 20)){

	case Rok:
	case Rrerror:
		return Eok;

	default:
		verbose("page: response %d", r);
		return Eproto;
	}
}

static int
receive(Modem *m, char *spool)
{
	int r;

   loop:
	switch(r = page(m, spool)){

	case Eok:
		/*
		 * Check we have a valid page reponse.
		 */
		if((m->valid & Vfhng) == 0 && (m->valid & (Vfet|Vfpts)) != (Vfet|Vfpts)){
			verbose("receive: invalid page reponse: #%4.4ux", m->valid);
			return seterror(m, Eproto);
		}

		/*
		 * Was the page successfully received?
		 * If not, try again.
		 */
		if((m->valid & Vfpts) && m->fpts[0] != 1)
			goto loop;

		/*
		 * Another page of the same document, a new document
		 * or no more pages.
		 * If no more pages we still have to get the FHNG, so
		 * the code is just the same as if there was another
		 * page.
		 */
		if(m->valid & Vfet){
			switch(m->fet){

			case 0:				/* another page */
			case 2:				/* no more pages */
				m->pageno++;
				goto loop;

			case 1:				/* new document */
				/*
				 * Bug: currently no way to run the
				 * fax-received process for this, so it
				 * just stays queued.
				 */
				faxrlog(m, Eok);
				m->pageno = 1;
				m->time = time(0);
				m->pid = getpid();
				goto loop;
			}

			verbose("receive: invalid FET: %d", m->fet);
			return seterror(m, Eproto);
		}

		/*
		 * All done or hangup error.
		 * On error remove all pages in the current document.
		 * Yik.
		 */
		if(m->valid & Vfhng){
			if(m->fhng == 0)
				return Eok;
			verbose("receive: FHNG: %d", m->fhng);
			/*
			for(r = 1; r <= m->pageno; r++){
				char pageid[128];

				setpageid(pageid, spool, m->time, m->pid, r);
				remove(pageid);
			}
			 */
			return seterror(m, Eattn);
		}
		/*FALLTHROUGH*/

	default:
		return r;
	}
}

int
faxreceive(Modem *m, char *spool)
{
	int r;

	verbose("faxdaemon");
	if((r = initfaxmodem(m)) != Eok)
		return r;

	/*
	 *  assume that the phone has been answered and
	 *  we have received +FCON
	 */
	m->pageno = 1;
	m->time = time(0);
	m->pid = getpid();
	fcon(m);

	/*
	 * I wish I knew how to set the default parameters on the
	 * MT1432 modem (+FIP in Class 2.0).
	 */
	return receive(m, spool);
}
