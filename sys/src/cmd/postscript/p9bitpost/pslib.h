char *psinit(int, int);		/* second arg is debug flag; returns "" on success */
int image2psfile(int, Memimage*, int);
void psopt(char *, void *);

int paperlength;
int paperwidth;
