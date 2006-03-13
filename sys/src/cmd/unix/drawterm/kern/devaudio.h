enum
{
	Fmono		= 1,
	Fin		= 2,
	Fout		= 4,

	Vaudio		= 0,
	Vsynth,
	Vcd,
	Vline,
	Vmic,
	Vspeaker,
	Vtreb,
	Vbass,
	Vspeed,
	Vpcm,
	Nvol,
};

void	audiodevopen(void);
void	audiodevclose(void);
int	audiodevread(void*, int);
int	audiodevwrite(void*, int);
void	audiodevgetvol(int, int*, int*);
void	audiodevsetvol(int, int, int);
