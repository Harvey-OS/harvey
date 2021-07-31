/* hub.c */
Hub* roothub(int);
Hub* newhub(Hub*, Device*);
void	freehub(Hub*);
int	Hfmt(Fmt*);
void	portenable(Hub*, int, int);
void	portreset(Hub*, int);
void	portpower(Hub*, int, int);
int	portstatus(Hub*, int);

/* setup.c */
void	devspeed(Device*, int);
void	setup0(Device*, int, int, int, int, int);
void	setconfig(Device*, int);
int	getmaxpkt(Device*);
int	setaddress(Device*, int);

/* usbd.c */
void	enumerate(void *);
Device* configure(Hub *h, int port);
void detach(Hub *h, int port);
