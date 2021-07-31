typedef struct Bank	Bank;
typedef struct Bootconf	Bootconf;

struct Bootconf
{
	int		nbank;
	Bank		*bank;
	PCB		*pcb;
	uvlong	maxphys;
	char		*bootargs;
};

struct Bank
{
	uvlong	min;
	uvlong	max;
};

#define	BOOTARGSLEN	(4096)
#define	MAXCONF		32
