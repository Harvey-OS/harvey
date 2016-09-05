
/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

/*
 * Count bytes within runes, if it fits in a uint64_t, and other things.
 */
#include <u.h>
#include <libc.h>
static int packetsize = 3;

void
main(int argc, char *argv[])
{
	int intellimouse = 0;
	int shift = 0;
	int ps2fd;
	int mousefd;
	int c;
	static short msg[4];
	static int nb;
	static uint8_t b[] = {0, 1, 4, 5, 2, 3, 6, 7, 0, 1, 2, 3, 2, 3, 6, 7 };
	int buttons, dx, dy;

	ps2fd = open("#P/ps2mouse", OREAD);
	if (ps2fd < 0) {
		print("#P/ps2mouse: %r");
		exits("can't open #P/ps2mouse");
	}
	mousefd = open("/dev/mousein", OWRITE);
	if (mousefd < 0) {
		print("/dev/mousein: %r");
		exits("can't open /dev/mouse");
	}
	while (read(ps2fd, &c, 1) > 0) {

		/*
		 * non-ps2 keyboards might not set shift
		 * but still set mouseshifted.
		 */
		//shift |= mouseshifted;
		/* 
		 *  check byte 0 for consistency
		 */
		if(nb==0 && (c&0xc8)!=0x08)
			if(intellimouse && (c==0x00 || c==0x01 || c==0xFF)){
				/* last byte of 4-byte packet */
				packetsize = 4;
				continue;
			}
		
		msg[nb] = c;
		if(++nb == packetsize){
			nb = 0;
			if(msg[0] & 0x10)
				msg[1] |= 0xFF00;
			if(msg[0] & 0x20)
				msg[2] |= 0xFF00;
			
			buttons = b[(msg[0]&7) | (shift ? 8 : 0)];
			if(intellimouse && packetsize==4){
				if((msg[3]&0xc8) == 0x08){
					/* first byte of 3-byte packet */
					packetsize = 3;
					msg[0] = msg[3];
					nb = 1;
					/* fall through to emit previous packet */
				}else{
					/* The AccuPoint on the Toshiba 34[48]0CT
					 * encodes extra buttons as 4 and 5. They repeat
					 * and don't release, however, so user-level
					 * timing code is required. Furthermore,
					 * intellimice with 3buttons + scroll give a
					 * two's complement number in the lower 4 bits
					 * (bit 4 is sign extension) that describes
					 * the amount the scroll wheel has moved during
					 * the last sample. Here we use only the sign to
					 * decide whether the wheel is moving up or down
					 * and generate a single button 4 or 5 click
					 * accordingly.
					 */
					if((msg[3] >> 3) & 1) 
						buttons |= 1<<3;
					else if(msg[3] & 0x7) 
						buttons |= 1<<4;
				}
			}
			dx = msg[1];
			dy = -msg[2];
			//mousetrack(dx, dy, buttons, TK2MS(MACHP(0)->ticks));
			fprint(mousefd,"m %d %d %#x\n", dx, dy, buttons);
		}
	}
}
