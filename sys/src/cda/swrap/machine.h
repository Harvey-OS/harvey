/*
 * Standard Logic terminal locator.
 *
 * The controller for this device is described in TM77-1273-11
 * by Gene Stark.   It operates as follows:

 * Commands from the PDP11 to the Locator are 4-byte strings
 *	byte 0:	=024 always
 *	byte 1:	data
 *	byte 2: data
 *	byte 3: command code

 * The command codes are as follows:
 *	0	load X register
 *	1	load Y register
 *	2	load X register and move pointer
 *	3	load Y register and move pointer
 *	4	load wire bin display
 *	5	poll Locator for status

 * The Locator responds to all commands by sending back a status
 *	packet.  It also sends back a status packet when pointer
 *	movement stops or when one of the switches is operated.
 *	The status packet is as follows:
 *	byte 1:	=006 always
 *	byte 2:	status code

 * The status code is a series of bits as follows:
 *	001	footswitch 1, set when switch open.
 *	002	footswitch 0, set when switch open.
 * 	004	pointer interlock, set when switch activated.
 *	010	pointer drive idle, set when not moving.
 *	020	receive error, error during command transmission.
 *	040	illegal command
 *	100	command acknowledge, sent when good command received.
 *
 */

/* escape characters */
#define CMD	024
#define STAT	06
#define EOFC	-1
#define DEL	0177

/* status bits */
#define FSWCH1	0001
#define FSWCH0	0002
#define EMICRO	0004
#define MDONE	0010
#define RERR	0020
#define ILLCMD	0040
#define CACK	0100

/* command codes */
#define LOADX	0
#define LOADY	1
#define LOADGOX	2
#define LOADGOY	3
#define LOADBIN	4
#define SENSTAT	5

/* wire bin character codes */
#define DASH	0100

/* left and right-hand direction characters */
#define DR	070
#define UR	061
#define UL	007
#define DL	016
#define UP	060
#define DN	006

#define STRIP	50
