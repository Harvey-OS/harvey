#include <u.h>
#include <libc.h>
#include <bio.h>
#include "debug.h"
#include "tap.h"
#include "chain.h"
#include "jtag.h"

/*
	We will only have one active tap at a time.
	For now we will suppose they are concatenated.
	IR will be 1s for the rest of the bits (Bypass the rest)
	DR will be 1 bit per ignored Tap
*/

static uchar  bufch[4*1024];
int
tapshift(JMedium *jmed, ShiftTDesc *req, ShiftRDesc *rep, int tapidx)
{
	int i;
	u32int ones;
	ShiftTDesc lreq;
	Chain *ch;

	lreq = *req;
	ones = ~0;
	memset(bufch, 0, sizeof bufch);
	ch = (Chain *)bufch;

	for(i = 0; i < jmed->ntaps; i++){
		if(req->reg == TapDR){
			if(i == tapidx){
				dprint(DState, "tap %d, DR\n", tapidx);
				putbits(ch, req->buf, req->nbits);
			}
			else {
				dprint(DState, "tap %d, ignoring DR\n", i);
				putbits(ch, &ones, 1);
			}
		}
		else if(req->reg == TapIR){
			if(i == tapidx){
				dprint(DState, "tap %d, IR\n", tapidx);
				putbits(ch, req->buf, req->nbits);
			}
			else {
				dprint(DState, "tap %d, IR\n", tapidx);
				putbits(ch, &ones, jmed->taps[tapidx].irlen);
			}
		}
	}

	lreq.buf = ch->buf;
	lreq.nbits = ch->e - ch->b;
	jmed->TapSm = jmed->taps[tapidx].TapSm;
	if(jmed->regshift(jmed, &lreq, rep) < 0)
		return -1;
	jmed->taps[tapidx].TapSm = jmed->TapSm;

	if(req->op & ShiftIn)
		for(i = 0; i < jmed->ntaps; i++){
			if(req->reg == TapDR){
				if(i == tapidx)
					getbits(req->buf, ch, ch->e - ch->b);
				else
					getbits(&ones, ch, 1);
			}
			else if(req->reg == TapIR){
				if(i == tapidx)
					getbits(req->buf, ch, ch->e - ch->b);
				else 
					getbits(&ones, ch, jmed->taps[tapidx].irlen);
			}
		}
	
	return 0;
}
