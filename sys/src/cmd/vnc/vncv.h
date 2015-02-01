/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

/* color.c */
extern	void		choosecolor(Vnc*);
extern	void		(*cvtpixels)(uchar*, uchar*, int);
extern  void            settranslation(Vnc*);

/* draw.c */
extern	void		sendencodings(Vnc*);
extern	void		requestupdate(Vnc*, int);
extern	void		readfromserver(Vnc*);

extern	uchar	zero[];

/* vncv.c */
extern	char		*encodings;
extern	int		bpp12;
extern	Vnc*		vnc;
extern	int		mousefd;

/* wsys.c */
extern	void		readkbd(Vnc*);
extern	void		initmouse(void);
extern	void		mousewarp(Point);
extern	void		readmouse(Vnc*);
extern  void            senddim(Vnc*);
extern  void            writesnarf(Vnc*, long);
extern  void            checksnarf(Vnc*);
