#define uartwrreg(u,r,v)	outb((u)->port + (r), (u)->sticky[r] | (v))
#define uartrdreg(u,r)		inb((u)->port + (r))

#define uartpower(x, y)

void ns16552setup(ulong, ulong, char*, int);
void ns16552intr(int);
void uartclock(void);

/*
 *  handle an interrupt to a single uart
 */
static void
ns16552intrx(Ureg*, void* arg)
{
	ns16552intr((ulong)arg);
}

void
ns16552install(void)
{
	static int already;

	if(already)
		return;
	already = 1;

	if(ioalloc(Uart0, 8, 0, "eia0") < 0)
		print("eia0: port %d in use\n", Uart0);
	ns16552setup(Uart0, UartFREQ, "eia0", Ns550);
	intrenable(IrqUART0, ns16552intrx, (void*)0, BUSUNKNOWN, "eia0");
	if(ioalloc(Uart1, 8, 0, "eia1") < 0)
		print("eia1: port %d in use\n", Uart1);
	ns16552setup(Uart1, UartFREQ, "eia1", Ns550);
	intrenable(IrqUART1, ns16552intrx, (void*)0, BUSUNKNOWN, "eia1");
	addclock0link(uartclock);
}

#define RD(r)	inb(Uart0+(r))
static void
ns16552iputc(char c)
{
	mb();
	while((RD(5) & (1<<5)) == 0)
		mb();
	outb(Uart0, c);
	mb();
	while((RD(5) & (1<<5)) == 0)
		mb();
}

