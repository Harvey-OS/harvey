typedef struct Label	Label;

/* this must have a sp */
struct Label
{
	unsigned long	pc;
	unsigned long	sp;
};

extern "C" {	/* like setjmp and longjmp */
	int	setlabel(Label *);
	void	gotolabel(long, Label *);	/* doesn't return */
	Label	upframe(Label);
	unsigned long	*argbase(Label);
	Label	movelabel(Label, unsigned long newsp);
	unsigned long	*stackbase(Label);
}
