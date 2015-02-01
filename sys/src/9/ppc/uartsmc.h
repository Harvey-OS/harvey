/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */


typedef struct UartData UartData;
struct UartData
{
	int	smcno;	/* smc number: 0 or 1 */
	SMC	*smc;
	Uartsmc	*usmc;
	char	*rxbuf;
	char	*txbuf;
	BD*	rxb;
	BD*	txb;
	int	initialized;
	int	enabled;
} uartdata[Nuart];

int baudgen(int);
void smcsetup(Uart *uart);
