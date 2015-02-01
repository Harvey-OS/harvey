/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

/*
 * Prototypes for libplot functions.
 */
void rarc(double, double, double, double, double, double, double);
void box(double, double, double, double);
void cfill(char *);
void circ(double, double, double);
void closepl(void);
void color(char *);
void plotdisc(double, double, double);
void dpoint(double, double);
void erase(void);
void fill(int [], double *[]);
void frame(double, double, double, double);
void grade(double);
void idle(void);
void plotline(double, double, double, double);
void move(double, double);
void openpl(char *);
void parabola(double, double, double, double, double, double);
void pen(char *);
void plotpoly(int [], double *[]);
void ppause(void);
void ptype(char *);
void range(double, double, double, double);
void restore(void);
void rmove(double, double);
void rvec(double, double);
void sbox(double, double, double, double);
void splin(int, int [], double *[]);
void text(char *);
void vec(double, double);
void save(void);
char *whoami(void);
void doublebuffer(void);
