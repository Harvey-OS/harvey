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
int		so_send(int, void*, int, int);
int		so_recv(int, void*, int, int);
int		so_accept(int, unsigned long*, unsigned short*);
int		so_getservbyname(char*, char*, char*);
int		so_gethostbyname(char*, char**, int);

char*	hostlookup(char*);

