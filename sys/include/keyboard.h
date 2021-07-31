#pragma src "/sys/src/libdraw"
#pragma lib "libdraw.a"

typedef struct 	Keyboardctl Keyboardctl;
typedef struct	Channel	Channel;

struct	Keyboardctl
{
	Channel	*c;	/* chan(Rune)[20] */

	char		*file;
	int		consfd;		/* to cons file */
	int		ctlfd;		/* to ctl file */
	int		pid;		/* of slave proc */
};


extern	Keyboardctl*	initkeyboard(char*);
extern	int			ctlkeyboard(Keyboardctl*, char*);
extern	void			closekeyboard(Keyboardctl*);

enum {
	KF=	0xF000,	/* Rune: beginning of private Unicode space */
	Khome=	KF|13,
	Kup=	KF|14,
	Kpgup=	KF|15,
	Kprint=	KF|16,
	Kleft=	KF|17,
	Kright=	KF|18,
	Kdown=	0x80,
	Kview=	0x80,
	Kpgdown=	KF|19,
	Kins=	KF|20,
	Kend=	'\r',	/* [sic] */
};
