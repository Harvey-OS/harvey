/* color.c */
extern	void		choosecolor(Vnc*);
extern	void		(*cvtpixels)(uchar*, uchar*, int);
extern  void            settranslation(Vnc*);

/* draw.c */
extern	void		sendencodings(Vnc*);
extern	void		requestupdate(Vnc*, int);
extern	void		readfromserver(Vnc*);

extern	uchar	zero[];

/* vncviewer.c */
extern	char		*encodings;
extern	int		bpp12;
extern	Vnc*		vnc;

/* wsys.c */
extern	void		readkbd(Vnc*);
extern	void		readmouse(Vnc*);
extern  void            senddim(Vnc*);
extern  void            writesnarf(Vnc*, long);
extern  void            checksnarf(Vnc*);
