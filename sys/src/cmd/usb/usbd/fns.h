/* hub.c */
Hub* roothub(int);
Hub* newhub(Hub*, Device*);
void	freehub(Hub*);
int	Hfmt(Fmt*);
int	portenable(Hub*, int, int);
int	portreset(Hub*, int);
int	portpower(Hub*, int, int);
int	portstatus(Hub*, int);

/* setup.c */
void	devspeed(Device*, int);
int	getmaxpkt(Device*);
int	setaddress(Device*, int);
int	setconfig(Device*, int);
int	setalternate(Device*, int, int);

/* usbd.c */
void	roothubproc(void *);
void	enumerate(void *);
Device* configure(Hub *h, int port);
void detach(Hub *h, int port);
