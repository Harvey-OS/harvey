#define UART_DAT 0xf300 /*  PCS6- */
#define UART_GER 0xf301
#define UART_BANK 0xf302
#define UART_LCR 0xf302 /* line control reg */
#define UART_MCR 0xf302
#define UART_LSR 0xf302
#define UART_MSR 0xf302

	outbyte(UART_BANK, 0); /* bank 0 -- 8250 mode */
	outbyte(UART_LCR, 0x80); /* sel baud rate divide regs */
	outbyte(UART_LCR, 0x80); /* sel baud rate divide regs */
	outbyte(UART_DAT, 196); /* baud rate divide lo = 196 */
	outbyte(UART_GER, 0); /* baud rate divide hi = 0 */
	outbyte(UART_LCR, 0x07); /* no parity, 1 stop, 8-bits */
