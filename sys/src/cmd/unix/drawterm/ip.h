enum
{
	S_TCP,
	S_UDP
};

int		so_socket(int type);
void		so_connect(int, unsigned long, unsigned short);
void		so_getsockname(int, unsigned long*, unsigned short*);
void		so_bind(int, int, unsigned short);
void		so_listen(int);
int		so_accept(int, unsigned long*, unsigned short*);
int		so_getservbyname(char*, char*, char*);
int		so_gethostbyname(char*, char**, int);

void		hnputl(void *p, unsigned long v);
void		hnputs(void *p, unsigned short v);
unsigned long	nhgetl(void *p);
unsigned short	nhgets(void *p);
unsigned long	parseip(char *to, char *from);
