
/* FCR */
#define	FPINEX	(1<<7)
#define	FPOVFL	(1<<9)
#define	FPUNFL	(1<<8)
#define	FPZDIV	(1<<10)
#define	FPRNR	(0<<0)
#define	FPRZ	(1<<0)
#define	FPRPINF	(2<<0)
#define	FPRNINF	(3<<0)
#define	FPRMASK	(3<<0)
#define	FPPEXT	0
#define	FPPSGL	0
#define	FPPDBL	0
#define	FPPMASK	0
/* FSR */
#define	FPAINEX	(1<<2)
#define	FPAOVFL	(1<<4)
#define	FPAUNFL	(1<<3)
#define	FPAZDIV	(1<<5)

extern	unsigned long	getfcr(void);
extern	unsigned long	getfsr(void);
extern	void	setfcr(unsigned long);
extern	void	setfsr(unsigned long);
extern	double	NaN(void);
extern	double	Inf(int);
extern	int	isNaN(double);
extern	int	isInf(double, int);
