/* config.c */
Audiofunc *getaudiofunc(Dinf*);

/* controls.c */
Nexus *findnexus(char *name);
Audiocontrol *findcontrol(Nexus *nx, char *name);
void	calcbounds(Nexus *nx);
int	setcontrol(Nexus *nx, char *name, long *value);
int	getcontrol(Nexus *nx, char *name, long *value);
void	getcontrols(Nexus *nx);
int	ctlparse(char *s, Audiocontrol *c, long *v);
int	Aconv(Fmt *fp);

/* dump.c */
void dumpaudiofunc(Audiofunc*);

/* fs.c */
void	serve(void *);
void	ctlevent(void);
