typedef enum { Black, Grey, White } Shade;
extern void G_init(int, int, int, int, int);
extern void G_exit(void);
extern void G_flush(void);

extern void G_rect(int, int, int, int, Shade);
extern void G_outline(int, int, int, int);
extern void G_quad(double, double, double, double, double, double, double, double, Shade);
extern void G_poly(double *, int, Shade);
extern void G_line(int, int, int, int, Shade);
extern void G_disc(int, int, int, Shade);

#define		DPI		100

extern void polyfill(int *num, double **ff, int dark);
