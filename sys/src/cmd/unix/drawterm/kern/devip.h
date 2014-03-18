enum
{
	S_TCP,
	S_UDP
};

int		so_socket(int, unsigned char*);
void		so_connect(int, unsigned char*, unsigned short);
void		so_getsockname(int, unsigned char*, unsigned short*);
void		so_bind(int, int, unsigned short, unsigned char*);
void		so_listen(int);
int		so_send(int, void*, int, int);
int		so_recv(int, void*, int, int);
int		so_accept(int, unsigned char*, unsigned short*);
int		so_getservbyname(char*, char*, char*);
int		so_gethostbyname(char*, char**, int);

char*	hostlookup(char*);

