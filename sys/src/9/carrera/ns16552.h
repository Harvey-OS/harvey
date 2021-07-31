#define outb(p, ch)			*(uchar*)((p)^7) = ch	
#define inb(p)	*(uchar*)((p)^7)
#define uartwrreg(u,r,v)	outb((u)->port + (r), (u)->sticky[r] | (v))
#define uartrdreg(u,r)		*(uchar*)(((u)->port + (r))^7)

#define uartpower(x, y)

void
ns16552install(void)
{
	static int already;

	if(already)
		return;
	already = 1;

	ns16552setup(Uart1, UartFREQ, "eia0", Ns550);
}

